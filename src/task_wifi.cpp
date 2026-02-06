#include "task_wifi.h"

void startSTA()
{
    // Kiểm tra nếu chưa cấu hình WiFi thì thôi
    if (WIFI_SSID.isEmpty())
    {
        Serial.println("❌ Lỗi: Chưa có tên WiFi!");
        vTaskDelete(NULL);
    }

    WiFi.mode(WIFI_STA);

    // Bắt đầu kết nối
    if (WIFI_PASS.isEmpty())
    {
        WiFi.begin(WIFI_SSID.c_str());
    }
    else
    {
        WiFi.begin(WIFI_SSID.c_str(), WIFI_PASS.c_str());
    }

    Serial.print("Connecting WiFi");
    
    // Vòng lặp chờ kết nối
    int retry_count = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        vTaskDelay(500 / portTICK_PERIOD_MS);
        Serial.print(".");
        retry_count++;
        // Nếu chờ quá lâu (20s) thì báo lỗi
        if(retry_count > 40) {
            Serial.println("\n❌ Kết nối quá lâu! Kiểm tra lại Tên/Mật khẩu WiFi.");
            break;
        }
    }

    // --- ĐÂY LÀ ĐOẠN QUAN TRỌNG ĐỂ HIỆN IP ---
    if(WiFi.status() == WL_CONNECTED) {
        Serial.println("\n------------------------------------------------");
        Serial.println("✅ KẾT NỐI THÀNH CÔNG!");
        Serial.print("📡 ĐỊA CHỈ IP CỦA WEB: http://");
        Serial.println(WiFi.localIP()); 
        Serial.println("------------------------------------------------");
    }
    // ------------------------------------------

    // Báo hiệu cho các task khác là đã có mạng
    xSemaphoreGive(xBinarySemaphoreInternet);
}

bool Wifi_reconnect()
{
    const wl_status_t status = WiFi.status();
    if (status == WL_CONNECTED)
    {
        return true;
    }
    startSTA();
    return false;
}