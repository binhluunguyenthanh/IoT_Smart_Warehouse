#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "global.h"
#include "config/SystemConfig.h"
#include "hal/HalLCD.h"
#include "hal/HalRFID.h" 

// Include các Task Header
#include "tasks/TaskInput.h"
#include "tasks/TaskDisplay.h"
#include "tasks/TaskManager.h"
#include "tasks/task_webserver.h"
#include "tasks/TaskTemp.h"
#include "tasks/task_wifi.h" 

// Khai báo đối tượng phần cứng toàn cục
HalLCD myLCD;
HalRFID myRFID;

// Khai báo Queue (Hàng đợi tin nhắn giữa các Task)
QueueHandle_t g_queueRFID_to_Manager; // Input -> Manager
QueueHandle_t g_queueManager_to_LCD;  // Manager -> LCD

void setup() {
    Serial.begin(115200);

    // 1. KHỞI TẠO PHẦN CỨNG CƠ BẢN (INIT HARDWARE)
    pinMode(PIN_LED_CHECK, OUTPUT);
    pinMode(PIN_LED_IMPORT, OUTPUT);
    pinMode(PIN_LED_SELL, OUTPUT);
    updateLeds(); // Cập nhật trạng thái đèn ban đầu
    
    myLCD.init();
    myLCD.showMessage("Smart Warehouse", "System Booting...");
    
    // 2. TẠO CÁC TÀI NGUYÊN RTOS (QUEUE & SEMAPHORE)
    // Lưu ý quan trọng: Phải tạo Queue TRƯỚC khi tạo Task để tránh lỗi Null Pointer
    g_queueRFID_to_Manager = xQueueCreate(10, sizeof(SystemMessage));
    g_queueManager_to_LCD  = xQueueCreate(10, sizeof(SystemMessage));
    
    // (Semaphore Internet đã được tạo trong global.cpp)

    // 3. CHUẨN BỊ THAM SỐ CHO TASK (TASK PARAMETERS)
    // Dùng static để biến không bị hủy khi thoát khỏi hàm setup()
    static InputTaskParams inputParams; 
    inputParams.outputQueue = g_queueRFID_to_Manager; 
    inputParams.rfidReader = &myRFID;                 

    static DisplayTaskParams displayParams;
    displayParams.inputQueue = g_queueManager_to_LCD; 
    displayParams.lcdScreen = &myLCD;                 

    static ManagerTaskParams managerParams;
    managerParams.inputQueue = g_queueRFID_to_Manager;
    managerParams.displayQueue = g_queueManager_to_LCD;

    // 4. TẠO TASK VÀ PHÂN LUỒNG (CORE PINNING)
    // Chiến thuật:
    // - Core 0 (App Core): Chạy các tác vụ Nền, Mạng (Wifi, Webserver) để không ảnh hưởng Logic.
    // - Core 1 (Pro Core): Chạy các tác vụ Thời gian thực (RFID, LCD, Logic kho) để phản hồi nhanh nhất.
    
    Serial.println(">>> STARTING TASKS...");

    // --- Core 0: Network & Background ---
    // Stack: 4096 bytes, Priority: 1
    xTaskCreatePinnedToCore(TaskWifiFunc, "TaskWifi", 4096, NULL, 1, NULL, 0);

    // Stack: 8192 bytes (Web cần nhiều RAM), Priority: 1
    xTaskCreatePinnedToCore(TaskWebServerFunc, "WebServer", 8192, NULL, 1, NULL, 0);

    // --- Core 1: Real-time Logic ---
    // Manager: Priority 2 (Cao hơn Display để xử lý logic trước)
    xTaskCreatePinnedToCore(TaskManagerFunc, "Manager", 8192, &managerParams, 2, NULL, 1);
    
    // Input: Priority 2 (Cần đọc thẻ nhanh)
    xTaskCreatePinnedToCore(TaskInputFunc,   "Input",   4096, &inputParams,   2, NULL, 1); 
    
    // Display: Priority 1 (Hiển thị chậm hơn xíu cũng được)
    xTaskCreatePinnedToCore(TaskDisplayFunc, "Display", 4096, &displayParams, 1, NULL, 1);
    
    // Temp: Priority 1 (Đọc nhiệt độ không cần gấp)
    xTaskCreatePinnedToCore(TaskTempFunc,    "Temp",    4096, NULL, 1, NULL, 1);

    Serial.println(">>> SYSTEM RUNNING <<<");
}

void loop() {
    // Trong mô hình FreeRTOS, loop() của Arduino không còn tác dụng.
    // Ta xóa Task này đi để giải phóng tài nguyên cho hệ thống.
    vTaskDelete(NULL); 
}