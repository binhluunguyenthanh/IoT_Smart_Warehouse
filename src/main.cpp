#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "global.h"
#include "config/SystemConfig.h"
#include "hal/HalRFID.h" 

#include "tasks/TaskInput.h"
#include "tasks/TaskManager.h"
#include "tasks/TaskTemp.h"
#include "tasks/task_wifi.h" 

HalRFID myRFID;
QueueHandle_t g_queueRFID_to_Manager; 

void setup() {
    Serial.begin(115200);
    Serial.setTimeout(50); // Chống kẹt Serial

    pinMode(PIN_LED_CHECK, OUTPUT);
    pinMode(PIN_LED_IMPORT, OUTPUT);
    pinMode(PIN_LED_SELL, OUTPUT);
    updateLeds(); 
    
    xSerialMutex = xSemaphoreCreateMutex();
    xLcdMutex = xSemaphoreCreateMutex();

    if (xSerialMutex == NULL || xLcdMutex == NULL) {
        Serial.println("Lỗi: Không thể tạo Mutex!");
        while(1); 
    }

    // Khởi tạo LCD toàn cục
    myLCD.init();
    myLCD.showMessage("Edge Device", "System Ready.");
    
    // Khởi tạo Queue
    g_queueRFID_to_Manager = xQueueCreate(10, sizeof(SystemMessage));
    
    // Cập nhật lại Params cho Task (Xóa bỏ Queue gửi tới LCD)
    static InputTaskParams inputParams = {g_queueRFID_to_Manager, &myRFID}; 
    static ManagerTaskParams managerParams = {g_queueRFID_to_Manager, NULL}; // Sửa lại param này trong TaskManager.h nếu cần

    Serial.println(">>> STARTING EDGE TASKS...");

    // Khởi chạy các Task
    xTaskCreate(TaskManagerFunc,  "Manager",  4096, &managerParams, 2, NULL);
    xTaskCreate(TaskInputFunc,    "Input",    4096, &inputParams,   2, NULL); 
    xTaskCreate(TaskTempFunc,     "Temp",     4096, NULL,           1, NULL);
    xTaskCreate(TaskWifiTimeFunc, "WifiTime", 4096, NULL,           1, NULL);
    
    // Đã xóa xTaskCreate của TaskDisplay
}

void loop() {
    // Vòng lặp chính bị FreeRTOS chiếm quyền điều khiển
    vTaskDelete(NULL); 
}