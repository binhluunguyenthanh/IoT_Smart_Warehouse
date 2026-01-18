// src/hal/HalLCD.cpp
#include "hal/HalLCD.h"

HalLCD::HalLCD() {
    hw_lcd = new LiquidCrystal_I2C(LCD_ADDRESS, LCD_COLS, LCD_ROWS);
}

void HalLCD::init() {
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

// --- THÊM ĐOẠN NÀY VÀO ---
void HalLCD::showStatus(const char* status) {
    // Cách 1: Xóa dòng 2 và ghi status mới (giữ nguyên dòng 1)
    hw_lcd->setCursor(0, 1); 
    hw_lcd->print("                "); // Xóa sạch dòng 2 bằng 16 khoảng trắng (giả sử LCD 1602)
    hw_lcd->setCursor(0, 1);
    hw_lcd->print(status);

    // Cách 2: Nếu bạn muốn clear toàn bộ thì dùng:
    // showMessage("Status:", status);
}