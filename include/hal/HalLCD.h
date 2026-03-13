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

    // --- CÁC HÀM MỚI BỔ SUNG ĐỂ KIỂM SOÁT TỌA ĐỘ ---
    
    // Xóa toàn bộ màn hình
    void clear(); 
    
    // Di chuyển con trỏ tới tọa độ (cột x, hàng y)
    // x: 0 đến 15 (cột)
    // y: 0 hoặc 1 (hàng)
    void moveTo(uint8_t x, uint8_t y); 
    
    // In chuỗi (String, số nguyên, số thực) tại vị trí hiện tại
    void putStr(const String& str);
    void putStr(int num);
    void putStr(float num, int decimalPlaces = 1);
};
#endif