#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "config/SystemConfig.h"
#include "hal/HalLCD.h"
#include "hal/HalRFID.h" 

// Include các file header chuẩn bạn đã gửi
#include "tasks/TaskInput.h"
#include "tasks/TaskDisplay.h"
#include "tasks/TaskManager.h"
#include "tasks/task_webserver.h"
#include "tasks/TaskTemp.h"
#include "tinyml.h"
#include "global.h"

// 1. Khai báo phần cứng toàn cục
HalLCD myLCD;
HalRFID myRFID;

// 2. Khai báo Queue Handle
QueueHandle_t g_queueRFID_to_Manager; // Queue gửi từ Input -> Manager
QueueHandle_t g_queueManager_to_LCD;  // Queue gửi từ Manager -> LCD

// Import hàm từ task_wifi (giữ nguyên logic cũ)
void startAP();
void startSTA();
bool check_info_File(bool check);

void setup() {
    Serial.begin(115200);
    // Cấu hình chân LED chế độ
    pinMode(PIN_LED_CHECK, OUTPUT);
    pinMode(PIN_LED_IMPORT, OUTPUT);
    pinMode(PIN_LED_SELL, OUTPUT);
    updateLeds();
    // Init phần cứng cơ bản
    myLCD.init();
    myLCD.showMessage("System Init...", "Booting RTOS");
    
    // Tạo Queue (QUAN TRỌNG)
    g_queueRFID_to_Manager = xQueueCreate(10, sizeof(SystemMessage));
    g_queueManager_to_LCD = xQueueCreate(10, sizeof(SystemMessage));

    // Setup WiFi
    check_info_File(false); // Kiểm tra file cấu hình
    myLCD.showStatus("Connecting WiFi");
    startSTA();

    // --- CẤU HÌNH THAM SỐ CHO TASK (ĐÃ SỬA LỖI Ở ĐÂY) ---

    // 1. Task Input: Sửa tên biến theo TaskInput.h
    static InputTaskParams inputParams; 
    inputParams.outputQueue = g_queueRFID_to_Manager; // Tên đúng là outputQueue
    inputParams.rfidReader = &myRFID;                 // Phải truyền địa chỉ (&) của biến myRFID

    // 2. Task Display: Sửa tên biến theo TaskDisplay.h
    static DisplayTaskParams displayParams;
    displayParams.inputQueue = g_queueManager_to_LCD; // Tên đúng là inputQueue
    displayParams.lcdScreen = &myLCD;                 // Phải truyền địa chỉ (&) của biến myLCD

    // 3. Task Manager: Sửa tên biến theo TaskManager.h
    static ManagerTaskParams managerParams;
    managerParams.inputQueue = g_queueRFID_to_Manager;
    managerParams.displayQueue = g_queueManager_to_LCD;

    // --- KÍCH HOẠT CÁC TASK ---

    // Core 0: WebServer
    xTaskCreatePinnedToCore(TaskWebServerFunc, "WebServer", 8192, NULL, 1, NULL, 0);

    // Core 1: Logic & Hardware
    xTaskCreatePinnedToCore(TaskManagerFunc, "Manager", 8192, &managerParams, 2, NULL, 1);
    xTaskCreatePinnedToCore(TaskInputFunc,   "Input",   4096, &inputParams,   1, NULL, 1);
    xTaskCreatePinnedToCore(TaskDisplayFunc, "Display", 4096, &displayParams, 1, NULL, 1);
    xTaskCreatePinnedToCore(TaskTempFunc, "TaskTemp", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(tiny_ml_task, "TinyML", 8192, NULL, 1, NULL, 0);
    Serial.println(">>> SYSTEM STARTED SUCCESS <<<");
}

void loop() {
    // Loop trống, nhường CPU cho RTOS
    vTaskDelete(NULL);
}