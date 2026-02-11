#include "tasks/TaskDisplay.h"
#include "global.h"

void TaskDisplayFunc(void *pvParameters) {
    // Ép kiểu tham số truyền vào để lấy Queue và LCD Object
    DisplayTaskParams* params = (DisplayTaskParams*)pvParameters;
    QueueHandle_t inQ = params->inputQueue;
    HalLCD* lcd = params->lcdScreen;

    SystemMessage msg;

    // Vòng lặp vô tận xử lý hiển thị
    for (;;) {
        // Chờ nhận dữ liệu từ Queue (Block vô thời hạn cho đến khi có tin nhắn)
        if (xQueueReceive(inQ, &msg, portMAX_DELAY) == pdTRUE) {
            
            if (msg.type == EVENT_UPDATE_LCD) {
                String data = String(msg.payload);
                
                // Tách chuỗi payload bằng dấu "|" (Quy ước: Dòng1|Dòng2)
                int splitIndex = data.indexOf('|');
                
                if (splitIndex != -1) {
                    String line1 = data.substring(0, splitIndex);
                    String line2 = data.substring(splitIndex + 1);
                    
                    // Hiển thị lên LCD (chuyển String về const char*)
                    lcd->showMessage(line1.c_str(), line2.c_str());
                } else {
                    // Nếu không có dấu |, mặc định hiển thị dòng 2
                    lcd->showMessage("Scanned:", data.c_str());
                }
            }
        }
    }
}