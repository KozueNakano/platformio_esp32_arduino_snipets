// lcd_ctrl.hpp
#ifndef LCD_CTRL
#define LCD_CTRL
/*
  ST7032 I2C LCD control sample
  Original source : HelloWorld.ino (LiquidCrystal Library)
  2013/05/21 tomozh@gmail.com
*/

/*
  LiquidCrystal Library - Hello World

 Demonstrates the use a 16x2 LCD display.  The LiquidCrystal
 library works with all LCD displays that are compatible with the
 Hitachi HD44780 driver. There are many of them out there, and you
 can usually tell them by the 16-pin interface.

 This sketch prints "Hello World!" to the LCD
 and shows the time.

  The circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)

 Library originally added 18 Apr 2008
 by David A. Mellis
 library modified 5 Jul 2009
 by Limor Fried (http://www.ladyada.net)
 example added 9 Jul 2009
 by Tom Igoe
 modified 22 Nov 2010
 by Tom Igoe

 This example code is in the public domain.

 http://www.arduino.cc/en/Tutorial/LiquidCrystal
 */

// include the library code:
//#include <LiquidCrystal.h>
#include <Wire.h>
#include <ST7032.h>

enum stateNum
{
  NP,
  REST,
  DO
};

void setState(stateNum state);
void setNetStatus(int net);
void lcdShutdown(void);
void lcdInit(void);


#endif