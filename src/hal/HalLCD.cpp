#include "hal/HalLCD.h"
#include <Wire.h>

HalLCD::HalLCD() {
    hw_lcd = new LiquidCrystal_I2C(LCD_ADDRESS, LCD_COLS, LCD_ROWS);
}

void HalLCD::init() {
    Wire.begin(PIN_LCD_SDA, PIN_LCD_SCL); 
    hw_lcd->init();      
    hw_lcd->backlight(); 
    showMessage("System Booting", "Please Wait..."); 
}

void HalLCD::showMessage(const char* line1, const char* line2) {
    hw_lcd->clear();          
    hw_lcd->setCursor(0, 0);  
    hw_lcd->print(line1);     
    hw_lcd->setCursor(0, 1);  
    hw_lcd->print(line2);     
}

// --- Design màn hình LCD ---

void HalLCD::clear() {
    hw_lcd->clear();
}

void HalLCD::moveTo(uint8_t x, uint8_t y) {
    hw_lcd->setCursor(x, y);
}

void HalLCD::putStr(const String& str) {
    hw_lcd->print(str);
}

void HalLCD::putStr(int num) {
    hw_lcd->print(num);
}

void HalLCD::putStr(float num, int decimalPlaces) {
    hw_lcd->print(num, decimalPlaces);
}