#ifndef HAL_RFID_H
#define HAL_RFID_H

#include <SPI.h>
#include <MFRC522.h>
#include "config/SystemConfig.h"

// -------------------------------------------------------------------------
// LỚP HAL RFID
// Quản lý giao tiếp SPI với module MFRC522 để đọc thẻ từ
// -------------------------------------------------------------------------
class HalRFID {
private:
    MFRC522* mfrc522; // Con trỏ tới đối tượng thư viện MFRC522
public:
    // Hàm khởi tạo: Cấu hình chân CS và RST
    HalRFID();

    // Khởi tạo giao tiếp SPI và module RFID
    void init();

    // Kiểm tra xem có thẻ mới được đặt vào hay không
    // Trả về: true nếu có thẻ, false nếu không
    bool checkTag();

    // Đọc và trả về mã UID của thẻ dưới dạng String (HEX)
    String getTagUID();
};

#endif