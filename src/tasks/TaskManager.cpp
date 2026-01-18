// src/tasks/TaskManager.cpp
#include "tasks/TaskManager.h"
#include "app/inventory.h"
#include "config/SystemConfig.h"
#include <WiFi.h>
#include <PubSubClient.h>

// --- ĐỊNH NGHĨA CÁC CHẾ ĐỘ ---
enum AppMode {
    MODE_CHECK = 0, // Chỉ kiểm tra (Mặc định)
    MODE_IMPORT,    // Nhập kho (Tăng số lượng)
    MODE_SELL       // Bán hàng (Giảm số lượng)
};

// --- CẤU HÌNH WIFI / MQTT ---
const char* ssid = "THD";             // Tên WiFi nhà bạn
const char* password = "hcmutk23@";   // Mật khẩu WiFi
const char* mqtt_server = "192.168.1.4"; // IP máy tính chạy Node-RED
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);
InventoryManager* myWarehouse;

// Biến toàn cục quản lý trạng thái
QueueHandle_t g_inputQ; 
QueueHandle_t g_displayQ;
AppMode currentMode = MODE_CHECK; // Mặc định là chế độ kiểm tra

// --- HÀM HỖ TRỢ ---
void sendLog(String msg) {
    if (client.connected()) {
        client.publish("warehouse/logs", msg.c_str());
    }
    Serial.println("[LOG] " + msg);
}

// --- CALLBACK MQTT (NHẬN LỆNH TỪ NODE-RED) ---
void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) message += (char)payload[i];
    Serial.print("[MQTT] Cmd: "); Serial.println(message);

    SystemMessage msgOut;
    msgOut.type = EVENT_UPDATE_LCD;

    if (String(topic) == "warehouse/control") {
        // 1. Lệnh Chuyển Chế Độ
        if (message == "SET_MODE_IMPORT") {
            currentMode = MODE_IMPORT;
            snprintf(msgOut.payload, 32, "MODE: IMPORT (+)");
            sendLog("Switched to IMPORT MODE");
        }
        else if (message == "SET_MODE_SELL") {
            currentMode = MODE_SELL;
            snprintf(msgOut.payload, 32, "MODE: SELL (-)");
            sendLog("Switched to SELL MODE");
        }
        else if (message == "SET_MODE_CHECK") {
            currentMode = MODE_CHECK;
            snprintf(msgOut.payload, 32, "MODE: CHECK (?)");
            sendLog("Switched to CHECK MODE");
        }
        // 2. Lệnh Hệ Thống
        else if (message == "SYNC_NOW") {
            SystemMessage syncMsg;
            syncMsg.type = EVENT_SYNC_CLOUD;
            xQueueSend(g_inputQ, &syncMsg, 10);
            return; // Thoát để task xử lý
        }
        
        // Cập nhật màn hình LCD báo chế độ mới
        xQueueSend(g_displayQ, &msgOut, 10);
    }
}

void setupWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(500 / portTICK_PERIOD_MS);
        Serial.print(".");
    }
    Serial.println("\n[WIFI] Connected");
}

void reconnectMQTT() {
    if (!client.connected()) {
        String clientId = "ESP32Warehouse-" + String(random(0xffff), HEX);
        if (client.connect(clientId.c_str())) {
            client.subscribe("warehouse/control");
            Serial.println("[MQTT] Connected");
        }
    }
}

void initMockData() {
    myWarehouse = new InventoryManager();
    // Tạo sẵn 1 món để test
    List1D<InventoryAttribute> attrs;
    attrs.add(InventoryAttribute("RFID", 12345)); 
    attrs.add(InventoryAttribute("Price", 1000));
    myWarehouse->addProduct(attrs, "Iphone 15", 5); // Có sẵn 5 cái
    Serial.println("[DATA] Database Initialized");
}

// --- LOGIC CHÍNH ---
void TaskManagerFunc(void *pvParameters) {
    ManagerTaskParams* params = (ManagerTaskParams*)pvParameters;
    g_inputQ = params->inputQueue;
    g_displayQ = params->displayQueue;

    initMockData();
    setupWiFi();
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(mqttCallback);

    SystemMessage msgIn, msgOut;

    for (;;) {
        if (WiFi.status() == WL_CONNECTED) {
            if (!client.connected()) reconnectMQTT();
            client.loop();
        }

        if (xQueueReceive(g_inputQ, &msgIn, 10) == pdTRUE) {
            
            // === XỬ LÝ QUÉT THẺ RFID ===
            if (msgIn.type == EVENT_SCAN_RFID) {
                String scannedStr = String(msgIn.payload);
                double rfidVal = scannedStr.toDouble();
                
                // Tìm xem hàng đã có chưa?
                // Lưu ý: query trả về List tên, ta cần index để sửa đổi. 
                // Do hàm query của bạn chỉ trả về tên, ta sẽ duyệt thủ công để lấy index cho chính xác.
                int foundIndex = -1;
                for(int i=0; i<myWarehouse->size(); i++) {
                    List1D<InventoryAttribute> attrs = myWarehouse->getProductAttributes(i);
                    // Giả sử thuộc tính đầu tiên là RFID (hoặc duyệt tìm)
                    for(int j=0; j<attrs.size(); j++) {
                        if(attrs.get(j).name == "RFID" && attrs.get(j).value == rfidVal) {
                            foundIndex = i;
                            break;
                        }
                    }
                    if(foundIndex != -1) break;
                }

                msgOut.type = EVENT_UPDATE_LCD;

                // --- LOGIC THEO TỪNG CHẾ ĐỘ ---
                switch (currentMode) {
                    
                    case MODE_CHECK: // Chỉ xem
                        if (foundIndex != -1) {
                            string name = myWarehouse->getProductName(foundIndex);
                            int qty = myWarehouse->getProductQuantity(foundIndex);
                            snprintf(msgOut.payload, 32, "%s: %d", name.c_str(), qty);
                            sendLog("Check: " + String(name.c_str()) + " (Qty: " + String(qty) + ")");
                        } else {
                            snprintf(msgOut.payload, 32, "Unknown Item");
                        }
                        break;

                    case MODE_IMPORT: // Nhập hàng
                        if (foundIndex != -1) {
                            // Hàng đã có -> Tăng số lượng
                            int oldQty = myWarehouse->getProductQuantity(foundIndex);
                            myWarehouse->updateQuantity(foundIndex, oldQty + 1);
                            snprintf(msgOut.payload, 32, "Added! Qty: %d", oldQty + 1);
                            sendLog("Imported existing item. New Qty: " + String(oldQty + 1));
                        } else {
                            // Hàng chưa có -> Tạo mới (Mặc định tên là New Item + ID)
                            List1D<InventoryAttribute> newAttrs;
                            newAttrs.add(InventoryAttribute("RFID", rfidVal));
                            newAttrs.add(InventoryAttribute("Price", 0)); // Giá mặc định 0
                            
                            String newName = "Item-" + scannedStr;
                            myWarehouse->addProduct(newAttrs, newName.c_str(), 1);
                            
                            snprintf(msgOut.payload, 32, "New: %s", newName.c_str());
                            sendLog("Created NEW item: " + newName);
                        }
                        break;

                    case MODE_SELL: // Bán hàng
                        if (foundIndex != -1) {
                            int oldQty = myWarehouse->getProductQuantity(foundIndex);
                            if (oldQty > 0) {
                                myWarehouse->updateQuantity(foundIndex, oldQty - 1);
                                snprintf(msgOut.payload, 32, "Sold! Rem: %d", oldQty - 1);
                                sendLog("Sold 1 item. Remaining: " + String(oldQty - 1));
                            } else {
                                snprintf(msgOut.payload, 32, "Out of Stock!");
                                sendLog("Sell failed: Out of Stock");
                            }
                        } else {
                            snprintf(msgOut.payload, 32, "Not Found!");
                        }
                        break;
                }
                xQueueSend(g_displayQ, &msgOut, 10);
            }
            
            // === XỬ LÝ ĐỒNG BỘ DỮ LIỆU ===
            else if (msgIn.type == EVENT_SYNC_CLOUD) {
                client.publish("warehouse/all", myWarehouse->toString().c_str());
                msgOut.type = EVENT_UPDATE_LCD;
                snprintf(msgOut.payload, 32, "Syncing...");
                xQueueSend(g_displayQ, &msgOut, 10);
            }
        }
    }
}