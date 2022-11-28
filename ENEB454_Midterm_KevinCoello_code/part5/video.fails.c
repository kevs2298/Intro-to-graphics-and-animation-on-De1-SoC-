#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#include "address_map_arm.h"

/* This kernel module provides a character device driver that controls a video display.
 * The user-interface to the driver is through the file /dev/video. The following commands
 * can be written to the driver through this file: "clear", "line x1,y1 x2,y2 color", "vsync",
 * "swap", "box x1,y1 x2,y2 color".
 * Reading from the /dev/video produces the string "screen_x screen_y", where these are
 * the number of columns and rows on the screen, respectively. */
		
/* Prototypes for functions used for video display */
void get_screen_specs (volatile int *);
void clear_screen (void);
void wait_for_vsync (volatile int *);
void swap_fb (void);
void draw_line(int, int, int, int, short int);
void draw_box(int, int, int, int, short int);
void plot_pixel(int, int, short int);

// START of declarations needed for a character device
static int device_open (struct inode *, struct file *);
static int device_release (struct inode *, struct file *);
static ssize_t device_read (struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

#define ABS(x)			(((x) > 0) ? (x) : -(x))

#define SUCCESS 0
#define DEVICE_NAME "video"
#define VIDEO_BYTES 8					// number of bytes to return when read (includes a \n)

static dev_t dev_no = 0;
static struct cdev *video_cdev = NULL;
static struct class *video_class = NULL;

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release
};
// END of declarations needed for a character device

// START of declarations for the video
void *LW_virtual;             	// used to map physical addresses for the light-weight bridge
volatile int * pixel_ctrl_ptr;	// virtual address of pixel buffer DMA controller
int pixel_buffer_start;				// current location of pixel buffer memory
int pixel_buffer_save;				// "other" location of pixel buffer memory
void *FPGA_ONCHIP_virtual;			// virtual addresses for the FPGA on-chip memory
void *SDRAM_virtual;					// virtual addresses for the SDRAM memory
int resolution_x, resolution_y;	// VGA screen size

// END of declarations

/* Code to initialize the video driver */
static int __init start_video(void)
{
	int err;
	/* START of code to make a character device driver */
	/* First, get a device number. Get one minor number (0) */
	err = alloc_chrdev_region (&dev_no, 0, 1, DEVICE_NAME);
	if (err < 0) {
		printk (KERN_ERR "video driver: alloc_chrdev_region() failed\n");
		return err;
	}
	video_class = class_create (THIS_MODULE, DEVICE_NAME);
	
	// Allocate and initialize the char device
	video_cdev = cdev_alloc (); 
	video_cdev->ops = &fops; 
	video_cdev->owner = THIS_MODULE; 
   
	// Add the character device to the kernel
	err = cdev_add (video_cdev, dev_no, 1);
	if (err < 0) {
		printk (KERN_ERR "video driver: cdev_add() failed\n");
		return err;
	}
	
	// Add the class to the kernel
	device_create (video_class, NULL, dev_no, NULL, DEVICE_NAME );
	/* END of code to make a character device driver */

   // generate a virtual address for the FPGA lightweight bridge
   LW_virtual = ioremap_nocache (LW_BRIDGE_BASE, LW_BRIDGE_SPAN);
	// Create virtual memory access to the FPGA on-chip memory
	FPGA_ONCHIP_virtual = ioremap_nocache (FPGA_ONCHIP_BASE, FPGA_ONCHIP_SPAN);
	// Create virtual memory access to the SDRAM memory
	SDRAM_virtual = ioremap_nocache (SDRAM_BASE, SDRAM_SPAN);

	if ((LW_virtual == NULL) || (FPGA_ONCHIP_virtual == NULL) || (SDRAM_virtual == NULL))
		printk (KERN_ERR "Error: ioremap_nocache returned NULL\n");

	// Set up the pixel buffers
	pixel_ctrl_ptr = (unsigned int *) (LW_virtual + PIXEL_BUF_CTRL_BASE);
	/* Initialize the location of the pixel buffer in the pixel buffer DMA controller. The
	 * DMA controller is connected to the FPGA on-chip memory, and the SDRAM memory. In
	 * this program we are using both of these memories, for the front and back buffers. The
	 * DMA controller requires the physical address of the memory; later in our code we will
	 * write to the pixels in the pixel buffer using virtual addresses that access this same 
	 * physical memory */
	*(pixel_ctrl_ptr + 1) = FPGA_ONCHIP_BASE; 	// First store the physical address of the 
	                                             // back buffer, because we can't directly write 
																// to the register that holds the address of
																// the front buffer
	/* Now, swap the front and back buffers, to initialize the front pixel buffer location */
	wait_for_vsync (pixel_ctrl_ptr);
	/* Initialize a virtual address pointer to the pixel buffer, used by drawing functions */
	pixel_buffer_start = (int) FPGA_ONCHIP_virtual;

	/* Erase the pixel buffer */
	get_screen_specs (pixel_ctrl_ptr);				// determine X, Y screen size
	clear_screen( );

	/* Set a location for the back pixel buffer in the pixel buffer controller */
	*(pixel_ctrl_ptr + 1) = SDRAM_BASE;
	pixel_buffer_save = pixel_buffer_start;		// save this pointer for later swapping
	pixel_buffer_start = (int) SDRAM_virtual;		// we draw on the back buffer

	return 0;
}

/* Function to find the VGA screen specifications */
void get_screen_specs(volatile int * pixel_ctrl_ptr)
{
	int resolution_reg;

	resolution_reg = *(pixel_ctrl_ptr + 2);
	resolution_x = resolution_reg & 0xFFFF;
	resolution_y = resolution_reg >> 16;
}

/* Function to blank the VGA screen */
void clear_screen( )
{
	int y, x;
	int pixel_ptr;

	for (y = 0; y < resolution_y; y++)
	{
		pixel_ptr = pixel_buffer_start + (y << 10);
		for (x = 0; x < resolution_x; x = x + 2)
		{
			*(int *)pixel_ptr = 0;	// clear two pixels at a time 
			pixel_ptr += 4;			// add 4 because each pixel takes two addresses
		}
	}
}

/* This function causes a swap of the front and back buffer addressed in the VGA pixel 
 * controller. The swap takes place only on a vertical sync of the VGA controller, which 
 * happens every is 1/60 seconds. */
void wait_for_vsync(volatile int * pixel_ctrl_ptr)
{
	volatile int status;

	*pixel_ctrl_ptr = 1;									// start the synchronization process

	status = *(pixel_ctrl_ptr + 3);
	while ((status & 0x01) != 0)
	{
		status = *(pixel_ctrl_ptr + 3);
	}
	swap_fb ();
	if (((pixel_buffer_start == SDRAM_virtual) && (*(pixel_ctrl_ptr + 1) != SDRAM_BASE)) ||
		((pixel_buffer_start == FPGA_ONCHIP_virtual) && (*(pixel_ctrl_ptr + 1) != FPGA_ONCHIP_BASE)))
		printk (KERN_ERR "ERROR: pixel_buffer_start: %X\n", *(pixel_ctrl_ptr + 1));
}

/* Swap the virtual address pointers that are used for the front and back buffers */
void swap_fb()
{
	int tmp;
	tmp = pixel_buffer_start;
	pixel_buffer_start = pixel_buffer_save;		// update back buffer pointer
	pixel_buffer_save = tmp;
	/* if (*(pixel_ctrl_ptr + 1) == SDRAM_BASE)
		pixel_buffer_start = SDRAM_virtual;		// update back buffer pointer
	else
		pixel_buffer_start = FPGA_ONCHIP_virtual;		// update back buffer pointer
	*/
}

void draw_box(int x1, int y1, int x2, int y2, short int pixel_color)
{
	int y, x;
	int pixel_ptr;
	
	/* assume that the box coordinates are valid */
	for (y = y1; y <= y2; y++)
	{
		x = x1;
		pixel_ptr = pixel_buffer_start + (y << 10) + (x << 1);
		for ( ; x <= x2; x++)
		{
			*(short *)pixel_ptr = pixel_color;	// set pixel 
			pixel_ptr += 2;							// add 2 because each pixel takes two addresses
		}
	}
}

/* Bresenham's line drawing algorithm. */
void draw_line(int x0, int y0, int x1, int y1, short int color)
/* This function draws a line between points (x0, y0) and (x1, y1). The function assumes
 * that the coordinates are valid */
{
	register int x_0 = x0;
	register int y_0 = y0;
	register int x_1 = x1;
	register int y_1 = y1;
	register char steep = (ABS(y_1 - y_0) > ABS(x_1 - x_0)) ? 1 : 0;
	register int deltax, deltay, tmp, error, ystep, x, y;

	/* Preprocessing inputs */
	if (steep > 0) {
		// Swap x_0 and y_0
		tmp = x_0;
		x_0 = y_0;
		y_0 = tmp;
		// Swap x_1 and y_1
		tmp = x_1;
		x_1 = y_1;
		y_1 = tmp;
	}
	if (x_0 > x_1) {
		// Swap x_0 and x_1
		tmp = x_0;
		x_0 = x_1;
		x_1 = tmp;
		// Swap y_0 and y_1
		tmp = y_0;
		y_0 = y_1;
		y_1 = tmp;
	}

	/* Setup local variables */
	deltax = x_1 - x_0;
	deltay = ABS(y_1 - y_0);
	error = -(deltax / 2); 
	y = y_0;
	if (y_0 < y_1)
		ystep = 1;
	else
		ystep = -1;

	/* Draw a line - either go along the x-axis (steep = 0) or along the y-axis (steep = 1) */
	for (x = x_0; x <= x_1; x++)
	{
		if (steep == 1)
			plot_pixel (y, x, color);
		else
			plot_pixel (x, y, color);
		error = error + deltay;
		if (error > 0) {
			y = y + ystep;
			error = error - deltax;
		}
	}
}

void plot_pixel(int x, int y, short int line_color)
{
	*(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}

/* Remove the character device driver */
static void __exit stop_video(void)
{
	/* unmap the physical-to-virtual mappings */
   iounmap (LW_virtual);
	iounmap (FPGA_ONCHIP_virtual);
	iounmap (SDRAM_virtual);

	/* Remove the device from the kernel */
	device_destroy (video_class, dev_no);
	cdev_del (video_cdev);
	class_destroy (video_class);
	unregister_chrdev_region (dev_no, 1);
}

/* Called when a process opens the video */
static int device_open(struct inode *inode, struct file *file)
{
	return SUCCESS;
}

/* Called when a process closes the video */
static int device_release(struct inode *inode, struct file *file)
{
	return 0;
}

/* Called when a process reads from the video. If *offset = 0, the current value of
 * the video is copied back to the user, and VIDEO_BYTES is returned. But if 
 * *offset = VIDEO_BYTES, nothing is copied back to the user and 0 (EOF) is returned. */
static ssize_t device_read(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
	char themessage[VIDEO_BYTES + 1];			// extra byte is for a NULL terminator
  	size_t bytes;

	bytes = VIDEO_BYTES - (*offset);				// has the time data been sent yet?
	if (bytes)
	{
		sprintf(themessage, "%03d %03d\n", resolution_x, resolution_y);
		if (copy_to_user (buffer, themessage, bytes) != 0)
			printk (KERN_ERR "Error: copy_to_user unsuccessful\n");
	}
	(*offset) = bytes;								// keep track of number of bytes sent to the user
	return bytes;										// returns VIDEO_BYTES, or EOF
}

/* Called when a process writes to the video */
static ssize_t device_write(struct file *filp, const char *buffer, size_t length, loff_t *offset)
{
	char command[256];
	int x1, y1, x2, y2, color;

	if (sscanf (buffer, "%s", command) != 1)
	{
		printk (KERN_ERR "Error: unable to read user input in video module\n");
		return (0);
	}
	if (strcmp (command, "clear") == 0)
	{
		clear_screen ();
		// printk (KERN_ERR "Executed clear_screen\n");
	}
	else if (strcmp (command, "vsync") == 0)
	{
		wait_for_vsync (pixel_ctrl_ptr);
		// printk (KERN_ERR "Executed vsync\n");
	}
	else if (strcmp (command, "swap") == 0)
	{
		swap_fb ();										// finish the synchronization process
		// printk (KERN_ERR "Executed swap_fb\n");
	}
	else
	{
		if (sscanf (buffer, "%s %d,%d %d,%d %X", command, &x1, &y1, &x2, &y2, &color) != 6)
		{
			printk (KERN_ERR "Error: unable to parse user input in video module: %s\n", buffer);
			return (0);
		}
		if (strcmp (command, "line") == 0)
		{
			// printk (KERN_ERR "Executing draw_line (%d, %d, %d, %d, %X)\n", x1,y1,x2,y2,color);
			draw_line (x1, y1, x2, y2, color);
		}
		else if (strcmp (command, "box") == 0)
		{
			// printk (KERN_ERR "Executed draw_box (%d, %d, %d, %d, %X)\n", x1,y1,x2,y2,color);
			draw_box (x1, y1, x2, y2, color);
		}
		else
			printk (KERN_ERR "Error: bad argument to video driver: %s\n", command);
	}
	return length;
}

MODULE_LICENSE("GPL");
module_init (start_video);
module_exit (stop_video);
