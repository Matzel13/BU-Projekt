#include <TinyWireM.h>                 
#include <TinyLiquidCrystal_I2C.h>         

TinyLiquidCrystal_I2C lcd(0x27,16,2);  // set address & 16 chars / 2 lines

void setup()
{

  TinyWireM.begin();                    // initialize I2C lib
  lcd.init();                           // initialize the lcd 
  lcd.backlight(); 
  lcd.clear();  // Print a message to the LCD.
}

void loop()
{
  lcd.setCursor(0, 0);
  lcd.print("Hello World on Attiny85");
  
}