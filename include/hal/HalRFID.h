#ifndef HAL_RFID_H
#define HAL_RFID_H

#include <SPI.h>
#include <MFRC522.h>
#include "config/SystemConfig.h"

class HalRFID {
private:
    MFRC522* mfrc522; // Con trỏ tới đối tượng thư viện
public:
    HalRFID();
    void init();
    // Hàm kiểm tra xem có thẻ mới không
    bool checkTag();
    // Hàm lấy UID của thẻ dưới dạng chuỗi (VD: "E2 A3 4F")
    String getTagUID();
};

#endif