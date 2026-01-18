#include "hal/HalRFID.h"

HalRFID::HalRFID() {
    // Khởi tạo đối tượng MFRC522 với chân SS và RST định nghĩa trong SystemConfig
    mfrc522 = new MFRC522(PIN_RFID_SDA, PIN_RFID_RST);
}

void HalRFID::init() {
    SPI.begin();        // Init SPI bus
    mfrc522->PCD_Init(); // Init MFRC522
    // Có thể chỉnh gain (độ nhạy) ăng ten nếu cần:
    // mfrc522->PCD_SetAntennaGain(mfrc522->RxGain_max);
}

bool HalRFID::checkTag() {
    // Phải có thẻ mới VÀ đọc được thẻ đó
    if (!mfrc522->PICC_IsNewCardPresent() || !mfrc522->PICC_ReadCardSerial()) {
        return false;
    }
    return true;
}

String HalRFID::getTagUID() {
    String uidString = "";
    for (byte i = 0; i < mfrc522->uid.size; i++) {
        if (mfrc522->uid.uidByte[i] < 0x10) {
            uidString += "0"; // Thêm số 0 nếu hex chỉ có 1 ký tự
        }
        uidString += String(mfrc522->uid.uidByte[i], HEX);
    }
    uidString.toUpperCase();
    
    // Halt thẻ để không đọc lại liên tục thẻ cũ
    mfrc522->PICC_HaltA();
    mfrc522->PCD_StopCrypto1();
    
    return uidString;
}