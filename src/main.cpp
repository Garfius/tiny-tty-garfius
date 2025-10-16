#include <TFT_eSPI.h>
#include <SPI.h>
#include <XPT2046_HR2046_touch.h>
#include <EEPROM.h>
/*#define TFT_WIDTH  320
#define TFT_HEIGHT 240*/
#define TOUCH_CS_PIN 7
#define TOUCH_IRQ 8
#define Z_THRESHOLD 350 // Touch pressure threshold for validating touches

TFT_eSPI tft = TFT_eSPI();
XPT2046_HR2046_touch ts(TFT_WIDTH, TFT_HEIGHT, TOUCH_CS_PIN, &SPI1);
bool TouchDetected = false;
void touchDetected()
{
	TouchDetected = !digitalRead(TOUCH_IRQ);
}

void calibrateTouch(uint16_t *parameters, uint32_t color_fg, uint32_t color_bg, uint8_t size)
{
	int16_t values[] = {0, 0, 0, 0, 0, 0, 0, 0};
	uint16_t x_tmp, y_tmp;

	for (uint8_t i = 0; i < 4; i++)
	{
		tft.fillRect(0, 0, size + 1, size + 1, color_bg);
		tft.fillRect(0, TFT_HEIGHT - size - 1, size + 1, size + 1, color_bg);
		tft.fillRect(TFT_WIDTH - size - 1, 0, size + 1, size + 1, color_bg);
		tft.fillRect(TFT_WIDTH - size - 1, TFT_HEIGHT - size - 1, size + 1, size + 1, color_bg);

		if (i == 5)
			break; // used to clear the arrows

		switch (i)
		{
		case 0: // up left
			tft.drawLine(0, 0, 0, size, color_fg);
			tft.drawLine(0, 0, size, 0, color_fg);
			tft.drawLine(0, 0, size, size, color_fg);
			break;
		case 1: // bot left
			tft.drawLine(0, TFT_HEIGHT - size - 1, 0, TFT_HEIGHT - 1, color_fg);
			tft.drawLine(0, TFT_HEIGHT - 1, size, TFT_HEIGHT - 1, color_fg);
			tft.drawLine(size, TFT_HEIGHT - size - 1, 0, TFT_HEIGHT - 1, color_fg);
			break;
		case 2: // up right
			tft.drawLine(TFT_WIDTH - size - 1, 0, TFT_WIDTH - 1, 0, color_fg);
			tft.drawLine(TFT_WIDTH - size - 1, size, TFT_WIDTH - 1, 0, color_fg);
			tft.drawLine(TFT_WIDTH - 1, size, TFT_WIDTH - 1, 0, color_fg);
			break;
		case 3: // bot right
			tft.drawLine(TFT_WIDTH - size - 1, TFT_HEIGHT - size - 1, TFT_WIDTH - 1, TFT_HEIGHT - 1, color_fg);
			tft.drawLine(TFT_WIDTH - 1, TFT_HEIGHT - 1 - size, TFT_WIDTH - 1, TFT_HEIGHT - 1, color_fg);
			tft.drawLine(TFT_WIDTH - 1 - size, TFT_HEIGHT - 1, TFT_WIDTH - 1, TFT_HEIGHT - 1, color_fg);
			break;
		}

		// user has to get the chance to release
		if (i > 0)
			delay(1000);

		for (uint8_t j = 0; j < 8; j++)
		{
			// Use a lower detect threshold as corners tend to be less sensitive
			while (!TouchDetected && !ts.isTouching())
				;
			while (!ts.validTouch(&x_tmp, &y_tmp, Z_THRESHOLD / 2))
				;
			values[i * 2] += x_tmp;
			values[i * 2 + 1] += y_tmp;
		}
		values[i * 2] /= 8;
		values[i * 2 + 1] /= 8;
	}

	// from case 0 to case 1, the y value changed.
	// If the measured delta of the touch x axis is bigger than the delta of the y axis, the touch and TFT axes are switched.
	ts.touchCalibration_rotate = false;
	if (abs(values[0] - values[2]) > abs(values[1] - values[3]))
	{
		ts.touchCalibration_rotate = true;
		ts.touchCalibration_x0 = (values[1] + values[3]) / 2; // calc min x
		ts.touchCalibration_x1 = (values[5] + values[7]) / 2; // calc max x
		ts.touchCalibration_y0 = (values[0] + values[4]) / 2; // calc min y
		ts.touchCalibration_y1 = (values[2] + values[6]) / 2; // calc max y
	}
	else
	{
		ts.touchCalibration_x0 = (values[0] + values[2]) / 2; // calc min x
		ts.touchCalibration_x1 = (values[4] + values[6]) / 2; // calc max x
		ts.touchCalibration_y0 = (values[1] + values[5]) / 2; // calc min y
		ts.touchCalibration_y1 = (values[3] + values[7]) / 2; // calc max y
	}

	// in addition, the touch screen axis could be in the opposite direction of the TFT axis
	ts.touchCalibration_invert_x = false;
	if (ts.touchCalibration_x0 > ts.touchCalibration_x1)
	{
		values[0] = ts.touchCalibration_x0;
		ts.touchCalibration_x0 = ts.touchCalibration_x1;
		ts.touchCalibration_x1 = values[0];
		ts.touchCalibration_invert_x = true;
	}
	ts.touchCalibration_invert_y = false;
	if (ts.touchCalibration_y0 > ts.touchCalibration_y1)
	{
		values[0] = ts.touchCalibration_y0;
		ts.touchCalibration_y0 = ts.touchCalibration_y1;
		ts.touchCalibration_y1 = values[0];
		ts.touchCalibration_invert_y = true;
	}

	// pre calculate
	ts.touchCalibration_x1 -= ts.touchCalibration_x0;
	ts.touchCalibration_y1 -= ts.touchCalibration_y0;

	if (ts.touchCalibration_x0 == 0)
		ts.touchCalibration_x0 = 1;
	if (ts.touchCalibration_x1 == 0)
		ts.touchCalibration_x1 = 1;
	if (ts.touchCalibration_y0 == 0)
		ts.touchCalibration_y0 = 1;
	if (ts.touchCalibration_y1 == 0)
		ts.touchCalibration_y1 = 1;

	// export parameters, if pointer valid
	if (parameters != NULL)
	{
		parameters[0] = ts.touchCalibration_x0;
		parameters[1] = ts.touchCalibration_x1;
		parameters[2] = ts.touchCalibration_y0;
		parameters[3] = ts.touchCalibration_y1;
		parameters[4] = ts.touchCalibration_rotate | (ts.touchCalibration_invert_x << 1) | (ts.touchCalibration_invert_y << 2);
	}
}
void xpt2046CalibrateSet()
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
	calDataTst = ts.isTouching();
	if (calDataOK && (!calDataTst))
	{
		// calibration data valid & NO TSOLICITED
		for (int i = 0; i < 10; i++)
		{
			calDataTst = EEPROM.read(eePos + i);
			calibrationDataBytePoint[i] = calDataTst;
			eePos++;
		}
		ts.setCalibrationData(calibrationData);
	}
	else
	{
		if (calDataTst)
		{
			tft.fillScreen(TFT_YELLOW);
			delay(1000);
			tft.fillScreen(TFT_BLACK);
			// data not valid. recalibrate
			calibrateTouch(calibrationData, TFT_WHITE, TFT_RED, 15);
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
			tft.fillScreen(TFT_BLACK);
		}
	}
}

void drawArrowToCorner(int corner, int start_dist, uint16_t color = TFT_RED)
{
	// Corner positions
	int corners[4][2] = {
		{1, 1},							 // Top-left
		{TFT_WIDTH - 1, 1},				 // Top-right
		{TFT_WIDTH - 1, TFT_HEIGHT - 1}, // Bottom-right
		{1, TFT_HEIGHT - 1}				 // Bottom-left
	};
	int ex = corners[corner][0];
	int ey = corners[corner][1];

	// Calculate start point based on which corner
	int sx = ex, sy = ey;
	switch (corner)
	{
	case 0:
		sx += start_dist;
		sy += start_dist;
		break; // Top-left
	case 1:
		sx -= start_dist;
		sy += start_dist;
		break; // Top-right
	case 2:
		sx -= start_dist;
		sy -= start_dist;
		break; // Bottom-right
	case 3:
		sx += start_dist;
		sy -= start_dist;
		break; // Bottom-left
	}

	int steps = 30;
	for (int i = 0; i <= steps; i++)
	{
		float progress = (float)i / steps;
		int ax = sx + (ex - sx) * progress;
		int ay = sy + (ey - sy) * progress;

		// tft.fillScreen(TFT_BLACK);

		// Draw arrow line
		tft.drawLine(sx, sy, ax, ay, color);

		// Draw simple arrowhead (vertical/horizontal only, no trig)
		int hx1 = ax, hy1 = ay, hx2 = ax, hy2 = ay;
		int arrow_size = 15;
		switch (corner)
		{
		case 0: // Top-left
			hx1 = ax + arrow_size;
			hy1 = ay;
			hx2 = ax;
			hy2 = ay + arrow_size;
			break;
		case 1: // Top-right
			hx1 = ax - arrow_size;
			hy1 = ay;
			hx2 = ax;
			hy2 = ay + arrow_size;
			break;
		case 2: // Bottom-right
			hx1 = ax - arrow_size;
			hy1 = ay;
			hx2 = ax;
			hy2 = ay - arrow_size;
			break;
		case 3: // Bottom-left
			hx1 = ax + arrow_size;
			hy1 = ay;
			hx2 = ax;
			hy2 = ay - arrow_size;
			break;
		}
		tft.drawLine(ax, ay, hx1, hy1, color);
		tft.drawLine(ax, ay, hx2, hy2, color);

		delay(30); // ~1.2s total animation
		tft.drawLine(sx, sy, ax, ay, TFT_BLACK);
		tft.drawLine(ax, ay, hx1, hy1, TFT_BLACK);
		tft.drawLine(ax, ay, hx2, hy2, TFT_BLACK);
	}
}

void setup()
{
	tft.init();
	// tft.setRotation(2);
	ts.begin(tft.getRotation());
	pinMode(TOUCH_IRQ, INPUT);
	attachInterrupt(digitalPinToInterrupt(TOUCH_IRQ), touchDetected, CHANGE);
	tft.fillScreen(TFT_BLACK);
	tft.setTextColor(TFT_WHITE, TFT_BLACK);
	tft.setTextDatum(MC_DATUM);
	tft.drawString("Touch Calibration", TFT_WIDTH / 2, TFT_HEIGHT / 2);
	// delay(1000);
	xpt2046CalibrateSet();
}
uint16_t xpos, ypos;
void loop()
{
	/*int dist = 50; // Starting distance from corner
	for (int corner = 0; corner < 4; corner++) {
		drawArrowToCorner(corner, dist, TFT_CYAN);
		delay(800);
	}*/

	if (ts.getTouch(&xpos, &ypos))
		tft.drawCircle(xpos, ypos, 5, TFT_GREEN);
	delay(50);
}