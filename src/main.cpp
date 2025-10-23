#include "Free_Fonts.h" // Include the header file attached to this sketch from TFT_eSPI
#include <Arduino.h>
#include <EEPROM.h>
#include <SPI.h>
#include "tintty.h"
#include "utils.h"
#include "input.h"
/**
 * canvi de prova per el git
 * TinTTY main sketch
 * by Nick Matantsev 2017 & Gerard Forcada 2024
 *
 * Original reference: VT100 emulation code written by Martin K. Schroeder
 * and modified by Peter Scargill.
 *
 * to-do:
 *  Test on the original ILI9341
 *  Port back to AVR if possible
 *  Scroll back
 *  Improve multi-core, use myCheesyFB.outputting so refresh while receiving
 *
 * PINOUT:
 *	T_IRQ				8 (config.h)TOUCH_IRQ
 *	T_OUT	MISO1	_RX	12
 *	T_DIN	MOSI1	_TX	15
 *	T_CS	CS1			13 ¿TOUCH_CS_PIN 7?
 *	T_CLK	SCK1	CLK	14
 *	SDO		MISO0		16 ¡NO TRISTATE!
 *	LED					3.3V || 5V || PWM
 *	SCK		CLK0		18
 *	SDI		MOSI0		19
 *	D/C					2 (TFT_eSPI\User_Setup.h)TFT_DC
 *	RESET				3 (TFT_eSPI\User_Setup.h)TFT_RST
 *	CS		CS0			15 (TFT_eSPI\User_Setup.h)TFT_CS
 *	GND		GND 0V
 *	VCC		5V
 */

volatile bool running = false;

tintty_display ili9341_display = {						  // from serial to display from ~236 tintty_idle(&ili9341_display)
	TFT_AMPLADA,										  // x
	(TFT_ALSSADA - KEYBOARD_HEIGHT),					  // y
	TFT_AMPLADA / TINTTY_CHAR_WIDTH,					  // colCount
	(TFT_ALSSADA - KEYBOARD_HEIGHT) / TINTTY_CHAR_HEIGHT, // rowCount

	[](int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) { // fill_rect, cursor
		assureRefreshArea(x, y, w, h);
		spr.fillRect(x, y, w, h, color);
	},

	[](int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *pixels) { // draw_pixels, no es fa servir
		assureRefreshArea(x, y, w, h);
		spr.setAddrWindow(x, y, x + w - 1, y + h - 1);
		spr.pushColors(pixels, w * h, 1);
	},

	[](int16_t offset) { // set_vscroll

	}};
// buffer to test various input sequences
// passa el tros indicat a;
// unsigned static int lastRefresh;
void refreshDisplayIfNeeded()
{
	static uint32_t last_check = 0;
	uint32_t current_time = millis();

	// Skip frequent checks - only check every few milliseconds
	if (current_time - last_check < 10)
	{
		return;
	}
	last_check = current_time;

	if (myCheesyFB.hasChanges)
	{
		if (current_time > (myCheesyFB.lastRemoteDataTime + snappyMillisLimit))
		{
			// Mark as outputting to prevent conflicts
			myCheesyFB.outputting = true;

			// Only push the changed region for better performance
			uint16_t width = myCheesyFB.maxX - myCheesyFB.minX;
			uint16_t height = myCheesyFB.maxY - myCheesyFB.minY;

			if (width > 0 && height > 0)
			{
				spr.pushSprite(myCheesyFB.minX, myCheesyFB.minY,
							   myCheesyFB.minX, myCheesyFB.minY, width, height);
			}

			myCheesyFB.outputting = false;
			myCheesyFB.hasChanges = false;
			myCheesyFB = fameBufferControl{UINT16_MAX, 0, UINT16_MAX, 0, false, false, 0};
		}
	}
	else
	{
		input_idle(); // aqui colisiona mutex
	}
}
void parseToBuffer()
{

	tintty_run( // serial to Screen
		[]()
		{
			// peek idle loop, non blocking?
			while (true)
			{
				// bufferAtoB();

				// if (userTty->available() > 0)             return (char)userTty->peek(); // Safe to read

				if (buffer.head != buffer.tail)
					return myCharBuffer[buffer.head];
				tintty_idle(&ili9341_display); // render if needed

				// input_idle();// aqui colisiona mutex
			}
		},
		[]()
		{
			while (true)
			{ // read char
				// bufferAtoB();
				tintty_idle(&ili9341_display);
				// if (userTty->available() > 0) return (char)userTty->read(); // Safe to read
				if (buffer.head != buffer.tail)
					return buffer.consumeChar();

				// input_idle();// aqui colisiona mutex
			}
		}, // send char
		[](char ch)
		{
			bufferoUT.addChar(ch);
			// if (userTty->available() == 0) userTty->write(ch);
		},
		&ili9341_display);
}
void loop1()
{
	refreshDisplayIfNeeded();
	// Reduced delay for faster response - use yield instead of delay when possible
	yield();
}
/*TaskHandle_t xHandle;
TaskHandle_t xHandle1;*/
void loop()
{
	/*xTaskCreate(vTaskReadSerial, "readSerial", 512, NULL, 1,  NULL );
	xTaskCreate(vTaskParseToBuffer, "parseToBuffer", 512, NULL, 1,  NULL );
	xTaskCreate(vTaskReadSerial, "readSerial", 512, NULL, 1,  &( xHandle ) );
	xTaskCreate(vTaskParseToBuffer, "parseToBuffer", 512, NULL, 1,  &( xHandle1 ) );
	vTaskCoreAffinitySet( xHandle, 1 << 0 );
	vTaskCoreAffinitySet( xHandle1, 1 << 0 );
	vTaskStartScheduler();
	*/
}

void setup1()
{
	while (!running)
	{
	}
}

void setup()
{
	//-----------------------setup
	pinMode(23, OUTPUT); // millora 3.3v GPIO23 controls the RT6150 PS (Power Save) pin. When PS is low (the default on Pico)
	digitalWrite(23, HIGH);

	// Optimize Serial FIFO for better performance
	Serial1.setFIFOSize(512); // Increased buffer size

	tft.begin();
	tft.setFreeFont(GLCD);
	tft.setTextSize(1);
	tft.setRotation(2);
	gpio_pull_up(2); // ensure pull-up for receiving wire

	userTty = &Serial1; // assign receiving serial port

	//-----------------------init
	giveErrorVisibility(1, 1, true);

	spr.setColorDepth(8);

	if (spr.createSprite(TFT_AMPLADA, (TFT_ALSSADA - KEYBOARD_HEIGHT)) == nullptr)
		giveErrorVisibility(1, 2);

	spr.setTextSize(1);
	spr.fillSprite(TFT_BLACK);
	#ifdef touchNoEspi
		ts.begin(tft.getRotation());
		calibrator cal = calibrator();
		cal.xpt2046CalibrateSet(30,30);
	#else
		tft_espi_calibrate_touch();
	#endif

	Serial1.begin(chooseBauds(), SERIAL_8N1);

	input_init();


	//---------------go!
	running = true;

	parseToBuffer();
}
