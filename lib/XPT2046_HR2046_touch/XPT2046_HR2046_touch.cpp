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
	Point_XPT2046_HR2046_touch mtpp;
	if (!this->mapPoint(x_tmp, y_tmp, mtpp))
	{
		return false; // Calibration error or invalid mapping
	}

	x_tmp = (uint16_t)mtpp.x;
	y_tmp = (uint16_t)mtpp.y;

	if (x_tmp >= _width || y_tmp >= _height)
		return false;
	this->rotateCoordinates(x_tmp,y_tmp,_rotation);
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
// map the point (x,y) from the linear source plane to the non-linear destination plane
// Returns false if calibration points are invalid or incorrectly ordered
bool XPT2046_HR2046_touch::mapPoint(float x, float y, Point_XPT2046_HR2046_touch &result)
{
	// Validate calibration point ordering
	// Expected order: [0]=top-left, [1]=top-right, [2]=bottom-left, [3]=bottom-right
	// Check if points form a proper quadrilateral
	if (src[0].x >= src[1].x || src[0].y >= src[2].y || // top-left constraints
		src[1].x <= src[0].x || src[1].y >= src[3].y || // top-right constraints
		src[2].x >= src[3].x || src[2].y <= src[0].y || // bottom-left constraints
		src[3].x <= src[2].x || src[3].y <= src[1].y)	// bottom-right constraints
	{
		return false; // Invalid calibration point ordering
	}

	// Validate dst points are not all zero (uncalibrated)
	if (dst[0].x == 0.0 && dst[0].y == 0.0 && dst[1].x == 0.0 && dst[1].y == 0.0 &&
		dst[2].x == 0.0 && dst[2].y == 0.0 && dst[3].x == 0.0 && dst[3].y == 0.0)
	{
		return false; // No calibration data
	}

	// Calculate the normalized position within the destination touch coordinate space
	// dst[0] = top-left touch coords, dst[1] = top-right touch coords
	// dst[2] = bottom-left touch coords, dst[3] = bottom-right touch coords

	// Calculate touch coordinate ranges
	float dst_width_top = dst[1].x - dst[0].x;	  // top edge width in touch coords
	float dst_width_bottom = dst[3].x - dst[2].x; // bottom edge width in touch coords
	float dst_height_left = dst[2].y - dst[0].y;  // left edge height in touch coords
	float dst_height_right = dst[3].y - dst[1].y; // right edge height in touch coords

	// Find the normalized position (u,v) within the touch coordinate quadrilateral
	// This is an inverse bilinear interpolation problem

	// For simplicity, we'll use the average method for now
	// Calculate u (horizontal position 0-1) by finding relative position along top/bottom edges
	float u = 0.5, v = 0.5; // default to center if calculation fails

	// Calculate u using top edge as reference
	if (abs(dst_width_top) > 1.0)
	{
		float u_top = (x - dst[0].x) / dst_width_top;
		u = u_top;
	}

	// Calculate v using left edge as reference
	if (abs(dst_height_left) > 1.0)
	{
		float v_left = (y - dst[0].y) / dst_height_left;
		v = v_left;
	}

	// Now map from normalized (u,v) to screen coordinates using src points
	// src[0] = top-left screen, src[1] = top-right screen
	// src[2] = bottom-left screen, src[3] = bottom-right screen

	// Bilinear interpolation in screen coordinate space
	// Top edge interpolation
	float top_x = (1.0 - u) * src[0].x + u * src[1].x;
	float top_y = (1.0 - u) * src[0].y + u * src[1].y;

	// Bottom edge interpolation
	float bottom_x = (1.0 - u) * src[2].x + u * src[3].x;
	float bottom_y = (1.0 - u) * src[2].y + u * src[3].y;

	// Final vertical interpolation
	result.x = (1.0 - v) * top_x + v * bottom_x;
	result.y = (1.0 - v) * top_y + v * bottom_y;

	return true;
}

/***************************************************************************************
** Function name:           rotateCoordinates
** Description:             Apply rotation transformation to coordinates based on display rotation
** Parameters:              x, y - coordinates to rotate (modified in place)
**                         rotation - rotation value (0=no rotation, 1=90°, 2=180°, 3=270°)
***************************************************************************************/
void XPT2046_HR2046_touch::rotateCoordinates(uint16_t &x, uint16_t &y, uint8_t rotation)
{
	uint16_t temp_x, temp_y;

	switch (rotation & 3) // Ensure rotation is 0-3
	{
	case 0: // No rotation
		// x and y remain unchanged
		break;

	case 3: // 90 degrees clockwise
		// (x,y) -> (height-1-y, x)
		temp_x = _height - 1 - y;
		temp_y = x;
		x = temp_x;
		y = temp_y;
		break;

	case 2: // 180 degrees
		// (x,y) -> (width-1-x, height-1-y)
		x = _width - 1 - x;
		y = _height - 1 - y;
		break;

	case 1: // 270 degrees clockwise (or 90 degrees counter-clockwise)
		// (x,y) -> (y, width-1-x)
		temp_x = y;
		temp_y = _width - 1 - x;
		x = temp_x;
		y = temp_y;
		break;
	}
}