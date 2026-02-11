#include "hal/HalRFID.h"

// Constructor: Cấu hình chân CS (SDA) và RST cho module RC522
HalRFID::HalRFID() {
    mfrc522 = new MFRC522(PIN_RFID_SDA, PIN_RFID_RST);
}

// Khởi tạo giao tiếp SPI và Module RFID
void HalRFID::init() {
    SPI.begin();         // Khởi động bus SPI
    mfrc522->PCD_Init(); // Khởi động chip RC522
    // Có thể chỉnh gain (độ nhạy) ăng ten nếu cần:
    // mfrc522->PCD_SetAntennaGain(mfrc522->RxGain_max);
}

// Kiểm tra xem có thẻ hợp lệ không
bool HalRFID::checkTag() {
    // Điều kiện: Phải có thẻ mới đặt vào VÀ đọc được dữ liệu thẻ đó
    if (!mfrc522->PICC_IsNewCardPresent() || !mfrc522->PICC_ReadCardSerial()) {
        return false;
    }
    return true;
}

// Lấy mã UID của thẻ và chuyển sang dạng chuỗi HEX 
String HalRFID::getTagUID() {
    String uidString = "";
    for (byte i = 0; i < mfrc522->uid.size; i++) {
        if (mfrc522->uid.uidByte[i] < 0x10) {
            uidString += "0"; // Thêm số 0 đằng trước nếu số hex chỉ có 1 ký tự 
        }
        uidString += String(mfrc522->uid.uidByte[i], HEX);
    }
    uidString.toUpperCase(); // Chuyển thành chữ hoa cho đẹp
    
    // Ngắt giao tiếp với thẻ hiện tại để tránh đọc lại liên tục thẻ cũ
    mfrc522->PICC_HaltA();
    mfrc522->PCD_StopCrypto1();
    
    return uidString;
}