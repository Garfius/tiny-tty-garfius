#include <Arduino.h>
#include "config.h"
#ifndef __tinttyUtils__
#define __tinttyUtils__


#define ERR_FAST_BLINK 50
#define ERR_SLOW_BLINK 300
//-------------coses baud menu
#define NUM_OPTIONS_BAUD_MENU 4
#define SQUARE_W_BAUD_MENU (TFT_AMPLADA - 60) // wide but with margin
#define SQUARE_H_BAUD_MENU 60				  // fixed height
#define GAP_Y_BAUD_MENU 20					  // space between squares

#define TOTAL_HEIGHT (NUM_OPTIONS_BAUD_MENU * SQUARE_H_BAUD_MENU + (NUM_OPTIONS_BAUD_MENU - 1) * GAP_Y_BAUD_MENU)

#define START_Y_BAUD_MENU ((TFT_ALSSADA - TOTAL_HEIGHT) / 2)
#define START_X_BAUD_MENU ((TFT_AMPLADA - SQUARE_W_BAUD_MENU) / 2)
#define DELAY_CONFIRM_BAUD_MENU 1500
//-------------altres
volatile static char myCharBuffer[INPUT_BUFFER_SIZE];	// whole ram must be buffer, lol
volatile static char myCharBuffer2[OUTPUT_BUFFER_SIZE]; // whole ram must be buffer, lol

class CharBuffer
{
public:
	volatile uint32_t head = 0;
	volatile uint32_t tail = 0;
	uint32_t localBufferSize;
	volatile char *myCharBuffer;

	CharBuffer(unsigned int size, volatile char *bufferArray);
	void addChar(char c);
	char consumeChar();

	// Inline helper functions for better performance
	inline bool isEmpty() const { return head == tail; }
	inline bool isFull() const { return ((tail + 1) % localBufferSize) == head; }
	inline uint32_t available() const { return (tail >= head) ? (tail - head) : (localBufferSize - head + tail); }
};
void tft_espi_calibrate_touch();
unsigned long chooseBauds();
void giveErrorVisibility(int slow, int fast, bool init=false);

#ifdef touchNoEspi
	void xpt2046CalibrateGet(uint16_t *parameters, uint32_t color_fg=TFT_WHITE, uint32_t color_bg=TFT_RED, uint8_t size=15);
	void convertRawXY(uint16_t *x, uint16_t *y);//convert raw touch x,y values to screen coordinates 
	bool getTouchRaw(uint16_t *x, uint16_t *y);
	bool getTouchDisplay(uint16_t *x, uint16_t *y);
	void setCalibrationData(uint16_t *parameters);
	void xpt2046CalibrateSet();
	bool isTouching();
#endif

extern CharBuffer buffer;
extern CharBuffer bufferoUT;
#ifdef touchNoEspi
    extern XPT2046_Touchscreen ts;
#endif
extern TFT_eSPI tft;
extern TFT_eSprite spr;
extern Stream *userTty;

#endif