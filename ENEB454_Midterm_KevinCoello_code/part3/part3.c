#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "defines.h"

#define video_BYTES 8				// number of characters to read from /dev/video

int screen_x, screen_y;

volatile sig_atomic_t stop;
void catchSIGINT(int signum){
    stop = 1;
}

/* This code uses the following character device driver: /dev/video. It draws a line on the 
 * screen, and moves the line up an down, giving the appearance of "bouncing" off the top and
 * bottom of the screen */
int main(int argc, char *argv[]){

	int i, video_FD;									// file descriptor
	char video_buffer[video_BYTES];				// buffer for video char data
	char command[256];
	int x1, x2, y, y_dir;
	time_t start_time, elapsed_time;
    
  	// catch SIGINT from ctrl+c, instead of having it abruptly close this program
  	signal(SIGINT, catchSIGINT);
	start_time = time (NULL);
    
	// Open the character device driver
  	if ((video_FD = open("/dev/video", O_RDWR)) == -1)
	{
		printf("Error opening /dev/video: %s\n", strerror(errno));
		return -1;
	}
	
	while (read (video_FD, video_buffer, video_BYTES))
		;	// read the video port until EOF
	sscanf (video_buffer, "%3d %3d", &screen_x, &screen_y);

	write (video_FD, "clear", 5);

	//========= ENEB454 - Add your code here ==========
	/* Set the initial location of the horizontal line */
	x1 = 40;// TODO
	x2 = 279;// TODO 
	y = 0;				// top of the screen
	y_dir = 1;			// y direction will move down

	sprintf (command, "line %d,%d %d,%d %X\n", x1, y, x2, y, ORANGE);
	write (video_FD, command, strlen(command));

  	while (!stop)
	{
		//========= ENEB454 - Add your code here ==========
		// 1. Erase old line
		sprintf (command, "clear\n");
		write (video_FD, command, strlen(command));
		
		// 2. Change line position.
		if (y == 240){
			y_dir = -1;
		} 
		else if (y == 0){
			y_dir = 1;
		}
		y += y_dir;

		
		// 3. Draw new line
		sprintf (command, "line %d,%d %d,%d %X\n", x1, y, x2, y, ORANGE);
		write (video_FD, command, strlen(command));
		
		// 4. sync with VGA display 
		sprintf (command, "sync\n");
		write (video_FD, command, strlen(command));

		elapsed_time = time (NULL) - start_time;
		if (elapsed_time > 12 || elapsed_time < 0) stop = 1;
	}
	printf ("\nExiting sample solution program\n");
	close (video_FD);
	return 0;
}
