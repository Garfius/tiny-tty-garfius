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
uint16_t touchCalibration_x0 = 300, touchCalibration_x1 = 3600, touchCalibration_y0 = 300, touchCalibration_y1 = 3600;
uint8_t  touchCalibration_rotate = 1, touchCalibration_invert_x = 2, touchCalibration_invert_y = 0;
bool isTouching(){
  uint16_t z1 = 1;
  uint16_t z2 = 0;
	
	while (z1 > z2)
	{
		z2 = z1;
		z1 = ts.getTouchRawZ();
		delay(1);
	}

  if (z1 <= TOUCH_SENSIVITY) return false;
  return true;
}
bool getTouchRaw(uint16_t *x, uint16_t *y){
  TS_Point puntTmp;
  //if(!isTouching())return false;
  
	puntTmp = ts.getPoint();
	*x = puntTmp.x;
	*y = puntTmp.y;
	return true;
}
bool getTouchDisplay(uint16_t *x, uint16_t *y){
	if(!getTouchRaw(x,y))return false;
	convertRawXY(x,y);
	return true;
}

void convertRawXY(uint16_t *x, uint16_t *y)
{
  uint16_t x_tmp = *x, y_tmp = *y, xx, yy;

  if(!touchCalibration_rotate){
    xx=(x_tmp-touchCalibration_x0)*TFT_AMPLADA/touchCalibration_x1;
    yy=(y_tmp-touchCalibration_y0)*TFT_ALSSADA/touchCalibration_y1;
    if(touchCalibration_invert_x)
      xx = TFT_AMPLADA - xx;
    if(touchCalibration_invert_y)
      yy = TFT_ALSSADA - yy;
  } else {
    xx=(y_tmp-touchCalibration_x0)*TFT_AMPLADA/touchCalibration_x1;
    yy=(x_tmp-touchCalibration_y0)*TFT_ALSSADA/touchCalibration_y1;
    if(touchCalibration_invert_x)
      xx = TFT_AMPLADA - xx;
    if(touchCalibration_invert_y)
      yy = TFT_ALSSADA - yy;
  }
  *x = xx;
  *y = yy;
}

/***************************************************************************************
** Function name:           calibrateTouch
** Description:             generates calibration parameters for touchscreen. 
***************************************************************************************/
void xpt2046CalibrateGet(uint16_t *parameters, uint32_t color_fg, uint32_t color_bg, uint8_t size){
  int16_t values[] = {0,0,0,0,0,0,0,0};
  uint16_t x_tmp, y_tmp;

  for(uint8_t i = 0; i<4; i++){
    tft.fillRect(0, 0, size+1, size+1, color_bg);
    tft.fillRect(0, TFT_ALSSADA-size-1, size+1, size+1, color_bg);
    tft.fillRect(TFT_AMPLADA-size-1, 0, size+1, size+1, color_bg);
    tft.fillRect(TFT_AMPLADA-size-1, TFT_ALSSADA-size-1, size+1, size+1, color_bg);

    if (i == 5) break; // used to clear the arrows
    
    switch (i) {
      case 0: // up left
        tft.drawLine(0, 0, 0, size, color_fg);
        tft.drawLine(0, 0, size, 0, color_fg);
        tft.drawLine(0, 0, size , size, color_fg);
        break;
      case 1: // bot left
        tft.drawLine(0, TFT_ALSSADA-size-1, 0, TFT_ALSSADA-1, color_fg);
        tft.drawLine(0, TFT_ALSSADA-1, size, TFT_ALSSADA-1, color_fg);
        tft.drawLine(size, TFT_ALSSADA-size-1, 0, TFT_ALSSADA-1 , color_fg);
        break;
      case 2: // up right
        tft.drawLine(TFT_AMPLADA-size-1, 0, TFT_AMPLADA-1, 0, color_fg);
        tft.drawLine(TFT_AMPLADA-size-1, size, TFT_AMPLADA-1, 0, color_fg);
        tft.drawLine(TFT_AMPLADA-1, size, TFT_AMPLADA-1, 0, color_fg);
        break;
      case 3: // bot right
        tft.drawLine(TFT_AMPLADA-size-1, TFT_ALSSADA-size-1, TFT_AMPLADA-1, TFT_ALSSADA-1, color_fg);
        tft.drawLine(TFT_AMPLADA-1, TFT_ALSSADA-1-size, TFT_AMPLADA-1, TFT_ALSSADA-1, color_fg);
        tft.drawLine(TFT_AMPLADA-1-size, TFT_ALSSADA-1, TFT_AMPLADA-1, TFT_ALSSADA-1, color_fg);
        break;
      }

    // user has to get the chance to release
    if(i>0) delay(1000);

    for(uint8_t j= 0; j<8; j++){
      // Use a lower detect threshold as corners tend to be less sensitive
	  while(!isTouching());
	  getTouchRaw(&x_tmp, &y_tmp);
      values[i*2  ] += x_tmp;
      values[i*2+1] += y_tmp;
      }
    values[i*2  ] /= 8;
    values[i*2+1] /= 8;
  }


  // from case 0 to case 1, the y value changed. 
  // If the measured delta of the touch x axis is bigger than the delta of the y axis, the touch and TFT axes are switched.
  touchCalibration_rotate = false;
  if(abs(values[0]-values[2]) > abs(values[1]-values[3])){
    touchCalibration_rotate = true;
    touchCalibration_x0 = (values[1] + values[3])/2; // calc min x
    touchCalibration_x1 = (values[5] + values[7])/2; // calc max x
    touchCalibration_y0 = (values[0] + values[4])/2; // calc min y
    touchCalibration_y1 = (values[2] + values[6])/2; // calc max y
  } else {
    touchCalibration_x0 = (values[0] + values[2])/2; // calc min x
    touchCalibration_x1 = (values[4] + values[6])/2; // calc max x
    touchCalibration_y0 = (values[1] + values[5])/2; // calc min y
    touchCalibration_y1 = (values[3] + values[7])/2; // calc max y
  }

  // in addition, the touch screen axis could be in the opposite direction of the TFT axis
  touchCalibration_invert_x = false;
  if(touchCalibration_x0 > touchCalibration_x1){
    values[0]=touchCalibration_x0;
    touchCalibration_x0 = touchCalibration_x1;
    touchCalibration_x1 = values[0];
    touchCalibration_invert_x = true;
  }
  touchCalibration_invert_y = false;
  if(touchCalibration_y0 > touchCalibration_y1){
    values[0]=touchCalibration_y0;
    touchCalibration_y0 = touchCalibration_y1;
    touchCalibration_y1 = values[0];
    touchCalibration_invert_y = true;
  }

  // pre calculate
  touchCalibration_x1 -= touchCalibration_x0;
  touchCalibration_y1 -= touchCalibration_y0;

  if(touchCalibration_x0 == 0) touchCalibration_x0 = 1;
  if(touchCalibration_x1 == 0) touchCalibration_x1 = 1;
  if(touchCalibration_y0 == 0) touchCalibration_y0 = 1;
  if(touchCalibration_y1 == 0) touchCalibration_y1 = 1;

  // export parameters, if pointer valid
  if(parameters != NULL){
    parameters[0] = touchCalibration_x0;
    parameters[1] = touchCalibration_x1;
    parameters[2] = touchCalibration_y0;
    parameters[3] = touchCalibration_y1;
    parameters[4] = touchCalibration_rotate | (touchCalibration_invert_x <<1) | (touchCalibration_invert_y <<2);
  }
}
void setCalibrationData(uint16_t *parameters){
  touchCalibration_x0 = parameters[0];
  touchCalibration_x1 = parameters[1];
  touchCalibration_y0 = parameters[2];
  touchCalibration_y1 = parameters[3];

  if(touchCalibration_x0 == 0) touchCalibration_x0 = 1;
  if(touchCalibration_x1 == 0) touchCalibration_x1 = 1;
  if(touchCalibration_y0 == 0) touchCalibration_y0 = 1;
  if(touchCalibration_y1 == 0) touchCalibration_y1 = 1;

  touchCalibration_rotate = parameters[4] & 0x01;
  touchCalibration_invert_x = parameters[4] & 0x02;
  touchCalibration_invert_y = parameters[4] & 0x04;
}

void xpt2046CalibrateSet()
{
	EEPROM.begin(255);

	ts.begin(TOUCH_SENSIVITY);
	touchCalibration_rotate = tft.getRotation();
	uint16_t calibrationData[7]; // els 2 extra son per fer el check de si es vol fer a mà
	uint8_t *calibrationDataBytePoint = (uint8_t *)calibrationData;
	uint8_t calDataOK = 0;
	uint8_t calDataTst = 0;
	//-------------------eeprom integrity check
	calDataOK = EEPROM.read(0);
	calDataTst = ~EEPROM.read(1);
	calDataOK = calDataOK == calDataTst;
	int eePos = 2;
	if (calDataOK && (!isTouching()))
	{
		// calibration data valid & NO TSOLICITED
		for (int i = 0; i < 10; i++)
		{
			calDataTst = EEPROM.read(eePos + i);
			calibrationDataBytePoint[i] = calDataTst;
			eePos++;
		}
		setCalibrationData(calibrationData);
	}
	else
	{
		if (isTouching())
		{
			tft.fillScreen(TFT_YELLOW);
			delay(1000);
			tft.fillScreen(TFT_BLACK);
			// data not valid. recalibrate
			xpt2046CalibrateGet(calibrationData, TFT_WHITE, TFT_RED, 15);
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
	}
	tft.fillScreen(TFT_BLACK);
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
	tft.setCursor(0, 0);
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
			if (getTouchDisplay(&xpos, &ypos))
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
		tft.fillScreen(TFT_BLACK);
		tft.setTextSize(2);
		tft.setTextColor(TFT_BLACK, TFT_WHITE);
		tft.setCursor(10,(TFT_ALSSADA - 30));
		tft.print("f:");
		tft.print(fast);
		tft.print("s:");
		tft.print(slow);
	}
	
	pinMode(errorLed, OUTPUT);
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

CharBuffer buffer = CharBuffer(INPUT_BUFFER_SIZE, myCharBuffer);
CharBuffer bufferoUT = CharBuffer(OUTPUT_BUFFER_SIZE, myCharBuffer2);
#ifdef touchNoEspi
	XPT2046_Touchscreen ts(TOUCH_CS_PIN, 255,&SPI1);
#endif