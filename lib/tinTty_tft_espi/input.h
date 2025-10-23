#include "Arduino.h"

#define KEYBOARD_GUTTER 4
#define KEY_WIDTH (16+10)
#define KEY_HEIGHT (16+8)
#define KEY_GUTTER 1
#define KEYBOARD_HEIGHT ((7 * KEY_GUTTER)+(6*KEY_HEIGHT)+KEYBOARD_GUTTER)
#define KEY_ROW_A_Y (TFT_ALSSADA - KEYBOARD_HEIGHT + KEYBOARD_GUTTER)

void input_init();
void input_idle();
