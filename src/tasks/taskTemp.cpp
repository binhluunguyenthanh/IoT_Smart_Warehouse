#include "tasks/TaskTemp.h"

DHT dht(DHT_PIN, DHT_TYPE); // Khởi tạo đối tượng DHT

void TaskTempFunc(void *pvParameters) {
    dht.begin();
    Serial.println("[TEMP] Task Started");

    for (;;) {
        // Đọc dữ liệu (quá trình này mất khoảng 250ms)
        float h = dht.readHumidity();
        float t = dht.readTemperature();

        // Kiểm tra xem đọc có thành công không
        if (isnan(h) || isnan(t)) {
            Serial.println("[TEMP] Failed to read DHT sensor!");
        } else {
            // Cập nhật biến toàn cục để WebServer sử dụng
            currentTemperature = t;
            currentHumidity = h;
            
            // Debug chơi (đã comment lại)
            // Serial.print("Temp: "); Serial.print(t);
            // Serial.print(" | Hum: "); Serial.println(h);
        }

        // Đọc định kỳ mỗi 2 giây
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}