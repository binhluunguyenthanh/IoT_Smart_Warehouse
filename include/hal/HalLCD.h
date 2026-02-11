// include/hal/HalLCD.h
#ifndef HAL_LCD_H
#define HAL_LCD_H
#include <LiquidCrystal_I2C.h>
#include "config/SystemConfig.h"

// -------------------------------------------------------------------------
// LỚP HAL LCD
// Quản lý việc hiển thị thông tin lên màn hình LCD qua giao tiếp I2C
// -------------------------------------------------------------------------
class HalLCD {
private:
    LiquidCrystal_I2C* hw_lcd; // Con trỏ tới đối tượng thư viện phần cứng thực tế
public:
    // Hàm khởi tạo: Cấp phát bộ nhớ cho đối tượng LCD
    HalLCD(); 

    // Khởi tạo phần cứng: Bật màn hình, bật đèn nền
    void init();

    // Hiển thị thông điệp tùy chỉnh trên 2 dòng
    // line1: Nội dung dòng trên
    // line2: Nội dung dòng dưới
    void showMessage(const char* line1, const char* line2);

    // Hiển thị trạng thái hệ thống (Thường dùng ở dòng 2 hoặc thông báo ngắn)
    void showStatus(const char* status);
};
#endif