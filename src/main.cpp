#include <Arduino.h>

// =================================================================================
// CHẾ ĐỘ HOẠT ĐỘNG (BẬT/TẮT Ở ĐÂY)
// =================================================================================

// --> BỎ COMMENT DÒNG DƯỚI ĐỂ CHẠY CHẾ ĐỘ THU THẬP DỮ LIỆU (TRAIN AI)
//#define DATA_COLLECTION_MODE 

// --> COMMENT DÒNG TRÊN LẠI ĐỂ CHẠY CHẾ ĐỘ KHO THÔNG MINH (NORMAL RUN)

// =================================================================================


#ifdef DATA_COLLECTION_MODE
// ---------------------------------------------------------------------------------
// CHẾ ĐỘ 1: THU THẬP DỮ LIỆU (DATA COLLECTION)
// Chỉ đọc cảm biến và in ra Serial để copy vào Python
// ---------------------------------------------------------------------------------
#include <DHT.h>

// Cấu hình giống hệt TaskTemp của bạn
#define DHTPIN 16      
#define DHTTYPE DHT22  

DHT dht_tool(DHTPIN, DHTTYPE);

void setup() {
    Serial.begin(115200);
    dht_tool.begin();
    Serial.println(">>> CHE DO THU THAP DU LIEU (DATA COLLECTION) <<<");
    Serial.println("Hay copy du lieu duoi day vao Python:");
    delay(2000);
}

void loop() {
    float h = dht_tool.readHumidity();
    float t = dht_tool.readTemperature();

    // Kiểm tra lỗi trước khi in
    if (!isnan(h) && !isnan(t)) {
        // In định dạng CSV: Nhiệt độ, Độ ẩm
        Serial.print("[");
        Serial.print(t);
        Serial.print(", ");
        Serial.print(h);
        Serial.println("],");
    } else {
        Serial.println("Error reading DHT!");
    }

    // Lấy mẫu mỗi 1 giây (nhanh hơn chút để lấy nhiều dữ liệu)
    delay(1000); 
}

#else
// ---------------------------------------------------------------------------------
// CHẾ ĐỘ 2: HỆ THỐNG KHO THÔNG MINH (SMART WAREHOUSE FULL SYSTEM)
// Chạy RTOS, Web, LCD, AI, RFID...
// ---------------------------------------------------------------------------------

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
#include "global.h"

// 1. Khai báo phần cứng toàn cục
HalLCD myLCD;
HalRFID myRFID;

// 2. Khai báo Queue Handle
QueueHandle_t g_queueRFID_to_Manager; // Queue gửi từ Input -> Manager
QueueHandle_t g_queueManager_to_LCD;  // Queue gửi từ Manager -> LCD

// Import hàm từ task_wifi
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

    // --- CẤU HÌNH THAM SỐ CHO TASK ---

    // 1. Task Input
    static InputTaskParams inputParams; 
    inputParams.outputQueue = g_queueRFID_to_Manager; 
    inputParams.rfidReader = &myRFID;                 

    // 2. Task Display
    static DisplayTaskParams displayParams;
    displayParams.inputQueue = g_queueManager_to_LCD; 
    displayParams.lcdScreen = &myLCD;                 

    // 3. Task Manager
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
  
    
    Serial.println(">>> SYSTEM STARTED SUCCESS <<<");
}

void loop() {
    // Loop trống, nhường CPU cho RTOS
    vTaskDelete(NULL);
}

#endif