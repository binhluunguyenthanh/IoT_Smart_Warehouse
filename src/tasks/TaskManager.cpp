#include "tasks/TaskManager.h"
#include "config/SystemConfig.h"
#include "global.h"

void TaskManagerFunc(void *pvParameters) {
    ManagerTaskParams* params = (ManagerTaskParams*)pvParameters;
    QueueHandle_t inputQ = params->inputQueue;     
    SystemMessage msgIn;

    for (;;) {
        if (xQueueReceive(inputQ, &msgIn, 10) == pdTRUE) {
            if (msgIn.type == EVENT_SCAN_RFID) {    
                String uidHex = String(msgIn.payload);
                // Bắn thẳng mã RFID qua Serial cho Python Gateway (Feed ID: 3)
                Serial.print("!3:" + uidHex + "#");
            }
        }
        vTaskDelay(10);
    }
}