#ifndef _XPT2046_Touchscreen_h_
#define _XPT2046_Touchscreen_h_

#include <Arduino.h>
#include <SPI.h>

#define SPI_TOUCH_FREQUENCY 2500000
#define TOUCH_THRESHOLD 600

class XPT2046_HR2046_touch
{
public:
	XPT2046_HR2046_touch(uint16_t width, uint16_t height, uint8_t cspin, SPIClass *spi = &SPI)
		: _width(width), _height(height), _csPin(cspin), _spi(spi) {}
	// Get raw x,y ADC values from touch controller
	uint8_t getTouchRaw(uint16_t *x, uint16_t *y);
	// Get raw z (i.e. pressure) ADC value from touch controller
	uint16_t getTouchRawZ(void);
	// Convert raw x,y values to calibrated and correctly rotated screen coordinates
	void convertRawXY(uint16_t *x, uint16_t *y);
	// Get the screen touch coordinates, returns true if screen has been touched
	// if the touch coordinates are off screen then x and y are not updated
	// The returned value can be treated as a bool type, false or 0 means touch not detected
	// In future the function may return an 8-bit "quality" (jitter) value.
	// The threshold value is optional, this must be higher than the bias level for z (pressure)
	// reported by Test_Touch_Controller when the screen is NOT touched. When touched the z value
	// must be higher than the threshold for a touch to be detected.
	uint8_t getTouch(uint16_t *x, uint16_t *y, uint16_t threshold = TOUCH_THRESHOLD);
	// Set the screen calibration values (parameters layout extended):
	// parameters[0] = touchCalibration_x0 (raw min)
	// parameters[1] = touchCalibration_x1 (raw range max-min)
	// parameters[2] = touchCalibration_y0 (raw min)
	// parameters[3] = touchCalibration_y1 (raw range max-min)
	// parameters[4] = flags (rotate/invert bits)
	// parameters[5] = displayOffsetX (pixels inset from left/right during calibration)
	// parameters[6] = displayOffsetY (pixels inset from top/bottom during calibration)
	//void setTouch(uint16_t *data);

	void begin(uint8_t touchCalibration_rotate = 1, uint8_t touchCalibration_invert_x = 2, uint8_t touchCalibration_invert_y = 0);
	uint16_t touchCalibration_x0 = 300, touchCalibration_x1 = 3600, touchCalibration_y0 = 300, touchCalibration_y1 = 3600;
	uint8_t touchCalibration_rotate, touchCalibration_invert_x, touchCalibration_invert_y;
	uint8_t validTouch(uint16_t *x, uint16_t *y, uint16_t threshold = TOUCH_THRESHOLD);
	void setCalibrationData(uint16_t *parameters);
	bool isTouching(uint16_t threshold = TOUCH_THRESHOLD);
	uint16_t displayOffsetX = 0;
	uint16_t displayOffsetY = 0;

private:
	// Legacy support only - deprecated TODO: delete
	void spi_begin_touch();
	void spi_end_touch();

	// Private function to validate a touch, allow settle time and reduce spurious coordinates

	// Initialise with example calibration values so processor does not crash if setTouch() not called in setup()

	uint32_t _pressTime;	   // Press and hold time-out
	uint16_t _pressX, _pressY; // For future use (last sampled calibrated coordinates)
	SPIClass *_spi;
	uint16_t _width, _height;
	uint8_t _csPin;

	// Display offsets (pixels) used when calibration targets were inset from edges.
	// These default to 0 (full-screen calibration). Values are set by setCalibrationData / setTouch
};
#endif