#include <EEPROM.h>
#include <utils.h>

#ifndef touchNoEspi
/**
 * NOT optimized AT ALL! just debugged
 */
void tft_espi_calibrate_touch()
{
	EEPROM.begin(255);

	uint16_t calibrationData[7]; // els 2 extra son per fer el check de si es vol fer a mà
	uint8_t *calibrationDataBytePoint = (uint8_t *)calibrationData;
	uint8_t calDataOK = 0;
	uint8_t calDataTst = 0;
	//-------------------eeprom integrity check
	calDataOK = EEPROM.read(0);
	calDataTst = ~EEPROM.read(1);
	calDataOK = calDataOK == calDataTst;
	int eePos = 2;
	if (calDataOK && (!tft.getTouch(&calibrationData[5], &calibrationData[6], TOUCH_SENSIVITY)))
	{
		// calibration data valid & NO TSOLICITED
		for (int i = 0; i < 10; i++)
		{
			calDataTst = EEPROM.read(eePos + i);
			calibrationDataBytePoint[i] = calDataTst;
			eePos++;
		}
		tft.setTouch(calibrationData);
	}
	else
	{
		if (tft.getTouch(&calibrationData[5], &calibrationData[6], TOUCH_SENSIVITY))
		{
			tft.fillScreen(TFT_YELLOW);
			delay(1000);
			tft.fillScreen(TFT_BLACK);
		}
		// data not valid. recalibrate
		tft.calibrateTouch(calibrationData, TFT_WHITE, TFT_RED, 15);
		// store data
		while (calDataOK == EEPROM.read(0))
			calDataOK = (uint8_t)random(0, 255); // firma
		EEPROM.write(0, calDataOK);
		calDataTst = ~calDataOK;
		EEPROM.write(1, calDataTst);
		int eePos = 2;
		for (int i = 0; i < 10; i++)
		{
			calDataTst = calibrationDataBytePoint[i];
			EEPROM.write(eePos + i, calDataTst);
			eePos++;
		}
		EEPROM.commit();
		EEPROM.end();
	}
	tft.fillScreen(TFT_BLACK);
}
#else

	bool calibrator::calibrateTouch(uint16_t borderX, uint16_t borderY, bool calibOk)
	{
		uint32_t color_fg = TFT_RED;
		uint32_t color_bg = TFT_GREEN;
		uint8_t size = 4;
		uint16_t x_tmp, y_tmp;
		Point_XPT2046_HR2046_touch dst[4];
		Point_XPT2046_HR2046_touch src[4];
		// Fill src[4] with screen bordered values, corresponds to the linear plane of the display
		// Order: [0]=top-left, [1]=top-right, [2]=bottom-left, [3]=bottom-right
		src[0].x = borderX;					// top-left x
		src[0].y = borderY;					// top-left y
		src[1].x = TFT_WIDTH - borderX - 1;	// top-right x
		src[1].y = borderY;					// top-right y
		src[2].x = borderX;					// bottom-left x
		src[2].y = TFT_HEIGHT - borderY - 1; // bottom-left y
		src[3].x = TFT_WIDTH - borderX - 1;	// bottom-right x
		src[3].y = TFT_HEIGHT - borderY - 1; // bottom-right y
		// Fill dst with zeroes
		for (uint8_t i = 0; i < 4; i++)
		{
			dst[i].x = 0.0;
			dst[i].y = 0.0;
		}
		
		// Draw cancel button in center of screen
		const char* cancelText = "CANCEL";
		tft.setTextSize(2);
		int16_t cancelWidth = tft.textWidth(cancelText);
		int16_t cancelHeight = tft.fontHeight();
		int16_t padding = 10;
		int16_t boxWidth = cancelWidth + (padding * 2);
		int16_t boxHeight = cancelHeight + (padding * 2);
		int16_t boxX = (TFT_WIDTH/2) - (boxWidth/2);
		int16_t boxY = (TFT_HEIGHT/2) - (boxHeight/2);
		
		tft.drawRect(boxX, boxY, boxWidth, boxHeight, TFT_WHITE);
		tft.setTextColor(TFT_WHITE, color_bg);
		#ifdef usingGFXfreefont
		tft.setCursor(boxX + padding, (boxY + padding)+cancelHeight);
		#else
		tft.setCursor(boxX + padding, boxY + padding);
		#endif
		tft.print(cancelText);

		// corner positions adjusted by borderX/borderY and size
		for (uint8_t i = 0; i < 4; i++)
		{
			// draw clear the 4 calibration boxes (background)
			tft.fillCircle(borderX, borderY, size + 2, color_bg);
			tft.fillCircle(TFT_WIDTH - borderX - (int)(size/2), borderY, size + 2, color_bg);
			tft.fillCircle(borderX, TFT_HEIGHT - borderY - (int)(size/2), size + 2, color_bg);
			tft.fillCircle(TFT_WIDTH - borderX - (int)(size/2), TFT_HEIGHT - borderY - (int)(size/2), size + 2, color_bg);

			// draw target in the corner depending on i
			switch (i)
			{
			case 0: // top-left
				tft.fillCircle(borderX, borderY, size, color_fg);
				break;
			case 1: // top-right
				tft.fillCircle(TFT_WIDTH - borderX, borderY, size, color_fg);
				break;
			case 2: // bottom-left
				tft.fillCircle(borderX, TFT_HEIGHT - borderY , size, color_fg);
				break;
			case 3: // bottom-right
				tft.fillCircle(TFT_WIDTH - borderX , TFT_HEIGHT - borderY , size, color_fg);
				break;
			}

			tft.fillCircle(TFT_WIDTH/2 , (TFT_HEIGHT /2) -(boxHeight+(size*2)), size*2, TFT_RED);
			// user has to get the chance to release
			if (i > 0)
				delay(2000);
			tft.fillCircle(TFT_WIDTH/2 , (TFT_HEIGHT /2) -(boxHeight+(size*2)) , size*2, TFT_BLACK);

			// sample 8 times and average for stability
			for (uint8_t j = 0; j < 8; j++)
			{
				// Read touch coordinates
				while (!ts.validTouch(&x_tmp, &y_tmp)){};
					dst[i].x += (float)x_tmp;
					dst[i].y += (float)y_tmp;
			}
			dst[i].x = dst[i].x / 8;
			dst[i].y = dst[i].y / 8;
			if(calibOk && ts.mapPoint(dst[i].x, dst[i].y, mtpp)){
				if (mtpp.x >= boxX && mtpp.x <= (boxX + boxWidth) &&
					mtpp.y >= boxY && mtpp.y <= (boxY + boxHeight))
				{
					tft.fillRect(boxX, boxY, boxWidth, boxHeight, TFT_WHITE);
					delay(500);
					tft.fillRect(boxX, boxY, boxWidth, boxHeight, TFT_RED);
					delay(500);
					tft.fillRect(boxX, boxY, boxWidth, boxHeight, TFT_WHITE);
					return false; // Calibration cancelled
				}
			}
			

		}
		for (uint8_t i = 0; i < 4; i++)
		{
			ts.dst[i] = dst[i];
			ts.src[i] = src[i];
		}

		return true;
	}
	// Store ts.src and ts.dst arrays to EEPROM
	void calibrator::storeCalibrationPoints()
	{
		EEPROM.begin(_eepromSizeInit);
		int eePos = _eepromPos; // Start after the main calibration data (14 bytes + 2 integrity bytes)

		// Store ts.src[4] points (4 points * 2 floats * 4 bytes = 32 bytes)
		uint8_t *srcBytePtr = (uint8_t *)ts.src;
		for (int i = 0; i < 32; i++)
		{
			EEPROM.write(eePos + i, srcBytePtr[i]);
		}
		eePos += 32;

		// Store ts.dst[4] points (4 points * 2 floats * 4 bytes = 32 bytes)
		uint8_t *dstBytePtr = (uint8_t *)ts.dst;
		for (int i = 0; i < 32; i++)
		{
			EEPROM.write(eePos + i, dstBytePtr[i]);
		}

		// Write integrity marker for calibration points
		EEPROM.write(80, 0xAA); // Marker at position 80
		EEPROM.write(81, 0x55); // Complement marker

		EEPROM.commit();
		EEPROM.end();
	}

	// Retrieve ts.src and ts.dst arrays from EEPROM
	bool calibrator::retrieveCalibrationPoints()
	{
		EEPROM.begin(_eepromSizeInit);

		// Check integrity markers
		if (EEPROM.read(80) != 0xAA || EEPROM.read(81) != 0x55)
		{
			EEPROM.end();
			return false; // No valid calibration points stored
		}

		int eePos = _eepromPos; // Start after the main calibration data

		// Retrieve ts.src[4] points
		uint8_t *srcBytePtr = (uint8_t *)ts.src;
		for (int i = 0; i < 32; i++)
		{
			srcBytePtr[i] = EEPROM.read(eePos + i);
		}
		eePos += 32;

		// Retrieve ts.dst[4] points
		uint8_t *dstBytePtr = (uint8_t *)ts.dst;
		for (int i = 0; i < 32; i++)
		{
			dstBytePtr[i] = EEPROM.read(eePos + i);
		}
		for(int i=0;i<4;i++){
			if((ts.dst->x ==0) || (ts.dst->y ==0)){
				EEPROM.write(80,0);
				EEPROM.commit();
				EEPROM.end();
				return false;
			}
		}

		EEPROM.end();
		return true; // Successfully retrieved calibration points
	}
	
	calibrator::calibrator(size_t eepromSizeInit,int eepromPos){
		_eepromSizeInit = eepromSizeInit;
		_eepromPos = eepromPos;
	}
	
	void calibrator::xpt2046CalibrateSet(uint16_t borderX, uint16_t borderY,tintty_display *display)
	{
		bool calibOk = retrieveCalibrationPoints();
		if (ts.isTouching() || (!calibOk))
		{
			tft.fillScreen(TFT_YELLOW);
			tft.setTextColor(TFT_BLACK, TFT_YELLOW);
			tft.setTextSize(2);
			char cpr_response[32];
			snprintf(cpr_response, sizeof(cpr_response), "Rows:%d Cols:%d", display->screen_col_count,display->screen_row_count);
			tft.setCursor((TFT_AMPLADA - tft.textWidth(cpr_response))/2, (TFT_ALSSADA/2)-tft.fontHeight());
			tft.print(cpr_response);//TFT_AMPLADA TFT_Alssada?
			snprintf(cpr_response, sizeof(cpr_response), "Char W:%d Char H:%d", CHAR_WIDTH,CHAR_HEIGHT);
			tft.setCursor((TFT_AMPLADA - tft.textWidth(cpr_response))/2, (TFT_ALSSADA/2));
			tft.print(cpr_response);
			delay(2000);
			tft.fillScreen(TFT_BLACK);
			// data not valid. recalibrate. Pass borderX/borderY into calibrateTouch
			uint8_t rot =tft.getRotation();
			tft.setRotation(0);
			if(calibrateTouch(borderX, borderY,calibOk))storeCalibrationPoints();
			tft.setRotation(rot);
			tft.fillScreen(TFT_BLACK);
		}
	}

#endif
// Add a character to the buffer - optimized version
CharBuffer::CharBuffer(unsigned int size, volatile char *bufferArray)
{
	myCharBuffer = bufferArray;
	localBufferSize = size;
}

void CharBuffer::addChar(char c)
{
	uint32_t next_tail = (tail + 1) % localBufferSize;
	if (next_tail == head)
	{ // Buffer is full
		giveErrorVisibility(2,1);
		return;
	}
	myCharBuffer[tail] = c;
	tail = next_tail;
}

// Consume a character from the buffer - optimized version
char CharBuffer::consumeChar()
{
	if (head == tail)
	{ // Buffer is empty
		giveErrorVisibility(2,2);
		return 0;
	}
	char c = myCharBuffer[head];
	head = (head + 1) % localBufferSize;
	return c;
}

const unsigned long optionValues[NUM_OPTIONS_BAUD_MENU] = {9600, 19200, 57600, 115200};
const char *optionLabels[NUM_OPTIONS_BAUD_MENU] = {"9600", "19200", "57600", "115200"};

// ==== Function to draw big named squares ====
unsigned long chooseBauds()
{
	tft.fillScreen(TFT_BLACK);
	tft.setTextSize(2);
	#ifdef usingGFXfreefont
	tft.setCursor(0, tft.fontHeight()+2);
	#else
	tft.setCursor(0, 0);
	#endif
	tft.println("Escull Velocitat");
	for (int i = 0; i < NUM_OPTIONS_BAUD_MENU; i++)
	{
		int x = START_X_BAUD_MENU;
		int y = START_Y_BAUD_MENU + i * (SQUARE_H_BAUD_MENU + GAP_Y_BAUD_MENU);

		// Draw outlined square (rectangle)
		tft.drawRect(x, y, SQUARE_W_BAUD_MENU, SQUARE_H_BAUD_MENU, TFT_WHITE);

		// Center text
		tft.setTextColor(TFT_WHITE, TFT_BLACK);
		tft.setCursor(x + (SQUARE_W_BAUD_MENU) / 2, y + (SQUARE_H_BAUD_MENU) / 2);
		tft.println(optionLabels[i]);
	}

	bool escullBe = false;
	uint16_t xpos, ypos;
	unsigned long tmp;
	while (!escullBe)
	{
		while (digitalRead(TOUCH_IRQ));
		#ifdef touchNoEspi
			if (ts.getTouch(&xpos, &ypos))
		#else
			if (tft.getTouch(&xpos, &ypos, TOUCH_SENSIVITY))
		#endif
		{
			for (int i = 0; i < NUM_OPTIONS_BAUD_MENU; i++)
			{
				int x = START_X_BAUD_MENU;
				int y = START_Y_BAUD_MENU + i * (SQUARE_H_BAUD_MENU + GAP_Y_BAUD_MENU);

				if (xpos >= x && xpos <= (x + SQUARE_W_BAUD_MENU) &&
					ypos >= y && ypos <= (y + SQUARE_H_BAUD_MENU))
				{
					tmp = optionValues[i]; // Found the option
					escullBe = true;
					tft.fillRect(x, y, SQUARE_W_BAUD_MENU, SQUARE_H_BAUD_MENU, TFT_WHITE);
					tft.setTextColor(TFT_BLACK, TFT_WHITE);
					tft.setCursor(x + (SQUARE_W_BAUD_MENU) / 2, y + (SQUARE_H_BAUD_MENU) / 2);
					tft.println(optionLabels[i]);
					delay(DELAY_CONFIRM_BAUD_MENU);
					break;
				}
			}
		}
	}
	return tmp;
}

void giveErrorVisibility(int fast,int slow, bool init)
{
	if(!init){
		running = false;
		delay(250);
		tft.fillScreen(TFT_BLACK);
		tft.setTextSize(2);
		tft.setTextColor(TFT_RED, TFT_GREEN);
		tft.setCursor(10,(TFT_ALSSADA - (50)));
		tft.print("f:");
		tft.print(fast);
		tft.print("s:");
		tft.print(slow);
	}
	
	while(true){
		for(int i=0;i<fast;i++){
			digitalWrite(errorLed, HIGH);
			delay(ERR_FAST_BLINK);
			digitalWrite(errorLed, LOW);
			delay(ERR_FAST_BLINK);
		}
		
		for(int i=0;i<slow;i++){
			digitalWrite(errorLed, HIGH);
			delay(ERR_SLOW_BLINK);
			digitalWrite(errorLed, LOW);
			delay(ERR_SLOW_BLINK);
		}
		delay(1000);
		if(init)break;
	}
	
}

volatile bool running = false;  // Definition
CharBuffer buffer = CharBuffer(INPUT_BUFFER_SIZE, myCharBuffer);
CharBuffer bufferoUT = CharBuffer(OUTPUT_BUFFER_SIZE, myCharBuffer2);
#ifdef touchNoEspi
	XPT2046_HR2046_touch ts = XPT2046_HR2046_touch(TFT_WIDTH,TFT_HEIGHT,TOUCH_CS_PIN,&SPI1);
#endif
