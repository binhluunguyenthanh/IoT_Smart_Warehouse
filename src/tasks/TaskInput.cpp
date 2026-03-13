#include "tasks/TaskInput.h"
#include "global.h"

void TaskInputFunc(void *pvParameters) {
    InputTaskParams* params = (InputTaskParams*)pvParameters;
    QueueHandle_t outQ = params->outputQueue;
    HalRFID* rfid = params->rfidReader;

    SystemMessage msg;

    rfid->init();
    Serial.println("[INPUT] Ready: Scan RFID or Type Serial...");

    for (;;) {
        // 1. KIỂM TRA RFID (PHẦN CỨNG THẬT)
        if (rfid->checkTag()) {
            String uid = rfid->getTagUID();
            
            msg.type = EVENT_SCAN_RFID;
            strncpy(msg.payload, uid.c_str(), sizeof(msg.payload) - 1);
            msg.payload[sizeof(msg.payload) - 1] = '\0'; 

            xQueueSend(outQ, &msg, 10);
            vTaskDelay(1000 / portTICK_PERIOD_MS); // Chống kẹt thẻ
        }

        // 2. ĐỌC SERIAL TỪ GATEWAY
        if (Serial.available() > 0) {
            String input = Serial.readStringUntil('#');
            int colonIndex = input.indexOf(':');
            
            if (input.startsWith("!") && colonIndex != -1) {
                String idStr = input.substring(1, colonIndex); 
                String valStr = input.substring(colonIndex + 1); 
                
                // NẾU GATEWAY YÊU CẦU ĐỔI MODE (ID 4)
                if (idStr == "4") {
                    currentSystemMode = valStr.toInt();
                    updateLeds(); 
                    if (xSemaphoreTake(xLcdMutex, portMAX_DELAY) == pdTRUE) {
                        myLCD.clear(); 
                        
                        myLCD.moveTo(0, 0);
                        myLCD.putStr("SYSTEM MODE:");
                        
                        myLCD.moveTo(0, 1);
                        if (currentSystemMode == 0) myLCD.putStr("-> CHECKING");
                        else if (currentSystemMode == 1) myLCD.putStr("-> IMPORT");
                        else if (currentSystemMode == 2) myLCD.putStr("-> EXPORT");
                        vTaskDelay(2000 / portTICK_PERIOD_MS); 
                        xSemaphoreGive(xLcdMutex);
                    }
                    if (xSemaphoreTake(xSerialMutex, portMAX_DELAY) == pdTRUE) {
                        Serial.print("!4:" + String(currentSystemMode) + "#"); 
                        xSemaphoreGive(xSerialMutex);
                    }
                }
                
                // NẾU GATEWAY YÊU CẦU HIỂN THỊ POP-UP (ID 5)
                else if (idStr == "5") {
                   int splitIndex = valStr.indexOf('|');
                   String line1 = "";
                   String line2 = "";
                   
                   if (splitIndex != -1) {
                       line1 = valStr.substring(0, splitIndex);
                       line2 = valStr.substring(splitIndex + 1);
                   } else {
                       line1 = "System Msg:";
                       line2 = valStr;
                   }

                   // Vẽ Pop-up lên màn hình
                   if (xSemaphoreTake(xLcdMutex, portMAX_DELAY) == pdTRUE) {
                       myLCD.clear(); // DUY NHẤT Ở ĐÂY ĐƯỢC DÙNG CLEAR
                       
                       myLCD.moveTo(0, 0);
                       myLCD.putStr(line1);
                       
                       myLCD.moveTo(0, 1);
                       myLCD.putStr(line2);
                       
                       xSemaphoreGive(xLcdMutex);
                   }
                }
            }
        }
        
        vTaskDelay(50 / portTICK_PERIOD_MS); // Cho CPU nghỉ ngơi
    }
}