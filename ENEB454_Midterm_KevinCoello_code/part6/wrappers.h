#ifndef SW_H
#define SW_H

/**
 * Function SW_open: opens the slide switch SW device
 * Return: 1 on success, else 0
 */
int SW_open (void);

/**
 * Function SW_read: reads the SW device
 * Parameter data: pointer for returning data. If no switches are set *data = 0.
 * 	If all switches are set *data = 0b1111111111
 * Return: 1 on success, else 0
 */
int SW_read (int * /*data*/);

/**
 * Function SW_close: closes the SW device
 * Return: void
 */
void SW_close (void);

#endif

#ifndef KEY_H
#define KEY_H

/**
 * Function KEY_open: opens the pushbutton KEY device
 * Return: 1 on success, else 0
 */
int KEY_open (void);

/**
 * Function KEY_read: reads the pushbutton KEY device
 * Parameter data: pointer for returning data. If no KEYs are pressed *data = 0. 
 * 	If all KEYs are pressed *data = 0b1111
 * Return: 1 on success, else 0
 */
int KEY_read (int * /*data*/);

/**
 * Function KEY_close: closes the KEY device
 * Return: void
 */
void KEY_close (void);

#endif

#ifndef video_H
#define video_H
/**
 * Function video_open: opens the VGA video device
 * Return: 1 on success, else 0
 */
int video_open (void);

/**
 * Function video_read: reads from the video device
 * Parameter cols: pointer for returning the number of columns in the display
 * Parameter rows: pointer for returning the number of rows in the display
 * Parameter tcols: pointer for returning the number of text columns
 * Parameter trows: pointer for returning the number of text rows
 * Return: 1 on success, else 0
 */
int video_read (int * /*cols*/, int * /*rows*/, int * /*tcols*/, int * /*trows*/);

/**
 * Function video_clear: clears all graphics from the video display
 * Return: void
 */
void video_clear (void);

/**
 * Function video_show: swaps the video front and back buffers
 * Return: void
 */
void video_show (void);

/**
 * Function video_pixel: sets pixel at (x, y) to color
 * Parameter x: the pixel column
 * Parameter y: the pixel row
 * Parameter color: the pixel color
 * Return: void
 */
void video_pixel (int /*x*/, int /*y*/, short /*color*/);

/**
 * Function video_line: draws a line
 * Parameter x1: the starting column
 * Parameter y1: the starting row
 * Parameter x2: the ending column
 * Parameter y2: the ending row
 * Parameter color: the line color
 * Return: void
 */
void video_line (int /*x1*/, int /*y1*/, int /*x2*/, int /*y2*/, short /*color*/);

/**
 * Function video_box: draws a filled box 
 * Parameter x1: the column for corner 1
 * Parameter y1: the row for corner 1
 * Parameter x2: the column for corner 2
 * Parameter y2: the row for corner 2
 * Parameter color: the box color
 * Return: void
 */
void video_box (int /*x1*/, int /*y1*/, int /*x2*/, int /*y2*/, short /*color*/);

/**
 * Function video_text: puts text on the video device
 * Parameter x: the pixel column
 * Parameter y: the pixel row
 * Parameter msg: pointer to the text string
 * Return: void
 */
void video_text (int /*x*/, int /*y*/, char * /*string*/);

/**
 * Function video_erase: erases all text on the video device
 * Return: void
 */
void video_erase (void);

/**
 * Function video_close: closes the video device
 * Return: void
 */
void video_close (void);

// Define some colors for video graphics
#define video_WHITE		0xFFFF
#define video_YELLOW 	0xFFE0
#define video_RED			0xF800
#define video_GREEN		0x07E0
#define video_BLUE		0x041F
#define video_CYAN		0x07FF
#define video_MAGENTA	0xF81F
#define video_GREY		0xC618
#define video_PINK		0xFC18
#define video_ORANGE		0xFC00

#endif

