// src/tasks/TaskManager.cpp
#include "tasks/TaskManager.h"
#include "app/inventory.h"
#include "config/SystemConfig.h"
#include <WiFi.h>
#include <PubSubClient.h>

// --- CẤU HÌNH WIFI / MQTT (Đã chỉnh theo mạng nhà bạn) ---
const char* ssid = "THD";
const char* password = "hcmutk23@";
const char* mqtt_server = "192.168.1.4"; 
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);
InventoryManager* myWarehouse;

// Queue Handles để gửi lệnh xuống các task khác
QueueHandle_t g_inputQ; 
QueueHandle_t g_displayQ;

// --- HÀM XỬ LÝ KHI NHẬN TIN NHẮN TỪ NODE-RED (CALLBACK) ---
// Đã sửa 'byte' thành 'uint8_t' để tránh lỗi biên dịch
void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.print("[MQTT] Received: ");
    Serial.println(message);

    // Xử lý lệnh từ Dashboard
    if (String(topic) == "warehouse/control") {
        SystemMessage msg;
        
        if (message == "SYNC_NOW") {
            // Ra lệnh cho hệ thống Sync
            msg.type = EVENT_SYNC_CLOUD;
            xQueueSend(g_inputQ, &msg, 10);
            Serial.println("[CMD] Remote Sync Triggered!");
        }
        else if (message == "CLEAR_SCREEN") {
            msg.type = EVENT_UPDATE_LCD;
            snprintf(msg.payload, 32, " "); // Xóa dòng 2
            xQueueSend(g_displayQ, &msg, 10);
        }
    }
}

// --- HÀM HỖ TRỢ HIỆU NĂNG ---
void printSystemPerformance(const char* taskName) {
    uint32_t freeHeap = esp_get_free_heap_size();
    UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(NULL); 
    // Serial.printf("[%s] PERF >> Heap: %d | Stack: %d\n", taskName, freeHeap, highWaterMark);
}

void setupWiFi() {
    Serial.print("[WIFI] Connecting to ");
    Serial.println(ssid);
    
    // Chế độ Station (Kết nối vào Router nhà)
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    int attempt = 0;
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(500 / portTICK_PERIOD_MS);
        Serial.print(".");
        attempt++;
        if (attempt > 20) { // Nếu 10s không kết nối được
            Serial.println("\n[WIFI] Connect failed! Check Info.");
            // Không block vĩnh viễn, để hệ thống vẫn chạy offline
            break; 
        }
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WIFI] Connected!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    }
}

void reconnectMQTT() {
    if (!client.connected()) {
        Serial.print("[MQTT] Connecting to ");
        Serial.print(mqtt_server);
        Serial.print("...");
        
        // Tạo Client ID ngẫu nhiên để không bị đá văng
        String clientId = "ESP32Warehouse-" + String(random(0xffff), HEX);
        
        if (client.connect(clientId.c_str())) {
            Serial.println("Done.");
            // Đăng ký nhận lệnh điều khiển từ Node-RED
            client.subscribe("warehouse/control");
        } else {
            Serial.print("Failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again later");
            // Đợi 2s rồi thử lại (Non-blocking delay trong Loop tốt hơn, ở đây demo đơn giản)
        }
    }
}

void initMockData() {
    myWarehouse = new InventoryManager();
    // Reset list rỗng
    List1D<InventoryAttribute> attrs = List1D<InventoryAttribute>(); 
    
    // SP 1
    attrs.add(InventoryAttribute("RFID", 12345)); 
    attrs.add(InventoryAttribute("Price", 1000));
    myWarehouse->addProduct(attrs, "Iphone 15", 10);

    // SP 2
    attrs = List1D<InventoryAttribute>(); // Reset list cho SP mới
    attrs.add(InventoryAttribute("RFID", 67890));
    myWarehouse->addProduct(attrs, "ESP32 Chip", 50);
    
    Serial.println("[MANAGER] Data Loaded!");
}

void TaskManagerFunc(void *pvParameters) {
    ManagerTaskParams* params = (ManagerTaskParams*)pvParameters;
    g_inputQ = params->inputQueue;    // Lưu vào biến global để callback dùng
    g_displayQ = params->displayQueue;

    initMockData();
    setupWiFi();
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(mqttCallback); // Đăng ký hàm xử lý tin nhắn

    SystemMessage msgIn, msgOut;
    TickType_t lastPerfCheck = xTaskGetTickCount();

    for (;;) {
        // Chỉ kết nối MQTT khi đã có WiFi
        if (WiFi.status() == WL_CONNECTED) {
            if (!client.connected()) {
                reconnectMQTT();
            }
            client.loop(); // Quan trọng: Duy trì kết nối MQTT
        }

        // Gửi báo cáo sức khỏe mỗi 5s
        if (xTaskGetTickCount() - lastPerfCheck > 5000 / portTICK_PERIOD_MS) {
            printSystemPerformance("MANAGER");
            lastPerfCheck = xTaskGetTickCount();
            if (client.connected()) {
                String status = "Online | Heap: " + String(esp_get_free_heap_size());
                client.publish("warehouse/status", status.c_str());
            }
        }

        if (xQueueReceive(g_inputQ, &msgIn, 10) == pdTRUE) {
            if (msgIn.type == EVENT_SCAN_RFID) {
                String scannedID = String(msgIn.payload);
                double searchID = scannedID.toDouble();
                List1D<string> foundItems = myWarehouse->query("RFID", searchID, searchID, 0, true);

                msgOut.type = EVENT_UPDATE_LCD;
                if (foundItems.size() > 0) {
                    string itemName = foundItems.get(0);
                    snprintf(msgOut.payload, 32, "Found: %s", itemName.c_str());
                    // Gửi lên Node-RED
                    client.publish("warehouse/logs", ("Scan: " + String(itemName.c_str())).c_str());
                } else {
                    snprintf(msgOut.payload, 32, "Unknown Tag!");
                    client.publish("warehouse/logs", ("Scan Failed: " + scannedID).c_str());
                }
                xQueueSend(g_displayQ, &msgOut, 10);
            }
            else if (msgIn.type == EVENT_SYNC_CLOUD) {
                // Gửi toàn bộ dữ liệu kho lên Node-RED
                client.publish("warehouse/all", myWarehouse->toString().c_str());
                
                msgOut.type = EVENT_UPDATE_LCD;
                snprintf(msgOut.payload, 32, "Syncing...");
                xQueueSend(g_displayQ, &msgOut, 10);
                
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                snprintf(msgOut.payload, 32, "Sync Done!");
                xQueueSend(g_displayQ, &msgOut, 10);
            }
        }
    }
}