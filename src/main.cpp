#include <TFT_eSPI.h>
#include <SPI.h>
#include <XPT2046_HR2046_touch.h>
#include <EEPROM.h>
/*#define TFTTFT_WIDTH  320
#define TFTTFT_HEIGHT 240*/
#define TOUCH_CS_PIN 7
#define TOUCH_IRQ 8
#define Z_THRESHOLD 350

TFT_eSPI tft = TFT_eSPI();
XPT2046_HR2046_touch ts(TFT_WIDTH, TFT_HEIGHT, TOUCH_CS_PIN, &SPI1);
class calibrator
{
public:
	bool TouchDetected = false;
	void touchDetected()
	{
		TouchDetected = !digitalRead(TOUCH_IRQ);
	}

	// New parameters borderX and borderY: horizontal and vertical distance from screen borders
	// during calibration. Defaults kept to 0 for backward compatibility.
	void calibrateTouch(uint16_t *parameters, uint32_t color_fg, uint32_t color_bg, uint8_t size, uint16_t borderX = 0, uint16_t borderY = 0)
	{
		int16_t values[] = {0, 0, 0, 0, 0, 0, 0, 0};
		uint16_t x_tmp, y_tmp;

		// corner positions adjusted by borderX/borderY and size
		for (uint8_t i = 0; i < 4; i++)
		{
			// clear the 4 calibration boxes (background)
			tft.fillRect(borderX, borderY, size + 1, size + 1, color_bg);														// top-left
			tft.fillRect(borderX, TFT_HEIGHT - borderY - size - 1, size + 1, size + 1, color_bg);								// bottom-left
			tft.fillRect(TFT_WIDTH - borderX - size - 1, borderY, size + 1, size + 1, color_bg);								// top-right
			tft.fillRect(TFT_WIDTH - borderX - size - 1, TFT_HEIGHT - borderY - size - 1, size + 1, size + 1, color_bg);		// bottom-right

			// draw target in the corner depending on i
			switch (i)
			{
			case 0: // up left
				tft.fillRect(borderX, borderY, size, size, color_fg);
				break;
			case 1: // bot left
				tft.fillRect(borderX, TFT_HEIGHT - borderY - size - 1, size, size, color_fg);
				break;
			case 2: // up right
				tft.fillRect(TFT_WIDTH - borderX - size - 1, borderY, size, size, color_fg);
				break;
			case 3: // bot right
				tft.fillRect(TFT_WIDTH - borderX - size - 1, TFT_HEIGHT - borderY - size - 1, size, size, color_fg);
				break;
			}

			// user has to get the chance to release
			if (i > 0)
				delay(1000);

			// sample 8 times and average for stability
			for (uint8_t j = 0; j < 8; j++)
			{
				// Use a lower detect threshold as corners tend to be less sensitive
				while (!ts.validTouch(&x_tmp, &y_tmp, Z_THRESHOLD / 2))
					;
				values[i * 2] += x_tmp;
				values[i * 2 + 1] += y_tmp;
			}
			values[i * 2] /= 8;
			values[i * 2 + 1] /= 8;
		}

		// determine if axes are swapped
		ts.touchCalibration_rotate = false;
		if (abs(values[0] - values[2]) > abs(values[1] - values[3]))
		{
			ts.touchCalibration_rotate = true;
			ts.touchCalibration_x0 = (values[1] + values[3]) / 2; // calc min x (raw)
			ts.touchCalibration_x1 = (values[5] + values[7]) / 2; // calc max x (raw)
			ts.touchCalibration_y0 = (values[0] + values[4]) / 2; // calc min y (raw)
			ts.touchCalibration_y1 = (values[2] + values[6]) / 2; // calc max y (raw)
		}
		else
		{
			ts.touchCalibration_x0 = (values[0] + values[2]) / 2; // calc min x (raw)
			ts.touchCalibration_x1 = (values[4] + values[6]) / 2; // calc max x (raw)
			ts.touchCalibration_y0 = (values[1] + values[5]) / 2; // calc min y (raw)
			ts.touchCalibration_y1 = (values[3] + values[7]) / 2; // calc max y (raw)
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

		// pre calculate ranges (store ranges like original code)
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
			// store display offsets used for calibration (so conversion can map raw -> full screen while
			// calibration was performed inset from borders).

			parameters[5] = borderX;
			parameters[6] = borderY;
			ts.displayOffsetX = borderX;
			ts.displayOffsetY = borderY;
		}
	}

	/* legacy/unused arrow drawing support adjusted to use TFT_WIDTH/TFT_HEIGHT */
	int start_dist = 50;
	int totalSteps = 30;
	void drawArrowToCorner(int corner, int step, bool erase = false)
	{
		// Corner positions (using full screen extents)
		int corners[4][2] = {
			{1, 1},						 // Top-left
			{1, TFT_HEIGHT - 1},		 // Bottom-left
			{TFT_WIDTH - 1, 1},			 // Top-right
			{TFT_WIDTH - 1, TFT_HEIGHT - 1}, // Bottom-right
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
			sx += start_dist;
			sy -= start_dist;
			break; // Bottom-left
		case 2:
			sx -= start_dist;
			sy += start_dist;
			break; // Top-right
		case 3:
			sx -= start_dist;
			sy -= start_dist;
			break; // Bottom-right
		}

		float progress = (float)step / totalSteps;
		int ax = sx + (ex - sx) * progress;
		int ay = sy + (ey - sy) * progress;

		// Draw arrow line (simple rectangle as line)
		if (erase)
			tft.fillRect(min(sx, ax), min(sy, ay), abs(ax - sx) + 1, abs(ay - sy) + 1, TFT_BLACK);
		else
			tft.fillRect(min(sx, ax), min(sy, ay), abs(ax - sx) + 1, abs(ay - sy) + 1, TFT_WHITE);

		// Simple arrowhead
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
		case 1: // Bottom-left
			hx1 = ax + arrow_size;
			hy1 = ay;
			hx2 = ax;
			hy2 = ay - arrow_size;
			break;
		case 2: // Top-right
			hx1 = ax - arrow_size;
			hy1 = ay;
			hx2 = ax;
			hy2 = ay + arrow_size;
			break;
		case 3: // Bottom-right
			hx1 = ax - arrow_size;
			hy1 = ay;
			hx2 = ax;
			hy2 = ay - arrow_size;
			break;
		}
		if (erase)
		{
			tft.fillRect(min(ax, hx1), min(ay, hy1), abs(hx1 - ax) + 1, abs(hy1 - ay) + 1, TFT_BLACK);
			tft.fillRect(min(ax, hx2), min(ay, hy2), abs(hx2 - ax) + 1, abs(hy2 - ay) + 1, TFT_BLACK);
		}
		else
		{
			tft.fillRect(min(ax, hx1), min(ay, hy1), abs(hx1 - ax) + 1, abs(hy1 - ay) + 1, TFT_WHITE);
			tft.fillRect(min(ax, hx2), min(ay, hy2), abs(hx2 - ax) + 1, abs(hy2 - ay) + 1, TFT_WHITE);
		}
	}
/* EEPROM-backed calibration handling
   This function was updated to store and load two extra uint16_t values:
   parameters[5] = borderX, parameters[6] = borderY
   So total stored bytes = 7 * 2 = 14 bytes (+ 2 integrity bytes at positions 0 and 1)
*/
void xpt2046CalibrateSet(uint16_t borderX, uint16_t borderY)
{
	EEPROM.begin(255);

	uint16_t calibrationData[7]; // 7 uint16_t -> 14 bytes stored (first 5 are as before, last two are offsets)
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
		for (int i = 0; i < 14; i++)
		{
			calDataTst = EEPROM.read(eePos + i);
			calibrationDataBytePoint[i] = calDataTst;
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
			// data not valid. recalibrate. Pass borderX/borderY into calibrateTouch
			calibrateTouch(calibrationData, TFT_WHITE, TFT_RED, 15, borderX, borderY);
			// store data
			while (calDataOK == EEPROM.read(0))
				calDataOK = (uint8_t)random(0, 255); // firma
			EEPROM.write(0, calDataOK);
			calDataTst = ~calDataOK;
			EEPROM.write(1, calDataTst);
			eePos = 2;
			for (int i = 0; i < 14; i++)
			{
				calDataTst = calibrationDataBytePoint[i];
				EEPROM.write(eePos + i, calDataTst);
			}
			EEPROM.commit();
			EEPROM.end();
			tft.fillScreen(TFT_BLACK);
		}
	}
}
};


static calibrator *cal = new calibrator();

void touchDetectedISR()
{
	cal->touchDetected();
}

void setup()
{
	tft.init();
	// tft.setRotation(2);
	ts.begin(tft.getRotation());
	pinMode(TOUCH_IRQ, INPUT);
	attachInterrupt(digitalPinToInterrupt(TOUCH_IRQ), touchDetectedISR, CHANGE);
	tft.fillScreen(TFT_BLACK);
	tft.setTextColor(TFT_WHITE, TFT_BLACK);
	tft.setTextDatum(MC_DATUM);
	tft.drawString("Touch Calibration", TFT_WIDTH / 2, TFT_HEIGHT / 2);
	// delay(1000);

	// Call calibration setup. You can pass border offsets (horizontal, vertical) in pixels:
	cal->xpt2046CalibrateSet(30, 30);
	//cal->xpt2046CalibrateSet();
}
uint16_t xpos, ypos;
void loop()
{
	if (cal->TouchDetected && ts.getTouch(&xpos, &ypos))
	{
		tft.fillCircle(xpos, ypos, 3, TFT_GREEN);
	}
	// delay(25);
}
