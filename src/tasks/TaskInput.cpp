#include "tasks/TaskInput.h"
#include "global.h"

void TaskInputFunc(void *pvParameters) {
    InputTaskParams* params = (InputTaskParams*)pvParameters;
    QueueHandle_t outQ = params->outputQueue;
    HalRFID* rfid = params->rfidReader;

    SystemMessage msg;

    // Khởi tạo RFID trong Task này (vì liên quan đến bus SPI của Task này)
    rfid->init();
    Serial.println("[INPUT] Ready: Scan RFID or Type Serial...");

    for (;;) {
        // 1. KIỂM TRA RFID (PHẦN CỨNG THẬT)
        if (rfid->checkTag()) {
            String uid = rfid->getTagUID();
            
            // Đóng gói tin nhắn
            msg.type = EVENT_SCAN_RFID;
            // Copy chuỗi UID vào payload (sử dụng strncpy để an toàn bộ nhớ đệm)
            strncpy(msg.payload, uid.c_str(), sizeof(msg.payload) - 1);
            msg.payload[sizeof(msg.payload) - 1] = '\0'; // Đảm bảo kết thúc chuỗi

            // Gửi vào Queue để Manager xử lý
            xQueueSend(outQ, &msg, 10);
            Serial.println("[INPUT] RFID Detected: " + uid);
            
            // Delay 1 giây để tránh đọc 1 thẻ nhiều lần liên tiếp
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }

        // 2. KIỂM TRA SERIAL (MÔ PHỎNG - Dùng để test khi không có thẻ thật)
        if (Serial.available()) {
            String input = Serial.readStringUntil('\n');
            input.trim(); // Xóa khoảng trắng thừa
            if (input.length() > 0) {
                msg.type = EVENT_SCAN_RFID;
                strncpy(msg.payload, input.c_str(), sizeof(msg.payload) - 1);
                
                xQueueSend(outQ, &msg, 10);
                Serial.println("[INPUT] Serial Command: " + input);
            }
        }

        // Nghỉ 100ms để nhường CPU
        vTaskDelay(100 / portTICK_PERIOD_MS); 
    }
}