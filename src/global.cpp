#include "global.h"

// --- BIẾN CẤU HÌNH ---
String WIFI_SSID = "THD";
String WIFI_PASS = "hcmutk23@";
String CORE_IOT_TOKEN = "";
String CORE_IOT_SERVER = "192.168.1.5";
String CORE_IOT_PORT = "1883";

// --- QUẢN LÝ KHO ---
InventoryManager* myWarehouse = new InventoryManager();
SemaphoreHandle_t xInventoryMutex = xSemaphoreCreateMutex(); // Tạo khóa

// --- TRẠNG THÁI ---
String lastScannedRFID = "";
bool hasNewTag = false;
int currentSystemMode = 0; // 0: Check, 1: Import, 2: Export
// --- CẢM BIẾN NHIỆT ĐỘ & ĐỘ ẨM ---
float currentTemperature = 0.0;
float currentHumidity = 0.0;
// --- THÊM KHỞI TẠO ---
float anomalyScore = 0.0;
bool isAnomaly = false;
boolean isWifiConnected = false;
SemaphoreHandle_t xBinarySemaphoreInternet = xSemaphoreCreateBinary();
// --- HÀM ĐIỀU KHIỂN LED ---
void updateLeds(){
    digitalWrite(PIN_LED_CHECK, LOW);
    digitalWrite(PIN_LED_IMPORT, LOW);      
    digitalWrite(PIN_LED_SELL, LOW);

    switch(currentSystemMode){
        case 0: //CHECK
            digitalWrite(PIN_LED_CHECK, HIGH);
            break;
        case 1: //IMPORT
            digitalWrite(PIN_LED_IMPORT, HIGH);
            break;
        case 2: //EXPORT
            digitalWrite(PIN_LED_SELL, HIGH);
            break;
    }
}