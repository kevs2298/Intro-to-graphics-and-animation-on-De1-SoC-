#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "defines.h"

#define video_BYTES 8				// number of characters to read from /dev/video
#define SW_BYTES 3					// number of characters to read from /dev/SW
#define KEY_BYTES 1					// number of characters to read from /dev/KEY

int screen_x, screen_y;
int x_box[NUM_BOXES_MAX], y_box[NUM_BOXES_MAX];		// x, y coordinates of boxes to draw
int dx_box[NUM_BOXES_MAX], dy_box[NUM_BOXES_MAX];	// amount to move boxes in animation
unsigned int color[] = {WHITE, YELLOW, RED, GREEN, BLUE, CYAN, MAGENTA, GREY, PINK, ORANGE};
int color_box[NUM_BOXES_MAX];								// color
int shifted_dx_dy = 0;										// used to speed up/down animation
int boxes_to_draw = NUM_BOXES;							// # of boxes to draw
int no_lines = FALSE;										// draw lines between boxes?
struct timespec animation_time;							// used to control animation speed

void draw (int);

volatile sig_atomic_t stop;
void catchSIGINT(int signum){
    stop = 1;
}

/* This code uses the following character device drivers: /dev/video, /dev/KEY, /dev/SW. It 
 * creates an animation of moving boxes and lines. Pressing KEY0 makes the animation move
 * more quickly, and KEY1 slows it down. Pressing KEY2 deletes a box, and KEY3 adds one.
 * If all SW switches are down, lines are drawn between boxes, otherwise only boxes are drawn */
int main(int argc, char *argv[]){

	int i, video_FD, KEY_FD, SW_FD;					// file descriptors
	char video_buffer[video_BYTES];					// buffer for video char data
	char SW_buffer[SW_BYTES];							// buffer for SW char data
	char KEY_buffer[SW_BYTES];							// buffer for KEYSW char data
	int x1, x2, y1, y2, KEY_data;
	time_t t;	// used to seed the rand() function
	time_t start_time, elapsed_time;
	char command[256];
    
  	// catch SIGINT from ^C, instead of having it abruptly close this program
  	signal(SIGINT, catchSIGINT);
	start_time = time (NULL);
    
	srand ((unsigned) time(&t));						// seed the rand function
	// Open the character device drivers
  	if ((video_FD = open("/dev/video", O_RDWR)) == -1)
		printf("Error opening /dev/video: %s\n", strerror(errno));
	if ((SW_FD = open("/dev/SW", O_RDONLY)) == -1)
		printf("Error opening /dev/SW: %s\n", strerror(errno));
	if ((KEY_FD = open("/dev/KEY", O_RDONLY)) == -1)
		printf("Error opening /dev/KEY: %s\n", strerror(errno));
	if ((SW_FD == -1) || (KEY_FD == -1) || (video_FD == -1))
		return -1;
	
	while (read (video_FD, video_buffer, video_BYTES))
		;	// read the video port until EOF
	sscanf (video_buffer, "%3d %3d", &screen_x, &screen_y);

	// set random initial position and "delta" for all rectangle boxes
	for (i = 0; i < NUM_BOXES_MAX; i ++) 
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
	// initialize the animation speed
	animation_time.tv_sec = 0;
	animation_time.tv_nsec = 10000000;
  	while (!stop)
	{
		while (read (SW_FD, SW_buffer, SW_BYTES))
			;	// read the SW port until EOF
		sscanf (SW_buffer, "%X", &no_lines);
		
		while (read (KEY_FD, KEY_buffer, KEY_BYTES))
			;	// read the KEY port until EOF
		sscanf (KEY_buffer, "%X", &KEY_data);
		pushbutton (KEY_data);
		draw (video_FD);
	
		//========= ENEB454 - Add your code here ==========
		// sync with VGA display 
		sprintf (command, "sync\n");
		write (video_FD, command, strlen(command));

		nanosleep (&animation_time, NULL);
		elapsed_time = time (NULL) - start_time;
		if (elapsed_time > 120 || elapsed_time < 0) stop = 1;
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
		sprintf (command, "box %d,%d %d,%d %X\n",x_box[i],y_box[i],(x_box[i] + BOX_LEN), (y_box[i] + BOX_LEN), color_box[i]);	
		write (fd, command, strlen(command));

		if (!no_lines)
		{
			sprintf (command, "line %d,%d %d,%d %X\n", x_box[i], y_box[i], x_box[(i+1)%boxes_to_draw], y_box[(i+1)%boxes_to_draw],color_box[i]);
			write (fd, command, strlen(command));
		}
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

