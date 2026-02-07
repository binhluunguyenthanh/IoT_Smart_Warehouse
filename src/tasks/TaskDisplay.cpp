#include "tasks/TaskDisplay.h"
#include "global.h"

void TaskDisplayFunc(void *pvParameters) {
    DisplayTaskParams* params = (DisplayTaskParams*)pvParameters;
    QueueHandle_t inQ = params->inputQueue;
    HalLCD* lcd = params->lcdScreen;

    SystemMessage msg;

    for (;;) {
        if (xQueueReceive(inQ, &msg, portMAX_DELAY) == pdTRUE) {
            
            if (msg.type == EVENT_UPDATE_LCD) {
                String data = String(msg.payload);
                
                // Tách chuỗi bằng dấu "|"
                int splitIndex = data.indexOf('|');
                
                if (splitIndex != -1) {
                    String line1 = data.substring(0, splitIndex);
                    String line2 = data.substring(splitIndex + 1);
                    
                    // --- SỬA Ở ĐÂY: Thêm .c_str() ---
                    lcd->showMessage(line1.c_str(), line2.c_str());
                } else {
                    // --- SỬA Ở ĐÂY: Thêm .c_str() ---
                    lcd->showMessage("Scanned:", data.c_str());
                }
            }
            else if (msg.type == EVENT_SYNC_CLOUD) {
                lcd->showStatus("Syncing Cloud...");
            }
        }
    }
}