
#include "tasks/TaskDisplay.h"
#include "config/SystemConfig.h"

void TaskDisplayFunc(void *pvParameters) {
    DisplayTaskParams* params = (DisplayTaskParams*)pvParameters;
    QueueHandle_t inQ = params->inputQueue;
    HalLCD* lcd = params->lcdScreen;

    SystemMessage msg;

    for (;;) {
        // Chờ nhận lệnh hiển thị (Block vô thời hạn cho đến khi có tin nhắn)
        if (xQueueReceive(inQ, &msg, portMAX_DELAY) == pdTRUE) {
            
            if (msg.type == EVENT_UPDATE_LCD) {
                // msg.payload chứa nội dung dòng 2 (VD: "Item: Drill")
                // Dòng 1 giữ nguyên hoặc set cố định
                lcd->showMessage("Smart Warehouse", msg.payload);
            }
            else if (msg.type == EVENT_SYNC_CLOUD) {
                lcd->showStatus("Syncing Cloud...");
            }
        }
    }
}