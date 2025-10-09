/**
 * ST7735 rows 16
 * ST7735 Cols 26
 * ILI9488 rows 24
 * ILI9488 Cols 80
 */
//----------de main.cpp

#define snappyMillisLimit 100 // idle refresh time - reduced for faster response
#define tintty_baud_rate 57600
#define refreshMillisLimit 2000
#define TOUCH_IRQ 8

#define TOUCH_SENSIVITY 600

// Performance optimization constants
#define RENDER_SKIP_CYCLES 3 // Skip render every N cycles when no changes
#define CURSOR_BLINK_SKIP 5	 // Check cursor blink less frequently

// de utils.h
#define INPUT_BUFFER_SIZE 400
#define OUTPUT_BUFFER_SIZE 400

// de input.h
#define errorLed 6 // <-- to giveErrorVisibility at input.cpp

#define keyboardAutoRepeatMillis 250
#define keyboardReleaseMillis 75 // anti bounce

#define TFT_AMPLADA TFT_WIDTH
#define TFT_ALSSADA TFT_HEIGHT

// de input.cpp

// de tintty.h

// de tintty.cpp
#define cursorBlinkDelay 650
