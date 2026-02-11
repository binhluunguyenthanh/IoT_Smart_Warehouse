#ifndef GLOBAL_H
#define GLOBAL_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "app/inventory.h"
#include "config/SystemConfig.h"

// Biến lưu SSID và Password Wifi (được nạp từ file cấu hình hoặc code cứng)
extern String WIFI_SSID;
extern String WIFI_PASS;

// --- CÁC KIỂU DỮ LIỆU DÙNG CHUNG ---

// Định nghĩa các loại sự kiện trong hệ thống để phân loại xử lý
enum EventType {
    EVENT_IDLE,         // Trạng thái nghỉ
    EVENT_SCAN_RFID,    // Sự kiện: Có thẻ RFID vừa được quét
    EVENT_UPDATE_LCD,   // Sự kiện: Cần cập nhật nội dung màn hình LCD
};

// Cấu trúc gói tin (Message) được gửi qua lại giữa các Task bằng Queue
struct SystemMessage {
    EventType type;     // Loại sự kiện là gì?
    char payload[32];   // Dữ liệu kèm theo (Ví dụ: Mã thẻ, Nội dung hiển thị)
    int value;          // Giá trị số (nếu cần, ví dụ: số lượng)
};

// --- CÁC ĐỐI TƯỢNG TOÀN CỤC (GLOBAL OBJECTS) ---

// Con trỏ tới đối tượng quản lý kho hàng (Database)
extern InventoryManager* myWarehouse;

// Mutex để bảo vệ biến myWarehouse (Chống tranh chấp dữ liệu giữa các Task)
extern SemaphoreHandle_t xInventoryMutex; 

// Các biến lưu trạng thái hệ thống
extern String lastScannedRFID; // Mã thẻ cuối cùng quét được
extern bool hasNewTag;         // Cờ báo có thẻ mới chưa xử lý
extern int currentSystemMode;  // Chế độ hiện tại: 0=Check, 1=Import, 2=Export

// Biến lưu dữ liệu cảm biến môi trường
extern float currentTemperature;
extern float currentHumidity;
   
// Hàm cập nhật trạng thái đèn LED (được định nghĩa ở global.cpp)
extern void updateLeds();

// Semaphore đồng bộ hóa: Báo hiệu khi đã kết nối Internet thành công
extern SemaphoreHandle_t xBinarySemaphoreInternet;

// Queue để gửi lệnh từ Task Manager sang Task Display
extern QueueHandle_t g_queueManager_to_LCD;

// Cờ báo trạng thái Wifi (dự phòng)
extern bool flagwwifiConnected;

#endif