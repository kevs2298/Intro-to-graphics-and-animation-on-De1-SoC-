#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "defines.h"
#include "wrappers.h"

int screen_x, screen_y;
int char_x, char_y;
int x_box[NUM_BOXES_MAX], y_box[NUM_BOXES_MAX];		// x, y coordinates of boxes to draw
int dx_box[NUM_BOXES_MAX], dy_box[NUM_BOXES_MAX];	// amount to move boxes in animation
unsigned int color[] = {WHITE, YELLOW, RED, GREEN, BLUE, CYAN, MAGENTA, GREY, PINK, ORANGE};
int color_box[NUM_BOXES_MAX];								// color
int shifted_dx_dy = 0;										// used to speed up/down animation
int boxes_to_draw = NUM_BOXES;							// # of boxes to draw
int no_lines = FALSE;										// draw lines between boxes?
int frames = 0;												// number of frames drawn
struct timespec animation_time;							// used to control animation speed

void draw ( );

volatile sig_atomic_t stop;
void catchSIGINT(int signum){
    stop = 1;
}

/* This code uses the following character device drivers: /dev/video, /dev/KEY, /dev/SW. It 
 * creates an animation of moving boxes and lines. Pressing KEY0 makes the animation move
 * more quickly, and KEY1 slows it down. Pressing KEY2 deletes a box, and KEY3 adds one.
 * If all SW switches are down, lines are drawn between boxes, otherwise only boxes are drawn.
 * A running count of the number of video frames drawn is displayed in the upper-right corner
 * of the screen. The program can be terminated by typing ^C. */
int main(int argc, char *argv[]){

	int i;
	char msg_buffer[80];
	int KEY_data;
	time_t t;	// used to seed the rand() function
	time_t start_time, elapsed_time;
    
  	// catch SIGINT from ^C, instead of having it abruptly close this program
  	signal(SIGINT, catchSIGINT);
	start_time = time (NULL);
    
	srand ((unsigned) time(&t));						// seed the rand function
	// Open the character device drivers
	if (!SW_open ( ) || !KEY_open ( ) || !video_open ( ))
		return -1;
	
	video_read (&screen_x, &screen_y, &char_x, &char_y);	// get VGA screen and text size

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
	// initialize the animation delay
	animation_time.tv_sec = 0;
	animation_time.tv_nsec = 0;
	video_erase ( );		// erase any text on the screen

  	while (!stop)
	{
		if (!SW_read (&no_lines) || !KEY_read (&KEY_data))
			stop = 1;
		pushbutton (KEY_data);
		draw ( );
	
		// sync with VGA display 
		video_show ( );
		++frames;
		sprintf (msg_buffer, "Frames rendered: %d\n", frames);
		video_text (0, 0, msg_buffer);
		nanosleep (&animation_time, NULL);
		elapsed_time = time (NULL) - start_time;
		if (elapsed_time > 30 || elapsed_time < 0) stop = 1;
	}
	video_close ( );
	KEY_close ( );
	SW_close ( );
	printf ("\nExiting sample solution program\n");
	return 0;
}

/* Code that draws the animation objects */
void draw( )
{
	int i; 

	/* Erase any boxes and lines that may have been previously drawn */
	video_clear ( );
	for (i = 0; i < boxes_to_draw; i++) 
	{
		//========= ENEB454 - Add your code here ==========
		// Draw new box
		video_box(x_box[i], y_box[i],(x_box[i] + BOX_LEN)%screen_x, (y_box[i] + BOX_LEN)%screen_y, color_box[i]);
	
		if (!no_lines)
		{
			video_line (x_box[i], y_box[i], x_box[(i+1)%boxes_to_draw],
				y_box[(i+1)%boxes_to_draw], color_box[i]);
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

