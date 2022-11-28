#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "wrappers.h"

/* This file contain wrappers for the slider switch SW character device driver */
int SW_FD = -1;

#define SW_CHAR_DEV "/dev/SW"
#define SW_BYTES 4					// number of bytes to read from SW device

#define BUF_SIZE 32					// reserve some buffer space
char SW_buffer[BUF_SIZE];			// buffer for SW data

/**
 * Opens the slide switch SW device. 
 * Return: 1 on success, else 0
 */
int SW_open( )
{
 	if ((SW_FD = open (SW_CHAR_DEV, O_RDWR)) < 0)
	{
		fprintf(stderr, "ERROR: Failed to open %s\n", SW_CHAR_DEV);
		return 0;
	}
	else
		return 1;
}

/**
 * Reads the slide switch SW device
 * Parameter data: pointer for returning data
 * Return: 1 on success, else 0
 */
int SW_read(int *data)
{
  	int bytes, bytes_read;
	bytes_read = 0;
	if (SW_FD > 0)
	{		 
		while ((bytes = read (SW_FD, SW_buffer, SW_BYTES)) > 0)
			bytes_read += bytes;	// read the port until EOF
		if (bytes_read != SW_BYTES)
		{
			fprintf (stderr, "Error: %d bytes expected from %s, but %d bytes read\n", SW_BYTES, 
				SW_CHAR_DEV, bytes_read);
			return 0;
		}
		if (sscanf (SW_buffer, "%X", data) != 1)
		{
			fprintf (stderr, "Can't parse data from %s: %s\n", SW_CHAR_DEV, SW_buffer);
			return 0;
		}
	}
	else
	{		  
		fprintf (stderr, "Can't read %s: SW_FD = %d\n", SW_CHAR_DEV, SW_FD);
		return 0;
	}
	return 1;
}

/**
 * Closes the SW device
 * Return: void
 */
void SW_close( )
{
	if (SW_FD > 0)
		close (SW_FD);
}

/* This file contains wrappers for the pushbutton KEY character device driver */
int KEY_FD = -1;

#define KEY_CHAR_DEV "/dev/KEY"
#define KEY_BYTES 2					// number of bytes to read from KEY device

#define BUF_SIZE 32					// reserve some buffer space
char KEY_buffer[BUF_SIZE];			// buffer for KEY data

/**
 * Opens the pushbutton KEY device
 * Return: 1 on success, else 0
 */
int KEY_open( )
{
 	if ((KEY_FD = open (KEY_CHAR_DEV, O_RDWR)) < 0)
	{
		fprintf(stderr, "ERROR: Failed to open %s\n", KEY_CHAR_DEV);
		return 0;
	}
	else
		return 1;
}

/**
 * Reads the pushbutton KEY device
 * Parameter data: pointer for returning data
 * Return: 1 on success, else 0
 */
int KEY_read(int *data)
{
	int bytes, bytes_read;
	bytes_read = 0;
	if (KEY_FD > 0)
	{
		while ((bytes = read (KEY_FD, KEY_buffer, KEY_BYTES)) > 0)
			bytes_read += bytes;	// read the port until EOF
		if (bytes_read != KEY_BYTES)
		{
			fprintf(stderr, "Error: %d bytes expected from %s, but %d bytes read\n", KEY_BYTES, 
				KEY_CHAR_DEV, bytes_read);
			return 0;
		}
		if (sscanf (KEY_buffer, "%X", data) != 1)
		{
			fprintf (stderr, "Can't parse data from %s: %s\n", KEY_CHAR_DEV, KEY_buffer);
			return 0;
		}
	}
	else
	{		  
		fprintf (stderr, "Can't read %s: KEY_FD = %d\n", KEY_CHAR_DEV, KEY_FD);
		return 0;
	}
	return 1;
}

/**
 * Closes the KEY device
 * Return: void
 */
void KEY_close( )
{
	if (KEY_FD > 0)
		close (KEY_FD);
}

/* This file contains wrappers for the VGA video character device driver */
int video_FD = -1;

#define video_CHAR_DEV "/dev/video"

#define VIDEO_BYTES 14				// number of bytes to read from video device

/* Declare buffers for reading/writing data to device driver */
#define BUF_SIZE 32							// reserve some buffer space
char video_buffer[BUF_SIZE];				// buffer for video data

/**
 * Opens the VGA video device. 
 * Return: 1 on success, else 0
 */
int video_open( )
{
 	if ((video_FD = open (video_CHAR_DEV, O_RDWR)) < 0)
	{
		fprintf(stderr, "ERROR: Failed to open %s\n", video_CHAR_DEV);
		return 0;
	}
	else
		return 1;
}

/**
 * Reads the VGA video device
 * Parameter columns: pointer for returning the number of columns in the display
 * Parameter rows: pointer for returning the number of rows in the display
 * Parameter tcolumns: pointer for returning the number of text columns
 * Parameter tcolumns: pointer for returning the number of text rows
 * Return: 1 on success, else 0
 */
int video_read(int *columns, int *rows, int *tcolumns, int *trows)
{
	int bytes, bytes_read;
	bytes_read = 0;
	if (video_FD > 0)
	{
		while ((bytes = read (video_FD, video_buffer, VIDEO_BYTES)) > 0)
			bytes_read += bytes;	// read the port until EOF
		if (bytes_read != VIDEO_BYTES)
		{
			fprintf (stderr, "Error: %d bytes expected from %s, but %d bytes read\n", VIDEO_BYTES, 
				video_CHAR_DEV, bytes_read);
			return 0;
		}
		if (sscanf (video_buffer, "%3d %3d %2d %2d", columns, rows, tcolumns, trows) != 4)
		{
			fprintf (stderr, "Can't parse data from %s: %s\n", video_CHAR_DEV, video_buffer);
			return 0;
		}
	}
	else
	{
		fprintf (stderr, "Can't read %s: video_FD = %d\n", video_CHAR_DEV, video_FD);
		return 0;
	}
	return 1;
}

/**
 * Clears all graphics on the VGA video device
 * Return: void
 */
void video_clear( )
{
	if (video_FD > 0)
		write (video_FD, "clear", 5);			// clear the video screen
	else
		fprintf (stderr, "Can't write %s: video_FD = %d\n", video_CHAR_DEV, video_FD);
}

/**
 * Performs a synchronization operation on the VGA video device (swaps front/back buffers)
 */
void video_show( )
{
	if (video_FD > 0)
		write (video_FD, "sync", 4);			// swap front/back buffers
	else
		fprintf (stderr, "Can't write %s: video_FD = %d\n", video_CHAR_DEV, video_FD);
}

/**
 * Colors a pixel on the VGA video device
 * Parameter x: the pixel column
 * Parameter y: the pixel row
 * Parameter color: the pixel color
 * Return: void
 */
void video_pixel(int x, int y, short color)
{
	if (video_FD > 0)
	{
		sprintf (video_buffer, "pixel %d,%d %X", x, y, color);
		write (video_FD, video_buffer, strlen(video_buffer));
	}
	else
		fprintf (stderr, "Can't write %s: video_FD = %d\n", video_CHAR_DEV, video_FD);
}

/**
 * Draws a line on the VGA video device
 * Parameter x1: the starting column
 * Parameter y1: the starting row
 * Parameter x2: the ending column
 * Parameter y2: the ending row
 * Parameter color: the pixel color
 * Return: void
 */
void video_line(int x1, int y1, int x2, int y2, short color)
{
	if (video_FD > 0)
	{
		sprintf (video_buffer, "line %d,%d %d,%d %X", x1, y1, x2, y2, color);
		write (video_FD, video_buffer, strlen(video_buffer));
	}
	else
		fprintf (stderr, "Can't write %s: video_FD = %d\n", video_CHAR_DEV, video_FD);
}

/**
 * Draws a filled box on the VGA video device
 * Parameter x1: the column for corner 1
 * Parameter y1: the row for corner 1
 * Parameter x2: the column for corner 2
 * Parameter y2: the row for corner 2
 * Parameter color: the pixel color
 * Return: void
 */
void video_box(int x1, int y1, int x2, int y2, short color)
{
	if (video_FD > 0)
	{
		sprintf (video_buffer, "box %d,%d %d,%d %X", x1, y1, x2, y2, color);
		write (video_FD, video_buffer, strlen(video_buffer));
	}
	else
		fprintf (stderr, "Can't write %s: video_FD = %d\n", video_CHAR_DEV, video_FD);
}

/**
 * Draws text on the VGA video device
 * Parameter x: the pixel column
 * Parameter y: the pixel row
 * Parameter msg: pointer to the text string
 * Return: void
 */
void video_text(int x, int y, char *msg)
{
	if (video_FD > 0)
	{
		sprintf (video_buffer, "text %d,%d %s", x, y, msg);
		write (video_FD, video_buffer, strlen(video_buffer));
	}
	else
		fprintf (stderr, "Can't write %s: video_FD = %d\n", video_CHAR_DEV, video_FD);
}

/**
 * Erases all text on the VGA video device
 * Return: void
 */
void video_erase( )
{
	if (video_FD > 0)
		write (video_FD, "erase", 5);			// erase all text
	else
		fprintf (stderr, "Can't write %s: video_FD = %d\n", video_CHAR_DEV, video_FD);
}

/**
 * Closes the VGA video device
 * Return: void
 */
void video_close ( )
{
	if (video_FD > 0)
		close (video_FD);
}

