#include <TFT_eSPI.h>
#include <Arduino.h>
#include <XPT2046_Touchscreen.h>
#include <TFT_eSPI.h>// Hardware-specific library, configure at User_Setup.h

/**
 * ST7735 rows 16
 * ST7735 Cols 26
 * ILI9488 rows 24
 * ILI9488 Cols 80
 */

#define errorLed 6 // <-- to giveErrorVisibility

// comportament teclat
#define snappyMillisLimit 100 // idle refresh time - reduced for faster response

#define refreshMillisLimit 2000
#define keyboardAutoRepeatMillis 250
#define keyboardReleaseMillis 75 // anti bounce

//-----------touch
#define TOUCH_CS_PIN  7
#define TOUCH_IRQ 8
//

#define TOUCH_SENSIVITY 600
#define touchNoEspi// <------setCalibrationData

// varis
#define INPUT_BUFFER_SIZE 400
#define OUTPUT_BUFFER_SIZE 400

#define TFT_AMPLADA TFT_WIDTH
#define TFT_ALSSADA TFT_HEIGHT
#define RENDER_SKIP_CYCLES 3 // Skip render every N cycles when no changes

// deprecatred
/*
#define cursorBlinkDelay 650
#define CURSOR_BLINK_SKIP 5	 // Check cursor blink less frequently
*/