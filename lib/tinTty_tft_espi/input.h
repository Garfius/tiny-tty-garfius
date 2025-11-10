#include "Arduino.h"

#define KEYBOARD_ROWS 5
#define KEYBOARD_GUTTER 4
#define KEY_WIDTH (16+7)
//#define KEY_HEIGHT (16+9)
#define KEY_HEIGHT (22+9)
#define KEY_GUTTER 1
#define KEYBOARD_HEIGHT (((KEYBOARD_ROWS-1) * KEY_GUTTER)+(KEYBOARD_ROWS*KEY_HEIGHT)+KEYBOARD_GUTTER)
#define KEY_ROW_A_Y (TFT_ALSSADA - KEYBOARD_HEIGHT + KEYBOARD_GUTTER)//480-(6 * 1)+(5*(16+3))+4 +4

void input_init();
void input_idle();
