#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "defines.h"

void reconfig_animation(int);

extern int boxes_to_draw;
extern volatile int dx_box[], dy_box[];		// amount to move boxes in animation
extern volatile int shifted_dx_dy;				// used to speed up/slow down animation
extern struct timespec animation_time;			// used to control animation speed

/****************************************************************************************
 * This routine checks which KEY has been pressed. It performs:
 * 	KEY0: speeds up the animation
 *		KEY1: slows down the animation
 *		KEY2: decreases # of boxes, lines drawn
 *		KEY3: increases # of boxes, lines drawn
 ***************************************************************************************/
void pushbutton(int press)
{
	if (press & KEY0)
		reconfig_animation (FASTER);
	else if (press & KEY1)
		reconfig_animation (SLOWER);
	else if (press & KEY2)
	{
		--boxes_to_draw;
		if (boxes_to_draw < 1)
			boxes_to_draw = 1;
	}
	else if (press & KEY3)
	{
		//========= ENEB454 - Add your code here ==========
		// Increment the number of boxes to draw and check if the amount exceeds the maximum 
		++boxes_to_draw;
		if (boxes_to_draw > NUM_BOXES_MAX)
			boxes_to_draw = NUM_BOXES_MAX;
	}
	return;
}

/* Change the speed of the animation */
void reconfig_animation(int action)
{
	int i, counter;

	counter = animation_time.tv_nsec;	// get current animation speed
	if (action == FASTER)
	{
		counter -= MIN_LOAD;	// subtract 1/60 seconds
		if (counter < 0)
		{
			counter = 0;
			if (shifted_dx_dy < MAX_SHIFT)	// limit the speed up
			{
				++shifted_dx_dy;
				for (i = 0; i < NUM_BOXES_MAX; i++) 
				{
					//========= ENEB454 - Add your code here ==========
					dx_box[i] = dx_box[i]<<1;
					dy_box[i] = dy_box[i]<<1;

				}
			}
		}
	}
	else
	{
		if (shifted_dx_dy)
		{
			counter = 0;
			--shifted_dx_dy;
			for (i = 0; i < NUM_BOXES_MAX; i++) 
			{
				//========= ENEB454 - Add your code here ==========
				dx_box[i] = dx_box[i]>>1;
				dy_box[i] = dy_box[i]>>1;
			}
		}
		else
			counter += MIN_LOAD;	// add 1/60 seconds
	}
	animation_time.tv_nsec = counter;	// update the animation speed
}
