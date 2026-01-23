#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include "config/SystemConfig.h"
#include "hal/HalLCD.h"
#include "hal/HalRFID.h"

#include "tasks/TaskInput.h"
#include "tasks/TaskDisplay.h"
#include "tasks/TaskManager.h"

// --- KHAI BÁO PHẦN CỨNG TOÀN CỤC ---
HalLCD myLCD;
HalRFID myRFID;

// --- KHAI BÁO HÀNG ĐỢI (QUEUE) ---
QueueHandle_t qSensorToManager;
QueueHandle_t qManagerToDisplay;

// --- THAM SỐ TRUYỀN VÀO TASK ---
InputTaskParams inputParams;
DisplayTaskParams displayParams;
ManagerTaskParams managerParams;

void setup() {
    Serial.begin(115200);
    Serial.println("--- SYSTEM BOOTING ---");

    // 1. Khởi động màn hình LCD (RFID sẽ khởi động trong Task riêng)
    myLCD.init();

    // 2. Tạo hàng đợi tin nhắn
    qSensorToManager = xQueueCreate(10, sizeof(SystemMessage));
    qManagerToDisplay = xQueueCreate(10, sizeof(SystemMessage));

    if (qSensorToManager == NULL || qManagerToDisplay == NULL) {
        Serial.println("ERR: Queue Create Failed");
        while(1); // Treo máy nếu lỗi
    }

    // 3. Đóng gói tham số để gửi cho từng nhân viên (Task)
    
    // -> Cho nhân viên Input (Bảo vệ)
    inputParams.outputQueue = qSensorToManager;
    inputParams.rfidReader = &myRFID;

    // -> Cho nhân viên Display (Bảng tin)
    displayParams.inputQueue = qManagerToDisplay;
    displayParams.lcdScreen = &myLCD;

    // -> Cho nhân viên Manager (Quản kho)
    managerParams.inputQueue = qSensorToManager;
    managerParams.displayQueue = qManagerToDisplay;

    Serial.println("--- SCHEDULER STARTED ---");

    // 4. Tạo và chạy các Task (Phân luồng)
    // Task Input chạy Core 1
    xTaskCreatePinnedToCore(TaskInputFunc, "Input", 4096, &inputParams, 1, NULL, 1);
    
    // Task Display chạy Core 1
    xTaskCreatePinnedToCore(TaskDisplayFunc, "Display", 4096, &displayParams, 1, NULL, 1);
    
    // Task Manager (Nặng nhất: WiFi + Logic) chạy Core 0
    xTaskCreatePinnedToCore(TaskManagerFunc, "Manager", 16384, &managerParams, 2, NULL, 0);
}

void loop() {
    // Trong FreeRTOS, loop của Arduino không làm gì cả.
    // Ta xóa task này để giải phóng RAM cho hệ thống.
    vTaskDelete(NULL);
}
// #include <Arduino.h>
// #include <Wire.h>

// // Định nghĩa đúng chân bạn đang dùng
// #define SDA_PIN 33
// #define SCL_PIN 32

// void setup() {
//   Serial.begin(115200);
//   Serial.println("\nDang khoi dong I2C Scanner...");
  
//   // Khởi động I2C với chân mới
//   Wire.begin(SDA_PIN, SCL_PIN);
// }

// void loop() {
//   byte error, address;
//   int nDevices = 0;

//   Serial.println("Dang quet tim thiet bi...");

//   for(address = 1; address < 127; address++ ) {
//     Wire.beginTransmission(address);
//     error = Wire.endTransmission();

//     if (error == 0) {
//       Serial.print("TIM THAY thiet bi I2C tai dia chi: 0x");
//       if (address < 16) Serial.print("0");
//       Serial.print(address, HEX);
//       Serial.println("  !");
//       nDevices++;
//     }
//   }
  
//   if (nDevices == 0)
//     Serial.println("KHONG tim thay thiet bi nao -> Kiem tra lai day noi (long day?), nguon (5V?), hoac doi day SDA/SCL.");
//   else
//     Serial.println("Quet hoan tat.\n");

//   delay(5000); // Quét lại sau 5 giây
// }