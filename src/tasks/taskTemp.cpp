#include "tasks/TaskTemp.h"
#include "global.h"
#include "time.h" // Thư viện đọc RTC

DHT dht(DHT_PIN, DHT_TYPE); 

void TaskTempFunc(void *pvParameters) {
    dht.begin();
    Serial.println("[TEMP] Task Started");

    for (;;) {
        // 1. Đọc dữ liệu cảm biến
        float h = dht.readHumidity();
        float t = dht.readTemperature();

        if (isnan(h) || isnan(t)) {
            Serial.println("[TEMP] Failed to read DHT sensor!");
        } else {
            // Cập nhật biến toàn cục 
            currentTemperature = t;
            currentHumidity = h;
            
            // 2. Gửi dữ liệu Sensor lên Gateway (Bọc Mutex)
            if (xSemaphoreTake(xSerialMutex, portMAX_DELAY) == pdTRUE) {
                Serial.print("!1:" + String(t, 1) + "#"); 
                Serial.print("!2:" + String(h, 0) + "#");
                xSemaphoreGive(xSerialMutex);
            }

            // ==========================================
            // 3. VẼ DASHBOARD LÊN LCD (KHÔNG DÙNG CLEAR)
            // ==========================================
            struct tm timeinfo;
            bool hasTime = getLocalTime(&timeinfo, 0); 

            char line1[17]; 
            char line2[17];

            // Format dòng 1: "25.5*C 60% 82%   " (3 khoảng trắng cuối để xóa chữ rác)
            sprintf(line1, "T:%.1f*C H:%.0f%%", t, h); 

            // Format dòng 2: "13/03/26 14:30  "
            if (hasTime) {
                strftime(line2, sizeof(line2), "%d/%m/%y %H:%M  ", &timeinfo);
            } else {
                sprintf(line2, "Time Syncing... ");
            }

            // Xin quyền vẽ lên LCD
            if (xSemaphoreTake(xLcdMutex, portMAX_DELAY) == pdTRUE) {
                myLCD.moveTo(0, 0);
                myLCD.putStr(line1);
                
                myLCD.moveTo(0, 1);
                myLCD.putStr(line2);
                
                xSemaphoreGive(xLcdMutex);
            }
        }

        // Đọc và vẽ lại màn hình định kỳ mỗi 8 giây
        vTaskDelay(8000 / portTICK_PERIOD_MS);
    }
}