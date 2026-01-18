// include/hal/HalLCD.h
#ifndef HAL_LCD_H
#define HAL_LCD_H
#include <LiquidCrystal_I2C.h>
#include "config/SystemConfig.h"

class HalLCD {
private:
    LiquidCrystal_I2C* hw_lcd;
public:
    HalLCD();
    void init();
    void showMessage(const char* line1, const char* line2);
    void showStatus(const char* status);
};
#endif