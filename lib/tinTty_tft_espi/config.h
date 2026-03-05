#include <TFT_eSPI.h>
#include <Arduino.h>
#include <XPT2046_HR2046_touch.h>
#include <TFT_eSPI.h>// Hardware-specific library, configure at User_Setup.h

/**
 * marge = 1
TINTTY_CHAR_WIDTH (5+marge) 6
TINTTY_CHAR_HEIGHT (7+marge) 8
font2 = 8x16

FreeSans9pt7b	tft.setFreeFont(&FreeMono9pt7b);	
	X= 11Y=18
FONT2	tft.setTextFont(2);
	H= 16 W=6
GFXFF	tft.setTextFont(1);
	H=8 W=6

 * ST7735 rows 16
 * ST7735 Cols 26
 * ILI9488 rows 24
 * ILI9488 Cols 80
 */

#define errorLed 6 // <-- to giveErrorVisibility

// comportament teclat
#define snappyMillisLimit 100 // idle refresh time - reduced for faster response
#define beepTimeMillis 750
#define beepBlinkSpeedMillis 175
#define refreshMillisLimit 2000
#define keyboardAutoRepeatMillis 200
#define keyboardReleaseMillis 75 // anti bounce

//-----------touch
#define TOUCH_CS_PIN  13
#define TOUCH_IRQ 8
//

#define TOUCH_SENSIVITY 400
#define touchNoEspi// <------setCalibrationData

// varis
#define INPUT_BUFFER_SIZE 400
#define OUTPUT_BUFFER_SIZE 400

#define TFT_AMPLADA TFT_WIDTH
#define TFT_ALSSADA TFT_HEIGHT
#define RENDER_SKIP_CYCLES 3 // Skip render every N cycles when no changes

#define TTY_TEXT_SIZE 1
#define usingGFXfreefont
/*
#define cursorBlinkDelay 650
#define CURSOR_BLINK_SKIP 5	 // Check cursor blink less frequently
*/