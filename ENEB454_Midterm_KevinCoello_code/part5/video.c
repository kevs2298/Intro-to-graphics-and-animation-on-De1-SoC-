#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include "/home/root/tutorial_files/address_map_arm.h"

/* This kernel module provides a character device driver that controls a video display.
 * The user-interface to the driver is through the file /dev/video. The following commands
 * can be written to the driver through this file: "clear", "sync", "pixel x,y color", 
 * "line x1,y1 x2,y2 color", and "box x1,y1 x2,y2 color".
 * Reading from the /dev/video produces the string "screen_x screen_y", where these are
 * the number of columns and rows on the screen, respectively. */
		
/* Prototypes for functions used for video display */
void get_screen_specs (volatile int *);
void clear_screen (void);
void wait_for_vsync (volatile int *);
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
#define DEV_NAME "video"
#define VIDEO_BYTES 8					// number of bytes to return when read (includes a \n)
#define VIDEO_SIZE 25					// max number of bytes accepted for a write 

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
int back_buffer;						// current location of pixel buffer memory
void *FPGA_ONCHIP_virtual;			// virtual addresses for the FPGA on-chip memory
void *SDRAM_virtual;					// virtual addresses for the SDRAM memory
int resolution_x, resolution_y;	// VGA screen size
int pixel_buffer;

// END of declarations

/* Code to initialize the video driver */
static int __init start_video(void)
{
	int err;
	/* START of code to make a character device driver */
	/* First, get a device number. Get one minor number (0) */
	err = alloc_chrdev_region (&dev_no, 0, 1, DEV_NAME);
	if (err < 0) {
		printk (KERN_ERR "/dev/%s: alloc_chrdev_region() failed\n", DEV_NAME);
		return err;
	}
	video_class = class_create (THIS_MODULE, DEV_NAME);
	
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
	device_create (video_class, NULL, dev_no, NULL, DEV_NAME);
	/* END of code to make a character device driver */

	//========= ENEB454 - Add your code here ==========
   	// 1. generate a virtual address for the FPGA lightweight bridge
    LW_virtual = ioremap_nocache (LW_BRIDGE_BASE, LW_BRIDGE_SPAN);
	if (LW_virtual == 0)
 		printk (KERN_ERR "Error: ioremap_nocache returned NULL\n");
	// 2. Create virtual memory access to the FPGA on-chip memory
	FPGA_ONCHIP_virtual =  ioremap_nocache (FPGA_ONCHIP_BASE, FPGA_ONCHIP_SPAN);
	if (FPGA_ONCHIP_virtual == 0)
 		printk (KERN_ERR "Error: ioremap_nocache returned NULL\n");
	// 3. Create virtual memory access to the SDRAM memory
	SDRAM_virtual =  ioremap_nocache (SDRAM_BASE, SDRAM_SPAN);
	if (SDRAM_virtual == 0)
 		printk (KERN_ERR "Error: ioremap_nocache returned NULL\n");

	// 4. Set up the pixel buffers
	pixel_ctrl_ptr = LW_virtual + PIXEL_BUF_CTRL_BASE;

	pixel_buffer = (int) ioremap_nocache (0xC8000000, 0x0003FFFF);
	if (pixel_buffer == 0)
	 	printk (KERN_ERR "Error: ioremap_nocache returned NULL\n");
				
	// determine X, Y screen size
	get_screen_specs (pixel_ctrl_ptr);

	/* 5. Initialize the location of the pixel buffer in the pixel buffer DMA controller. The
	 * DMA controller is connected to the FPGA on-chip memory, and the SDRAM memory. In
	 * this program we are using both of these memories, for the front and back buffers. The
	 * DMA controller requires the physical address of the memory; in our code we write to
	 * the pixels in the back buffer using virtual addresses for this physical memory */
	*(pixel_ctrl_ptr + 1) = FPGA_ONCHIP_BASE;  // set current back buffer address

	/* 6. Initialize a virtual address pointer to the back buffer, used by drawing functions */
	back_buffer =  (int)FPGA_ONCHIP_virtual;

	/* 7. Erase the back buffer */
	clear_screen();

	/* 8. Now, swap the front and back buffers, to initialize the front pixel buffer location */
	wait_for_vsync(pixel_ctrl_ptr);

	/* 9. Set a new location for the back buffer in the pixel buffer controller */
	
	back_buffer = (int) SDRAM_virtual;				// we draw on the back buffer
	/* Erase the back buffer */
	clear_screen( );

	return 0;
}

/* Function to find the VGA screen specifications */
void get_screen_specs(volatile int * pixel_ctrl_ptr)
{
	int resolution_reg;

	resolution_reg = *(pixel_ctrl_ptr + 0x2);//TODO
	resolution_x = (resolution_reg & 0x0000FFFF);//TODO
	resolution_y = (resolution_reg & 0xFFFF0000)>>16;//TODO
}

//========= ENEB454 - Add your code here ==========
/* Function to clear the pixel buffer */
void clear_screen( )
{
	int x, y;
	for (x = 0; x<320;x++){
		for (y=0;y<240;y++){
			plot_pixel(x,y,0);
		}
	}
}

void plot_pixel(int x, int y, short int line_color)
{
	*(short int *)(back_buffer + (y << 10) + (x << 1)) = line_color;
}

//========= ENEB454 - Add your code here ==========
/* This function causes a swap of the front and back buffer addressed in the VGA pixel 
 * controller. The swap takes place only on a vertical sync of the VGA controller, which 
 * happens every is 1/60 seconds. */
void wait_for_vsync(volatile int * pixel_ctrl_ptr)
{
	volatile int status;

	*pixel_ctrl_ptr = 1;											// start the synchronization process

	// TODO
	do{
		status = *(pixel_ctrl_ptr + 0x3) & 0x00000001;
	} while (status != 0); // wait until the S bit turns to 0

	if (*(pixel_ctrl_ptr + 1) == SDRAM_BASE){
		back_buffer = (int)SDRAM_virtual;
	}
	else {
		back_buffer = (int)FPGA_ONCHIP_virtual;

	}
}

//========= ENEB454 - Add your code here ==========
void draw_box(int x1, int y1, int x2, int y2, short int color)
{
	int x, y;
	for (x = x1; x<=x2;x++){
		for (y=y1;y<=y2;y++){
			plot_pixel(x,y,color);
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

	//========= ENEB454 - Add your code here ==========
	/* Preprocessing inputs */
	// Refer to lines 4-9 from Figure 7 in lab
	if (steep){
      tmp = x_0;
      x_0 = y_0;
      y_0 = tmp;

      tmp = x_1;
      x_1 = y_1;
      y_1 = tmp;
   }
   if (x_0 > x_1){
      tmp = x_0;
      x_0 = x_1;
      x_1 = tmp;

      tmp = y_0;
      y_0 = y_1;
      y_1 = tmp;
   }
	/* Setup local variables */
	// Refer to lines 11-15 from Figure 7 in lab
	deltax = x_1 - x_0;
    deltay = ABS(y_1 - y_0);
    error = -(deltax/2);
    y = y_0;
    if (y_0 < y_1){
       ystep = 1;
    }
    else{
       ystep = -1;
    }
	/* Draw a line - either go along the x-axis (steep = 0) or along the y-axis (steep = 1) */
	// Refer to lines 17-22 from Figure 7 in lab
	for(x = x_0; x<= x_1; x++){
      
      if (steep) plot_pixel(y,x,color);
      else plot_pixel(x,y,color);

      error = error + deltay;
      if (error >= 0){
         y = y + ystep;
         error = error - deltax;
      }
   }
}

/* Remove the character device driver */
static void __exit stop_video(void)
{
	*(pixel_ctrl_ptr + 1) = FPGA_ONCHIP_BASE; 	// Restore default state of pixel controller
	/* Now, swap the front and back buffers, to initialize the front pixel buffer location */
	wait_for_vsync (pixel_ctrl_ptr);
	*(pixel_ctrl_ptr + 1) = FPGA_ONCHIP_BASE; 	// Restore default state of pixel controller
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

	bytes = VIDEO_BYTES - (*offset);				// has the video data been sent yet?
	if (bytes)
	{
		sprintf(themessage, "%03d %03d\n", resolution_x, resolution_y);
		if (copy_to_user (buffer, themessage, bytes) != 0)
			printk (KERN_ERR "/dev/%s: copy_to_user unsuccessful\n", DEV_NAME);
	}
	(*offset) = bytes;								// keep track of number of bytes sent to the user
	return bytes;										// returns VIDEO_BYTES, or EOF
}

/* Called when a process writes to the video */
static ssize_t device_write(struct file *filp, const char *buffer, size_t length, loff_t *offset)
{
 	char video_string[VIDEO_SIZE + 1]; 			// extra byte is for a NULL terminator
	int bytes; 
	char command[8];
	int x1, y1, x2, y2, color;

	bytes = length;
	if (bytes > VIDEO_SIZE)						// can copy all at once, or not?
		bytes = VIDEO_SIZE;
	if (copy_from_user (video_string, buffer, bytes) != 0)
		printk (KERN_ERR "/dev/%s: copy_from_user unsuccessful\n", DEV_NAME);

	video_string[bytes] = '\0';
	if (video_string[0] == '-' && video_string[1] == '-')
	{
		printk (KERN_ERR "Usage: --\n");
		printk (KERN_ERR "       clear\n");
		printk (KERN_ERR "       sync\n");
		printk (KERN_ERR "       pixel X,Y color\n");
		printk (KERN_ERR "       line X1,Y1 X2,Y2 color\n");
		printk (KERN_ERR "       box X1,Y1 X2,Y2 color\n");
		printk (KERN_ERR "Notes: X,Y are integers, color is 16-bit hex\n");
	}
	else if (video_string[0] == 'c' && video_string[1] == 'l')
	{
		clear_screen ( );
	}
	else if (video_string[0] == 's' && video_string[1] == 'y')
	{
		//========= ENEB454 - Add your code here ==========
		// TODO
		wait_for_vsync(pixel_ctrl_ptr);
	}
	else if (video_string[0] == 'p' && video_string[1] == 'i')
	{
		if (sscanf (video_string, "%s %d,%d %X", command, &x1, &y1, &color) != 4)
		{
			printk (KERN_ERR "/dev/%s: can't parse user input: %s\n", DEV_NAME, video_string);
		}
		//========= ENEB454 - Add your code here ==========
		// TODO
		plot_pixel(x1,y1,color);
	}
	else
	{
		if (sscanf (video_string, "%s %d,%d %d,%d %X", command, &x1, &y1, &x2, &y2, &color) != 6)
		{
			printk (KERN_ERR "/dev/%s: can't parse user input: %s\n", DEV_NAME, video_string);
		}
		if ((x1 < 0) || (x1 > resolution_x - 1) || (x2 < 0) || (x2 > resolution_x - 1) ||
			(y1 < 0) || (y1 > resolution_y - 1) || (y2 < 0) || (y2 > resolution_y - 1))
			printk (KERN_ERR "/dev/%s: bad coordinates: %s %d,%d %d,%d\n", DEV_NAME, command,
				x1, y1, x2, y2);
		else if (strcmp (command, "line") == 0)
		{
			//========= ENEB454 - Add your code here ==========
			// TODO
			draw_line(x1,y1,x2,y2,color);
		}
		else if (strcmp (command, "box") == 0)
		{
			//========= ENEB454 - Add your code here ==========
			// TODO
			draw_box(x1,y1,x2,y2,color);
		}
		else
			printk (KERN_ERR "/dev/%s: bad argument: %s\n", DEV_NAME, command);
	}
	return length;
}

MODULE_LICENSE("GPL");
module_init (start_video);
module_exit (stop_video);
