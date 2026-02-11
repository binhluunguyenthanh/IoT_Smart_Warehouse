// src/hal/HalLCD.cpp
#include "hal/HalLCD.h"
#include <Wire.h>

// Constructor: Khởi tạo đối tượng thư viện LiquidCrystal_I2C
HalLCD::HalLCD() {
    hw_lcd = new LiquidCrystal_I2C(LCD_ADDRESS, LCD_COLS, LCD_ROWS);
}

// Khởi tạo phần cứng LCD
void HalLCD::init() {
    // --- CẤU HÌNH I2C ---
    // Quan trọng: Gán chân SDA, SCL cụ thể cho ESP32
    Wire.begin(PIN_LCD_SDA, PIN_LCD_SCL); 
    // ----------------------------------------------

    hw_lcd->init();      // Khởi động màn hình
    hw_lcd->backlight(); // Bật đèn nền
    showMessage("System Booting", "Please Wait..."); // Hiển thị màn hình chờ
}

// Hiển thị thông điệp lên 2 dòng
void HalLCD::showMessage(const char* line1, const char* line2) {
    hw_lcd->clear();          // Xóa màn hình cũ
    hw_lcd->setCursor(0, 0);  // Về đầu dòng 1
    hw_lcd->print(line1);     // In nội dung dòng 1
    hw_lcd->setCursor(0, 1);  // Xuống đầu dòng 2
    hw_lcd->print(line2);     // In nội dung dòng 2
}

// Hiển thị trạng thái (chỉ cập nhật dòng 2, giữ nguyên dòng 1)
void HalLCD::showStatus(const char* status) {
    // Cách 1: Xóa dòng 2 và ghi status mới (giữ nguyên dòng 1)
    hw_lcd->setCursor(0, 1); 
    hw_lcd->print("                "); // Xóa sạch dòng 2 bằng 16 khoảng trắng (giả sử LCD 1602)
    hw_lcd->setCursor(0, 1);
    hw_lcd->print(status);

    // Cách 2: Nếu bạn muốn clear toàn bộ thì dùng:
    // showMessage("Status:", status);
}