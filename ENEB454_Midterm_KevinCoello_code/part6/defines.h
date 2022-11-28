void pushbutton (int);
#define		KEY0 						0b0001
#define		KEY1 						0b0010
#define		KEY2						0b0100
#define		KEY3						0b1000

#define		FASTER					1
#define		SLOWER					2

#define ABS(x)			(((x) > 0) ? (x) : -(x))

/* Constants for animation */
#define BOX_LEN						2
#define NUM_BOXES						15			// default number of boxes to draw
#define NUM_BOXES_MAX				25			// maximum number of boxes to draw
#define MIN_LOAD						1000000000 / 60	// minimum timer load value for animation
#define MAX_SHIFT						5			// used when speeding up the animation
#define FALSE							0
#define TRUE							1

#define WHITE 0xFFFF
#define YELLOW 0xFFE0
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x041F
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define GREY 0xC618
#define PINK 0xFC18
#define ORANGE 0xFC00
