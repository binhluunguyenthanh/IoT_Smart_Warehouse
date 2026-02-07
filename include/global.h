#ifndef GLOBAL_H
#define GLOBAL_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "app/inventory.h"
#include "config/SystemConfig.h"

extern String WIFI_SSID;
extern String WIFI_PASS;
extern String CORE_IOT_TOKEN;
extern String CORE_IOT_SERVER;
extern String CORE_IOT_PORT;

// --- DATA TYPES ---
enum EventType {
    EVENT_IDLE,         // Trạng thái nhàn rỗi
    EVENT_SCAN_RFID,    // Đọc thẻ RFID
    EVENT_UPDATE_LCD,   // Cập nhật hiển thị LCD
    EVENT_SYNC_CLOUD,   // Đồng bộ dữ liệu lên Cloud
};

struct SystemMessage {
    EventType type;
    char payload[32];   
    int value;
};

extern InventoryManager* myWarehouse;
// Khóa bảo vệ kho hàng
extern SemaphoreHandle_t xInventoryMutex; 

extern String lastScannedRFID;
extern bool hasNewTag;
extern int currentSystemMode; // 0: Check, 1: Import, 2: Export
extern float currentTemperature;
extern float currentHumidity;
// --- THÊM BIẾN AI ---
extern float anomalyScore;  // Điểm bất thường (0.0 -> 1.0)
extern bool isAnomaly;      // Cờ báo động: True = Có vấn đề
extern void updateLeds();
extern SemaphoreHandle_t xBinarySemaphoreInternet;
extern QueueHandle_t g_queueManager_to_LCD;
#endif