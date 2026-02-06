#include "tasks/TaskTemp.h"
#include "global.h"
#include <DHT.h>

#define DHTPIN 16      // Chân nối cảm biến
#define DHTTYPE DHT22  /**< DHT TYPE 22 */

DHT dht(DHTPIN, DHTTYPE);

void TaskTempFunc(void *pvParameters) {
    dht.begin();
    Serial.println("[TEMP] Task Started");

    for (;;) {
        // Đọc dữ liệu (mất khoảng 250ms)
        float h = dht.readHumidity();
        float t = dht.readTemperature();

        // Kiểm tra lỗi
        if (isnan(h) || isnan(t)) {
            Serial.println("[TEMP] Failed to read DHT sensor!");
        } else {
            // Cập nhật biến toàn cục
            currentTemperature = t;
            currentHumidity = h;
            
            // Debug chơi
            // Serial.print("Temp: "); Serial.print(t);
            // Serial.print(" | Hum: "); Serial.println(h);
        }

        // DHT11 lấy mẫu chậm, nên delay ít nhất 2 giây
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}