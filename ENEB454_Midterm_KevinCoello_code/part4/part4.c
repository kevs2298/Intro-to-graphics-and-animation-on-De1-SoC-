#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "defines.h"

#define video_BYTES 8				// number of characters to read from /dev/video

int screen_x, screen_y;
int x_box[NUM_BOXES], y_box[NUM_BOXES];		// x, y coordinates of boxes to draw
int dx_box[NUM_BOXES], dy_box[NUM_BOXES];		// amount to move boxes in animation
unsigned int color[] = {WHITE, YELLOW, RED, GREEN, BLUE, CYAN, MAGENTA, GREY, PINK, ORANGE};
int color_box[NUM_BOXES];							// color
int boxes_to_draw = NUM_BOXES;					// # of boxes to draw

void draw (int);

volatile sig_atomic_t stop;
void catchSIGINT(int signum){
    stop = 1;
}

/* This code uses the following character device driver: /dev/video. It creates an animation of 
 * moving boxes and lines. */
int main(int argc, char *argv[]){

	int i, video_FD;								// file descriptor
	char video_buffer[video_BYTES];			// buffer for video char data
	int x1, x2, y1, y2;
	time_t t;	// used to seed the rand() function
	time_t start_time, elapsed_time;
	char command[256];
    
  	// catch SIGINT from ^C, instead of having it abruptly close this program
  	signal(SIGINT, catchSIGINT);
	start_time = time (NULL);
    
	srand ((unsigned) time(&t));						// seed the rand function
	// Open the character device drivers
  	if ((video_FD = open("/dev/video", O_RDWR)) == -1)
	{
		printf("Error opening /dev/video: %s\n", strerror(errno));
		return -1;
	}
	
	while (read (video_FD, video_buffer, video_BYTES))
		;	// read the video port until EOF
	sscanf (video_buffer, "%3d %3d", &screen_x, &screen_y);

	// set random initial position and "delta" for all rectangle boxes
	for (i = 0; i < NUM_BOXES; i ++) 
	{
		//========= ENEB454 - Add your code here ==========
			// random x position
			x_box[i] = (rand()%screen_x);
			// random y position
			y_box[i] = (rand()%screen_y);

		if (rand()%2)															// two slightly different animations
		{
			dx_box[i] = ((rand()%2)*2) - 1;								// random value of 1 or -1
			dy_box[i] = ((rand()%2)*2) - 1;								// random value of 1 or -1
		}
		else
		{
			dx_box[i] = 1;														// same dx, dy (looks "different")
			dy_box[i] = 1;
		}
		color_box[i] = color[(rand()%10)];								// random out of 10 colors
	}
  	while (!stop)
	{
		//========= ENEB454 - Add your code here ==========
		// 1. Draw
		draw(video_FD);
	
		// 2. Sync with VGA display 
		sprintf (command, "sync\n");
		write (video_FD, command, strlen(command));

		elapsed_time = time (NULL) - start_time;
		if (elapsed_time > 12 || elapsed_time < 0) stop = 1;
	}
	close (video_FD);
	printf ("\nExiting sample solution program\n");
	return 0;
}

/* Code that draws the animation objects */
void draw(int fd)
{
	int i; 
	char command[256];

	/* Erase any boxes and lines that may have been previously drawn */
	write (fd, "clear", 5);					// clear the video screen
	for (i = 0; i < boxes_to_draw; i++) 
	{
		//========= ENEB454 - Add your code here ==========
		// Draw new box
		sprintf (command, "box %d,%d %d,%d %X\n",x_box[i],y_box[i],(x_box[i] + BOX_LEN)%screen_x, (y_box[i] + BOX_LEN)%screen_y, color_box[i]);
		write (fd, command, strlen(command));

		//========= ENEB454 - Add your code here ==========
		// Draw the lines
		sprintf (command, "line %d,%d %d,%d %X\n", x_box[i], y_box[i], x_box[(i+1)%boxes_to_draw], y_box[(i+1)%boxes_to_draw],color_box[i]);
		write (fd, command, strlen(command));
	}
	for (i = 0; i < boxes_to_draw; i++)
	{
		/* Change box position */
		x_box[i] += dx_box[i];
		y_box[i] += dy_box[i];

		if ((x_box[i] + BOX_LEN >= screen_x) || (x_box[i] <= 0))
		{
			dx_box[i] = -dx_box[i];
			x_box[i] += dx_box[i];
		}
		if ((y_box[i] + BOX_LEN >= screen_y) || (y_box[i] <= 0))
		{
			dy_box[i] = -dy_box[i];
			y_box[i] += dy_box[i];
		}
	}
}

