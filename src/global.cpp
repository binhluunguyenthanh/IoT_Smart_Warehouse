#include "global.h"

// --- KHỞI TẠO QUẢN LÝ KHO ---
// Cấp phát bộ nhớ cho đối tượng quản lý kho
InventoryManager* myWarehouse = new InventoryManager();

// Tạo Mutex (Khóa) để bảo vệ dữ liệu kho khi nhiều Task cùng truy cập
SemaphoreHandle_t xInventoryMutex = xSemaphoreCreateMutex(); 

// --- KHỞI TẠO BIẾN TRẠNG THÁI ---
String lastScannedRFID = "";
bool hasNewTag = false;
int currentSystemMode = 0; // Mặc định là chế độ kiểm tra (Check)

// --- KHỞI TẠO BIẾN CẢM BIẾN ---
float currentTemperature = 0.0;
float currentHumidity = 0.0;

// --- KHỞI TẠO BIẾN MẠNG ---
boolean isWifiConnected = false;

// Tạo Binary Semaphore (dùng như một lá cờ báo hiệu sự kiện)
SemaphoreHandle_t xBinarySemaphoreInternet = xSemaphoreCreateBinary();

// Hàm điều khiển đèn LED dựa trên chế độ hoạt động (Mode)
void updateLeds(){
    // Tắt hết đèn trước
    digitalWrite(PIN_LED_CHECK, LOW);
    digitalWrite(PIN_LED_IMPORT, LOW);      
    digitalWrite(PIN_LED_SELL, LOW);

    // Bật đèn tương ứng với Mode
    switch(currentSystemMode){
        case 0: digitalWrite(PIN_LED_CHECK, HIGH); break;  // Mode Check
        case 1: digitalWrite(PIN_LED_IMPORT, HIGH); break; // Mode Import
        case 2: digitalWrite(PIN_LED_SELL, HIGH); break;   // Mode Export
    }
}