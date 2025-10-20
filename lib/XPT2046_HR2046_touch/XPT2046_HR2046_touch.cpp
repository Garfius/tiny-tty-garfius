#include <XPT2046_HR2046_touch.h>
// The following touch screen support code by maxpautsch was merged 1/10/17
// https://github.com/maxpautsch

// Define TOUCH_CS is the user setup file to enable this code

// A demo is provided in examples Generic folder

// Additions by Bodmer to double sample, use Z value to improve detection reliability
// and to correct rotation handling

// See license in root directory.

// Define a default pressure threshold
#define SPI_SETTING SPISettings(SPI_TOUCH_FREQUENCY, MSBFIRST, SPI_MODE0)
/***************************************************************************************
** Function name:           getTouchRaw
** Description:             read raw touch position.  Always returns true.
***************************************************************************************/
uint8_t XPT2046_HR2046_touch::getTouchRaw(uint16_t *x, uint16_t *y)
{
	uint16_t tmp;

	_spi->beginTransaction(SPI_SETTING);
	digitalWrite(_csPin, LOW);

	// Start YP sample request for x position, read 4 times and keep last sample
	_spi->transfer(0xd0); // Start new YP conversion
	_spi->transfer(0);	  // Read first 8 bits
	_spi->transfer(0xd0); // Read last 8 bits and start new YP conversion
	_spi->transfer(0);	  // Read first 8 bits
	_spi->transfer(0xd0); // Read last 8 bits and start new YP conversion
	_spi->transfer(0);	  // Read first 8 bits
	_spi->transfer(0xd0); // Read last 8 bits and start new YP conversion

	tmp = _spi->transfer(0); // Read first 8 bits
	tmp = tmp << 5;
	tmp |= 0x1f & (_spi->transfer(0x90) >> 3); // Read last 8 bits and start new XP conversion

	*x = tmp;

	// Start XP sample request for y position, read 4 times and keep last sample
	_spi->transfer(0);	  // Read first 8 bits
	_spi->transfer(0x90); // Read last 8 bits and start new XP conversion
	_spi->transfer(0);	  // Read first 8 bits
	_spi->transfer(0x90); // Read last 8 bits and start new XP conversion
	_spi->transfer(0);	  // Read first 8 bits
	_spi->transfer(0x90); // Read last 8 bits and start new XP conversion

	tmp = _spi->transfer(0); // Read first 8 bits
	tmp = tmp << 5;
	tmp |= 0x1f & (_spi->transfer(0) >> 3); // Read last 8 bits

	*y = tmp;

	digitalWrite(_csPin, HIGH);
	_spi->endTransaction();

	return true;
}

/***************************************************************************************
** Function name:           getTouchRawZ
** Description:             read raw pressure on touchpad and return Z value.
***************************************************************************************/
uint16_t XPT2046_HR2046_touch::getTouchRawZ(void)
{

	_spi->beginTransaction(SPI_SETTING);
	digitalWrite(_csPin, LOW);

	// Z sample request
	int16_t tz = 0xFFF;
	_spi->transfer(0xb0);			   // Start new Z1 conversion
	tz += _spi->transfer16(0xc0) >> 3; // Read Z1 and start Z2 conversion
	tz -= _spi->transfer16(0x00) >> 3; // Read Z2

	digitalWrite(_csPin, HIGH);
	_spi->endTransaction();

	if (tz == 4095)
		tz = 0;

	return (uint16_t)tz;
}
/**
 * Function name:           isTouching
 * Description:             Returns true if screen is being touched, FFFASTER than validTouch()
 */
bool XPT2046_HR2046_touch::isTouching(uint16_t threshold)
{
	uint16_t z1 = 1;
	uint16_t z2 = 0;
	while (z1 > z2)
	{
		z2 = z1;
		z1 = getTouchRawZ();
		delay(1);
	}

	return !(z1 <= threshold);
}
/***************************************************************************************
** Function name:           validTouch
** Description:             read validated position. Return false if not pressed.
***************************************************************************************/
#define _RAWERR 20 // Deadband error allowed in successive position samples
uint8_t XPT2046_HR2046_touch::validTouch(uint16_t *x, uint16_t *y, uint16_t threshold)
{
	uint16_t x_tmp, y_tmp, x_tmp2, y_tmp2;

	// Wait until pressure stops increasing to debounce pressure
	uint16_t z1 = 1;
	uint16_t z2 = 0;
	while (z1 > z2)
	{
		z2 = z1;
		z1 = getTouchRawZ();
		delay(1);
	}

	//  Serial.print("Z = ");Serial.println(z1);

	if (z1 <= threshold)
		return false;

	getTouchRaw(&x_tmp, &y_tmp);

	//  Serial.print("Sample 1 x,y = "); Serial.print(x_tmp);Serial.print(",");Serial.print(y_tmp);
	//  Serial.print(", Z = ");Serial.println(z1);

	delay(1); // Small delay to the next sample
	if (getTouchRawZ() <= threshold)
		return false;

	delay(2); // Small delay to the next sample
	getTouchRaw(&x_tmp2, &y_tmp2);

	//  Serial.print("Sample 2 x,y = "); Serial.print(x_tmp2);Serial.print(",");Serial.println(y_tmp2);
	//  Serial.print("Sample difference = ");Serial.print(abs(x_tmp - x_tmp2));Serial.print(",");Serial.println(abs(y_tmp - y_tmp2));

	if (abs(x_tmp - x_tmp2) > _RAWERR)
		return false;
	if (abs(y_tmp - y_tmp2) > _RAWERR)
		return false;

	*x = x_tmp;
	*y = y_tmp;

	return true;
}

/***************************************************************************************
** Function name:           getTouch
** Description:             read calibrated position. Return false if not pressed.
***************************************************************************************/
uint8_t XPT2046_HR2046_touch::getTouch(uint16_t *x, uint16_t *y, uint16_t threshold)
{
	uint16_t x_tmp, y_tmp;

	if (threshold < 20)
		threshold = 20;
	if (_pressTime > millis())
		threshold = 20;

	uint8_t n = 5;
	uint8_t valid = 0;
	while (n--)
	{
		if (validTouch(&x_tmp, &y_tmp, threshold))
			valid++;
		;
	}

	if (valid < 1)
	{
		_pressTime = 0;
		return false;
	}

	_pressTime = millis() + 50;
	Point_XPT2046_HR2046_touch mtpp = this->mapPoint(x_tmp, y_tmp);
	x_tmp = (uint16_t)mtpp.x;
	y_tmp = (uint16_t)mtpp.y;

	if (x_tmp >= _width || y_tmp >= _height)
		return false;

	_pressX = x_tmp;
	_pressY = y_tmp;
	*x = _pressX;
	*y = _pressY;
	return valid;
}
void XPT2046_HR2046_touch::begin()
{
	_spi->begin();
	pinMode(_csPin, OUTPUT);
	digitalWrite(_csPin, HIGH);
}
Point_XPT2046_HR2046_touch XPT2046_HR2046_touch::mapPoint(float x, float y)
{
	float u = x / 100.0;
	float v = y / 100.0;

	Point_XPT2046_HR2046_touch top, bottom, result;

	// Interpolació horitzontal
	top.x = (1 - u) * dst[0].x + u * dst[1].x;
	top.y = (1 - u) * dst[0].y + u * dst[1].y;

	bottom.x = (1 - u) * dst[2].x + u * dst[3].x;
	bottom.y = (1 - u) * dst[2].y + u * dst[3].y;

	// Interpolació vertical
	result.x = (1 - v) * top.x + v * bottom.x;
	result.y = (1 - v) * top.y + v * bottom.y;

	return result;
}