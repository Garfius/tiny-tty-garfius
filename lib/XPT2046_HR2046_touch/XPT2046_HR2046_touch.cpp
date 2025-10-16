#include <XPT2046_HR2046_touch.h>
// The following touch screen support code by maxpautsch was merged 1/10/17
// https://github.com/maxpautsch

// Define TOUCH_CS is the user setup file to enable this code

// A demo is provided in examples Generic folder

// Additions by Bodmer to double sample, use Z value to improve detection reliability
// and to correct rotation handling

// See license in root directory.

// Define a default pressure threshold
#ifndef Z_THRESHOLD
  #define Z_THRESHOLD 350 // Touch pressure threshold for validating touches
#endif
#define SPI_SETTING SPISettings(SPI_TOUCH_FREQUENCY, MSBFIRST, SPI_MODE0)
/***************************************************************************************
** Function name:           getTouchRaw
** Description:             read raw touch position.  Always returns true.
***************************************************************************************/
uint8_t XPT2046_HR2046_touch::getTouchRaw(uint16_t *x, uint16_t *y){
  uint16_t tmp;

  _spi->beginTransaction(SPI_SETTING);digitalWrite(_csPin, LOW);
  
  // Start YP sample request for x position, read 4 times and keep last sample
  _spi->transfer(0xd0);                    // Start new YP conversion
  _spi->transfer(0);                       // Read first 8 bits
  _spi->transfer(0xd0);                    // Read last 8 bits and start new YP conversion
  _spi->transfer(0);                       // Read first 8 bits
  _spi->transfer(0xd0);                    // Read last 8 bits and start new YP conversion
  _spi->transfer(0);                       // Read first 8 bits
  _spi->transfer(0xd0);                    // Read last 8 bits and start new YP conversion

  tmp = _spi->transfer(0);                   // Read first 8 bits
  tmp = tmp <<5;
  tmp |= 0x1f & (_spi->transfer(0x90)>>3);   // Read last 8 bits and start new XP conversion

  *x = tmp;

  // Start XP sample request for y position, read 4 times and keep last sample
  _spi->transfer(0);                       // Read first 8 bits
  _spi->transfer(0x90);                    // Read last 8 bits and start new XP conversion
  _spi->transfer(0);                       // Read first 8 bits
  _spi->transfer(0x90);                    // Read last 8 bits and start new XP conversion
  _spi->transfer(0);                       // Read first 8 bits
  _spi->transfer(0x90);                    // Read last 8 bits and start new XP conversion

  tmp = _spi->transfer(0);                 // Read first 8 bits
  tmp = tmp <<5;
  tmp |= 0x1f & (_spi->transfer(0)>>3);    // Read last 8 bits

  *y = tmp;

  digitalWrite(_csPin, HIGH);  _spi->endTransaction();

  return true;
}

/***************************************************************************************
** Function name:           getTouchRawZ
** Description:             read raw pressure on touchpad and return Z value. 
***************************************************************************************/
uint16_t XPT2046_HR2046_touch::getTouchRawZ(void){

  _spi->beginTransaction(SPI_SETTING);digitalWrite(_csPin, LOW);

  // Z sample request
  int16_t tz = 0xFFF;
  _spi->transfer(0xb0);               // Start new Z1 conversion
  tz += _spi->transfer16(0xc0) >> 3;  // Read Z1 and start Z2 conversion
  tz -= _spi->transfer16(0x00) >> 3;  // Read Z2

  digitalWrite(_csPin, HIGH);  _spi->endTransaction();

  if (tz == 4095) tz = 0;

  return (uint16_t)tz;
}

/***************************************************************************************
** Function name:           validTouch
** Description:             read validated position. Return false if not pressed. 
***************************************************************************************/
#define _RAWERR 20 // Deadband error allowed in successive position samples
uint8_t XPT2046_HR2046_touch::validTouch(uint16_t *x, uint16_t *y, uint16_t threshold){
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

  if (z1 <= threshold) return false;
    
  getTouchRaw(&x_tmp,&y_tmp);

  //  Serial.print("Sample 1 x,y = "); Serial.print(x_tmp);Serial.print(",");Serial.print(y_tmp);
  //  Serial.print(", Z = ");Serial.println(z1);

  delay(1); // Small delay to the next sample
  if (getTouchRawZ() <= threshold) return false;

  delay(2); // Small delay to the next sample
  getTouchRaw(&x_tmp2,&y_tmp2);
  
  //  Serial.print("Sample 2 x,y = "); Serial.print(x_tmp2);Serial.print(",");Serial.println(y_tmp2);
  //  Serial.print("Sample difference = ");Serial.print(abs(x_tmp - x_tmp2));Serial.print(",");Serial.println(abs(y_tmp - y_tmp2));

  if (abs(x_tmp - x_tmp2) > _RAWERR) return false;
  if (abs(y_tmp - y_tmp2) > _RAWERR) return false;
  
  *x = x_tmp;
  *y = y_tmp;
  
  return true;
}
  
/***************************************************************************************
** Function name:           getTouch
** Description:             read callibrated position. Return false if not pressed. 
***************************************************************************************/
uint8_t XPT2046_HR2046_touch::getTouch(uint16_t *x, uint16_t *y, uint16_t threshold){
  uint16_t x_tmp, y_tmp;
  
  if (threshold<20) threshold = 20;
  if (_pressTime > millis()) threshold=20;

  uint8_t n = 5;
  uint8_t valid = 0;
  while (n--)
  {
    if (validTouch(&x_tmp, &y_tmp, threshold)) valid++;;
  }

  if (valid<1) { _pressTime = 0; return false; }
  
  _pressTime = millis() + 50;

  convertRawXY(&x_tmp, &y_tmp);

  if (x_tmp >= _width || y_tmp >= _height) return false;

  _pressX = x_tmp;
  _pressY = y_tmp;
  *x = _pressX;
  *y = _pressY;
  return valid;
}

/***************************************************************************************
** Function name:           convertRawXY
** Description:             convert raw touch x,y values to screen coordinates 
***************************************************************************************/
void XPT2046_HR2046_touch::convertRawXY(uint16_t *x, uint16_t *y)
{
  uint16_t x_tmp = *x, y_tmp = *y, xx, yy;

  if(!touchCalibration_rotate){
    xx=(x_tmp-touchCalibration_x0)*_width/touchCalibration_x1;
    yy=(y_tmp-touchCalibration_y0)*_height/touchCalibration_y1;
    if(touchCalibration_invert_x)
      xx = _width - xx;
    if(touchCalibration_invert_y)
      yy = _height - yy;
  } else {
    xx=(y_tmp-touchCalibration_x0)*_width/touchCalibration_x1;
    yy=(x_tmp-touchCalibration_y0)*_height/touchCalibration_y1;
    if(touchCalibration_invert_x)
      xx = _width - xx;
    if(touchCalibration_invert_y)
      yy = _height - yy;
  }
  *x = xx;
  *y = yy;
}

/***************************************************************************************
** Function name:           setTouch
** Description:             imports calibration parameters for touchscreen. 
***************************************************************************************/
void XPT2046_HR2046_touch::setTouch(uint16_t *parameters){
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
void XPT2046_HR2046_touch::begin(uint8_t  touchCalibration_rotate ,uint8_t   touchCalibration_invert_x,uint8_t   touchCalibration_invert_y){
    this->touchCalibration_rotate = touchCalibration_rotate;
    this->touchCalibration_invert_x = touchCalibration_invert_x;
    this->touchCalibration_invert_y = touchCalibration_invert_y;
    _spi->begin();
	pinMode(_csPin, OUTPUT);
	digitalWrite(_csPin, HIGH);
}
void XPT2046_HR2046_touch::setCalibrationData(uint16_t *parameters){
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
