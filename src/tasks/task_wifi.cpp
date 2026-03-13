#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include "global.h" 

const char* ssid       = "";
const char* password   = "";

const char* ntpServer  = "pool.ntp.org";
const long  gmtOffset_sec = 7 * 3600; // GMT+7 cho Việt Nam
const int   daylightOffset_sec = 0;

void TaskWifiTimeFunc(void *pvParameters) {
    Serial.print("[WIFI] Dang ket noi den ");
    Serial.println(ssid);
    
    WiFi.begin(ssid, password);
    
    // Chờ kết nối
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(500 / portTICK_PERIOD_MS);
        Serial.print(".");
    }
    Serial.println("\n[WIFI] Da ket noi! Dang lay thoi gian...");

    // Khởi tạo và lấy thời gian thực từ Internet (NTP)
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    vTaskDelete(NULL); 
}