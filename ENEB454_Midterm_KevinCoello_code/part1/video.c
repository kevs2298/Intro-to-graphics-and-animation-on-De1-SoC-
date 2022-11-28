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
 * can be written to the driver through this file: "clear" and "pixel x,y color".
 * Reading from the /dev/video produces the string "screen_x screen_y", where these are
 * the number of columns and rows on the screen, respectively. */
		
/* Prototypes for functions used for video display */
void get_screen_specs (volatile int *);
void clear_screen (void);
void plot_pixel(int, int, short int);
void usage(void);

// START of declarations needed for a character device
static int device_open (struct inode *, struct file *);
static int device_release (struct inode *, struct file *);
static ssize_t device_read (struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

#define SUCCESS 0
#define DEV_NAME "video"
#define VIDEO_BYTES 8					// number of bytes to return when read (includes a \n)
#define VIDEO_SIZE 18					// max number of bytes accepted for a write 

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
int pixel_buffer;						// current location of pixel buffer memory
int resolution_x, resolution_y;	// VGA screen size
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
		printk (KERN_ERR "/dev/%s: cdev_add() failed\n", DEV_NAME);
		return err;
	}
	
	// Add the class to the kernel
	device_create (video_class, NULL, dev_no, NULL, DEV_NAME );
	/* END of code to make a character device driver */

	//========= ENEB454 - Add your code here ==========
   	// 1. generate a virtual address for the FPGA lightweight bridge
	LW_virtual = ioremap_nocache (LW_BRIDGE_BASE, LW_BRIDGE_SPAN);
	if (LW_virtual == 0)
 		printk (KERN_ERR "Error: ioremap_nocache returned NULL\n");
	// 2. Create virtual memory access to the pixel buffer controller
	// determine X, Y screen size
	pixel_ctrl_ptr = LW_virtual + PIXEL_BUF_CTRL_BASE;
	get_screen_specs (pixel_ctrl_ptr);

	// 3. Create virtual memory access to the pixel buffer
	pixel_buffer = (int) ioremap_nocache (0xC8000000, 0x0003FFFF);
	if (pixel_buffer == 0)
		printk (KERN_ERR "Error: ioremap_nocache returned NULL\n");
	// Erase the pixel buffer
	clear_screen ( );
	return 0;
}

//========= ENEB454 - Add your code here ==========
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

void plot_pixel(int x, int y, short int color)
{
	*(short int *)(pixel_buffer + (y << 10) + (x << 1)) = color;
}

/* Remove the character device driver */
static void __exit stop_video(void)
{
	/* unmap the physical-to-virtual mappings */
   iounmap (LW_virtual);
	iounmap ((void *) pixel_buffer);

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

void usage( )
{
	printk (KERN_ERR "Usage: --\n");
	printk (KERN_ERR "Usage: clear\n");
	printk (KERN_ERR "       pixel X,Y color\n");
	printk (KERN_ERR "Notes: X,Y are integers, color is 4-digit hex value\n");
}

/* Called when a process writes to the video */
static ssize_t device_write(struct file *filp, const char *buffer, size_t length, loff_t *offset)
{
	char video_string[VIDEO_SIZE + 1]; 			// extra byte is for a NULL terminator
	char command[8];
	int bytes;
	int x, y, color;

	bytes = length;
	if (bytes > VIDEO_SIZE)						// can copy all at once, or not?
		bytes = VIDEO_SIZE;
	if (copy_from_user (video_string, buffer, bytes) != 0)
		printk (KERN_ERR "/dev/%s: copy_from_user unsuccessful\n", DEV_NAME);

	video_string[bytes] = '\0';					// NULL terminate
	
	//========= ENEB454 - Add your code here ==========
	if (video_string[0] == '-' && video_string[1] == '-')
	{
		// TODO
		usage();
	}
	else if (video_string[0] == 'c' && video_string[1] == 'l')
	{
		// TODO
		clear_screen();
	}
	else
	{
		if (sscanf (video_string, "%s %d,%d %X", command, &x, &y, &color) != 4)
		{
			printk (KERN_ERR "/dev/%s: can't parse user input: %s\n", DEV_NAME, video_string);
		}
		if (strcmp (command, "pixel") == 0)
		{
			if ((x < 0) || (x > resolution_x - 1) || (y < 0) || (y > resolution_y - 1))
			{
				printk (KERN_ERR "/dev/%s: bad coordinates: %d,%d\n", DEV_NAME, x, y);
				usage ( );
			}
			else
				plot_pixel (x, y, color);
		}
		else
		{
			printk (KERN_ERR "/dev/%s: bad argument: %s\n", DEV_NAME, command);
			usage ( );
		}
	}
	return length;
}

MODULE_LICENSE("GPL");
module_init (start_video);
module_exit (stop_video);
