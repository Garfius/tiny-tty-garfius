#include <TFT_eSPI.h>
#include <SPI.h>
#include <XPT2046_HR2046_touch.h>
#include <EEPROM.h>
/*#define TFT_WIDTH  320
#define TFT_HEIGHT 240*/
#define TOUCH_CS_PIN 7
#define TOUCH_IRQ 8

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

	void calibrateTouch(uint16_t *parameters, uint32_t color_fg, uint32_t color_bg, uint8_t size)
	{
		int16_t values[] = {0, 0, 0, 0, 0, 0, 0, 0};
		uint16_t x_tmp, y_tmp;
		unsigned int endTime;
		int step = 0;
		tft.fillScreen(TFT_BLACK);
		tft.setTextColor(TFT_WHITE, TFT_BLACK);
		tft.setTextDatum(MC_DATUM);
		tft.drawString("Touch Calibration", TFT_WIDTH / 3, TFT_HEIGHT / 3);
		for (uint8_t i = 0; i < 4; i++)
		{
			if (i > 0)
				delay(1000);
			endTime = millis() + 5000;
			// init reversed values
			switch (i)
			{
				case 0:
				values[i * 2] = 4095;
				values[i * 2 + 1] = 0;
				break;
				case 1:
				values[i * 2] = 4095;
				values[i * 2 + 1] = 4095;
				break;
				case 2:
				values[i * 2] = 0;
				values[i * 2 + 1] = 0;
				break;
				case 3:
				values[i * 2] = 0;
				values[i * 2 + 1] = 4095;
					break;
			}
			bool better;
			int touches = 0;
			tft.setCursor(TFT_WIDTH / 2, TFT_HEIGHT / 2);
			tft.setTextColor(TFT_WHITE, TFT_BLACK);
			tft.print("      ");
			while (millis() < endTime)
			{
				better = false;
				this->drawArrowToCorner(i, step, false);
				tft.setCursor(TFT_WIDTH / 2, TFT_HEIGHT / 2);
				tft.setTextColor(TFT_WHITE, better ? TFT_GREEN : TFT_RED);
				tft.printf("T:%d", touches);
				if (ts.validTouch(&x_tmp, &y_tmp))
				{
					touches++;
					switch (i)
					{
					case 0:
						if (values[i * 2] > x_tmp)
						{
							values[i * 2] = x_tmp;
							better = true;
						}
						if (values[i * 2 + 1] < y_tmp)
						{
							values[i * 2 + 1] = y_tmp;
							better = true;
						}

						break;
					case 1:
						if (values[i * 2] > x_tmp)
                        {
                            values[i * 2] = x_tmp;
                            better = true;
                        }
                        if (values[i * 2 + 1] > y_tmp)
                        {
                            values[i * 2 + 1] = y_tmp;
                            better = true;
                        }

						break;
					case 2:
						if (values[i * 2] < x_tmp)
						{
							values[i * 2] = x_tmp;
							better = true;
						}
						if (values[i * 2 + 1] < y_tmp)
						{
							values[i * 2 + 1] = y_tmp;
							better = true;
						}
						
						break;
					case 3:
						if (values[i * 2] < x_tmp)
						{
							values[i * 2] = x_tmp;
							better = true;
						}
						if (values[i * 2 + 1] > y_tmp)
						{
							values[i * 2 + 1] = y_tmp;
							better = true;
						}
						break;
					}
				}
				delay(10);
				this->drawArrowToCorner(i, step, true);
				(step < totalSteps) ? step++ : step = 0;
			}
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
		tft.fillScreen(TFT_BLACK);
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
	int start_dist = 50;
	int totalSteps = 30;
	void drawArrowToCorner(int corner, int step, bool erase = false)
	{
		// Corner positions
		int corners[4][2] = {
			{1, 1},							 // Top-left - bx dre
			{1, TFT_HEIGHT - 1},				 // Bottom-left - dl dre
			{TFT_WIDTH - 1, 1},				 // Top-right - bx esq
			{TFT_WIDTH - 1, TFT_HEIGHT - 1}, // Bottom-right - dl esq
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
			break; // Top-left - bx dre
		case 1:
			sx += start_dist;
			sy -= start_dist;
			break; // Bottom-left - dl dre
		case 2:
			sx -= start_dist;
			sy += start_dist;
			break; // Top-right - bx esq
		case 3:
			sx -= start_dist;
			sy -= start_dist;
			break; // Bottom-right - dl esq
		}

		float progress = (float)step / totalSteps;
		int ax = sx + (ex - sx) * progress;
		int ay = sy + (ey - sy) * progress;

		// Draw arrow line
		tft.drawLine(sx, sy, ax, ay, erase ? TFT_BLACK : TFT_WHITE);
		
		// Draw simple arrowhead (vertical/horizontal only, no trig)
		int hx1 = ax, hy1 = ay, hx2 = ax, hy2 = ay;
		int arrow_size = 15;
		switch (corner)
		{
		case 0: // Top-left - bx dre
			hx1 = ax + arrow_size;
			hy1 = ay;
			hx2 = ax;
			hy2 = ay + arrow_size;
		break;
		case 1: // Bottom-left - dl dre
			hx1 = ax + arrow_size;
			hy1 = ay;
			hx2 = ax;
			hy2 = ay - arrow_size;
			break;
		case 2: // Top-right - bx esq
			hx1 = ax - arrow_size;
			hy1 = ay;
			hx2 = ax;
			hy2 = ay + arrow_size;
			break;
		case 3: // Bottom-right - dl esq
			hx1 = ax - arrow_size;
			hy1 = ay;
			hx2 = ax;
			hy2 = ay - arrow_size;
			break;
		}
		if (erase)
		{
			tft.drawLine(ax, ay, hx1, hy1, TFT_BLACK);
			tft.drawLine(ax, ay, hx2, hy2, TFT_BLACK);
		}
		else
		{
			tft.drawLine(ax, ay, hx1, hy1, TFT_WHITE);
			tft.drawLine(ax, ay, hx2, hy2, TFT_WHITE);
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
	cal->xpt2046CalibrateSet();
}
uint16_t xpos, ypos;
void loop()
{
	if (ts.getTouch(&xpos, &ypos))
		tft.drawCircle(xpos, ypos, 5, TFT_GREEN);
	delay(50);
}