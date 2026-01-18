#include "tasks/TaskInput.h"
#include "config/SystemConfig.h"

void TaskInputFunc(void *pvParameters) {
    InputTaskParams* params = (InputTaskParams*)pvParameters;
    QueueHandle_t outQ = params->outputQueue;
    HalRFID* rfid = params->rfidReader;

    SystemMessage msg;

    // init RFID trong Task này (vì liên quan đến SPI bus)
    rfid->init();
    Serial.println("[INPUT] Ready: Scan RFID or Type Serial...");

    for (;;) {
        // 1. ƯU TIÊN KIỂM TRA RFID (PHẦN CỨNG THẬT)
        if (rfid->checkTag()) {
            String uid = rfid->getTagUID();
            
            msg.type = EVENT_SCAN_RFID;
            // Copy chuỗi UID vào payload (an toàn bộ nhớ)
            strncpy(msg.payload, uid.c_str(), sizeof(msg.payload) - 1);
            msg.payload[sizeof(msg.payload) - 1] = '\0'; // Null terminate

            xQueueSend(outQ, &msg, 10);
            Serial.println("[INPUT] RFID Detected: " + uid);
            
            // Delay nhẹ để tránh đọc trùng lặp quá nhanh
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }

        // 2. KIỂM TRA SERIAL (MÔ PHỎNG WOKWI)
        if (Serial.available()) {
            String input = Serial.readStringUntil('\n');
            input.trim();
            if (input.length() > 0) {
                if (input == "SYNC") {
                    msg.type = EVENT_SYNC_CLOUD;
                } else {
                    msg.type = EVENT_SCAN_RFID;
                    strncpy(msg.payload, input.c_str(), sizeof(msg.payload) - 1);
                }
                
                xQueueSend(outQ, &msg, 10);
                Serial.println("[INPUT] Serial Command: " + input);
            }
        }

        vTaskDelay(100 / portTICK_PERIOD_MS); // Nghỉ 100ms
    }
}