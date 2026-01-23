#include "tasks/TaskManager.h"
#include "app/inventory.h"
#include "config/SystemConfig.h"
#include <WiFi.h>
#include <PubSubClient.h>

// --- ĐỊNH NGHĨA CÁC CHẾ ĐỘ ---
enum AppMode {
    MODE_CHECK = 0, // Chỉ kiểm tra (Mặc định)
    MODE_IMPORT,    // Nhập kho (Tăng số lượng)
    MODE_SELL       // Bán hàng qua RFID (Giảm số lượng)
};

// --- CẤU HÌNH WIFI / MQTT ---
const char* ssid = "THD";             
const char* password = "hcmutk23@";   
const char* mqtt_server = "192.168.1.3"; 
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);
InventoryManager* myWarehouse;

// Biến toàn cục quản lý trạng thái
QueueHandle_t g_inputQ; 
QueueHandle_t g_displayQ;
AppMode currentMode = MODE_CHECK; // Mặc định là chế độ kiểm tra

// --- HÀM HỖ TRỢ GỬI LOG ---
void sendLog(String msg) {
    if (client.connected()) {
        client.publish("warehouse/logs", msg.c_str());
    }
    Serial.println("[LOG] " + msg);
}

// --- HÀM MỚI: Gửi dữ liệu JSON chuẩn cho Node-RED ---
void sendDataToDashboard() {
    if (!client.connected()) return;

    // Bắt đầu chuỗi JSON mảng: [
    String json = "[";
    
    for (int i = 0; i < myWarehouse->size(); i++) {
        // 1. Lấy tên và số lượng
        String pName = String(myWarehouse->getProductName(i).c_str());
        int pQty = myWarehouse->getProductQuantity(i);
        
        // 2. Lấy giá tiền (Cần duyệt qua attributes để tìm "Price")
        double pPrice = 0;
        List1D<InventoryAttribute> attrs = myWarehouse->getProductAttributes(i);
        for(int j=0; j<attrs.size(); j++) {
            if(attrs.get(j).name == "Price") {
                pPrice = attrs.get(j).value;
                break;
            }
        }

        // 3. Đóng gói thành object JSON: {"name":"...", "qty":..., "price":...}
        json += "{\"name\":\"" + pName + "\",";
        json += "\"qty\":" + String(pQty) + ",";
        json += "\"price\":" + String((long)pPrice) + "}"; 
        
        // Nếu không phải phần tử cuối thì thêm dấu phẩy
        if (i < myWarehouse->size() - 1) json += ",";
    }
    
    // Kết thúc chuỗi JSON: ]
    json += "]";

    // Gửi vào topic "warehouse/data" với cờ RETAIN = true (Lưu trữ tin nhắn)
    client.publish("warehouse/data", json.c_str(), true);
}

// --- CALLBACK MQTT (NHẬN LỆNH TỪ NODE-RED) ---
void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) message += (char)payload[i];
    Serial.print("[MQTT] Cmd: "); Serial.println(message);

    SystemMessage msgOut;

    // Xử lý lệnh điều khiển
    if (String(topic) == "warehouse/control") {
        
        // 1. Chuyển chế độ
        if (message == "MODE:IMPORT") {
            currentMode = MODE_IMPORT;
            msgOut.type = EVENT_UPDATE_LCD;
            snprintf(msgOut.payload, 32, "Mode: IMPORT");
            xQueueSend(g_displayQ, &msgOut, 10);
            sendLog("Switched to IMPORT Mode");
            sendDataToDashboard(); // Cập nhật Web ngay
        } 
        else if (message == "MODE:SELL") {
            currentMode = MODE_SELL;
            msgOut.type = EVENT_UPDATE_LCD;
            snprintf(msgOut.payload, 32, "Mode: SELL");
            xQueueSend(g_displayQ, &msgOut, 10);
            sendLog("Switched to SELL Mode");
            sendDataToDashboard(); // Cập nhật Web ngay
        }
        else if (message == "MODE:CHECK") {
            currentMode = MODE_CHECK;
            msgOut.type = EVENT_UPDATE_LCD;
            snprintf(msgOut.payload, 32, "Mode: CHECK");
            xQueueSend(g_displayQ, &msgOut, 10);
            sendLog("Switched to CHECK Mode");
            sendDataToDashboard(); // Cập nhật Web ngay
        }

        // 2. Xử lý lệnh Xuất Kho Thủ Công (EXPORT:Name:Qty)
        if (message.startsWith("EXPORT:")) {
            int firstColon = message.indexOf(':');
            int secondColon = message.lastIndexOf(':');
            
            if (firstColon > 0 && secondColon > firstColon) {
                String pName = message.substring(firstColon + 1, secondColon);
                String pQtyStr = message.substring(secondColon + 1);
                int pQty = pQtyStr.toInt();

                SystemMessage exportMsg;
                exportMsg.type = EVENT_EXPORT_CMD;
                exportMsg.value = pQty; 
                strncpy(exportMsg.payload, pName.c_str(), sizeof(exportMsg.payload) - 1);
                exportMsg.payload[sizeof(exportMsg.payload) - 1] = '\0';

                xQueueSend(g_inputQ, &exportMsg, 10);
            }
        }
    }
}

void setupWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 20) {
        vTaskDelay(500 / portTICK_PERIOD_MS);
        Serial.print(".");
        retries++;
    }
    if(WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WIFI] Connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\n[WIFI] Connection Failed!");
    }
}

void reconnectMQTT() {
    if (!client.connected()) {
        String clientId = "ESP32Warehouse-" + String(random(0xffff), HEX);
        if (client.connect(clientId.c_str())) {
            client.subscribe("warehouse/control");
            Serial.println("[MQTT] Connected");
            sendDataToDashboard(); // Gửi dữ liệu ngay khi kết nối lại
        } else {
             vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
    }
}

void addItem(String hexId, String name, int qty, double price) {
    List1D<InventoryAttribute> attrs;
    unsigned long rfidDec = strtoul(hexId.c_str(), NULL, 16);
    attrs.add(InventoryAttribute("RFID", (double)rfidDec));
    attrs.add(InventoryAttribute("Price", price));
    myWarehouse->addProduct(attrs, name.c_str(), qty);
}

void initMockData() {
    myWarehouse = new InventoryManager();

    // Dữ liệu mẫu (Thêm nhiều món để test)
    addItem("19D01AB3", "Noi com dien",    10, 500000);  
    addItem("297124B3", "May xay sinh to", 5,  350000);  
    addItem("191D1BB3", "Quat dung",       15, 250000);  
    addItem("195623B3", "Ban ui hoi nuoc", 8,  180000);  
    addItem("59B4DCC2", "Am sieu toc",     20, 120000);  
    addItem("2D0F7506", "May say toc",     12, 150000);  
    addItem("66E5C901", "Lo vi song",      3,  1200000); 

    Serial.println("[DATA] Initialized mock data.");
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
    
    // TĂNG BỘ ĐỆM MQTT ĐỂ GỬI JSON DÀI
    client.setBufferSize(2048); 

    SystemMessage msgIn, msgOut;

    for (;;) {
        if (WiFi.status() == WL_CONNECTED) {
            if (!client.connected()) reconnectMQTT();
            client.loop();
        }

        if (xQueueReceive(g_inputQ, &msgIn, 10) == pdTRUE) {
            
            // === 1. XỬ LÝ QUÉT THẺ RFID ===
            if (msgIn.type == EVENT_SCAN_RFID) {
                String scannedStr = String(msgIn.payload);
                double rfidVal = (double)strtoul(scannedStr.c_str(), NULL, 16);
                
                int foundIndex = -1;
                for(int i=0; i<myWarehouse->size(); i++) {
                    List1D<InventoryAttribute> attrs = myWarehouse->getProductAttributes(i);
                    for(int j=0; j<attrs.size(); j++) {
                        if(attrs.get(j).name == "RFID" && attrs.get(j).value == rfidVal) {
                            foundIndex = i;
                            break;
                        }
                    }
                    if(foundIndex != -1) break;
                }

                msgOut.type = EVENT_UPDATE_LCD;

                switch (currentMode) {
                    case MODE_CHECK: // Chỉ xem
                        if (foundIndex != -1) {
                            // Chuyển std::string sang String Arduino
                            String name = String(myWarehouse->getProductName(foundIndex).c_str());
                            int qty = myWarehouse->getProductQuantity(foundIndex);
                            snprintf(msgOut.payload, 32, "%s: %d", name.c_str(), qty);
                            sendLog("Check: " + name);
                        } else {
                            snprintf(msgOut.payload, 32, "Unknown Item");
                        }
                        sendDataToDashboard(); // Cập nhật Web
                        break;

                    case MODE_IMPORT: // Nhập hàng
                        if (foundIndex != -1) {
                            int oldQty = myWarehouse->getProductQuantity(foundIndex);
                            myWarehouse->updateQuantity(foundIndex, oldQty + 1);
                            snprintf(msgOut.payload, 32, "Added! Qty: %d", oldQty + 1);
                            sendLog("Imported: " + String(myWarehouse->getProductName(foundIndex).c_str()));
                        } else {
                            List1D<InventoryAttribute> newAttrs;
                            newAttrs.add(InventoryAttribute("RFID", rfidVal));
                            newAttrs.add(InventoryAttribute("Price", 0)); 
                            String newName = "Item-" + scannedStr;
                            myWarehouse->addProduct(newAttrs, newName.c_str(), 1);
                            snprintf(msgOut.payload, 32, "New: %s", newName.c_str());
                            sendLog("Created NEW: " + newName);
                        }
                        sendDataToDashboard(); // Cập nhật Web
                        break;

                    case MODE_SELL: // Bán hàng
                        if (foundIndex != -1) {
                            int oldQty = myWarehouse->getProductQuantity(foundIndex);
                            if (oldQty > 0) {
                                myWarehouse->updateQuantity(foundIndex, oldQty - 1);
                                snprintf(msgOut.payload, 32, "Sold! Rem: %d", oldQty - 1);
                                sendLog("Sold via RFID. Rem: " + String(oldQty - 1));
                                sendDataToDashboard(); // Cập nhật Web
                            } else {
                                snprintf(msgOut.payload, 32, "Out of Stock!");
                            }
                        } else {
                            snprintf(msgOut.payload, 32, "Not Found!");
                        }
                        break;
                }
                xQueueSend(g_displayQ, &msgOut, 10);
            }
            
            // === 2. XỬ LÝ LỆNH XUẤT KHO TỪ DASHBOARD (EXPORT) ===
            else if (msgIn.type == EVENT_EXPORT_CMD) {
                String targetName = String(msgIn.payload);
                int exportQty = msgIn.value;
                bool found = false;

                for (int i = 0; i < myWarehouse->size(); i++) {
                    if (String(myWarehouse->getProductName(i).c_str()) == targetName) {
                        int currentQty = myWarehouse->getProductQuantity(i);
                        
                        if (currentQty >= exportQty) {
                            myWarehouse->updateQuantity(i, currentQty - exportQty);
                            msgOut.type = EVENT_UPDATE_LCD;
                            snprintf(msgOut.payload, 32, "Exp: %s -%d", targetName.c_str(), exportQty);
                            xQueueSend(g_displayQ, &msgOut, 10);
                            sendLog("Web Export: " + targetName + " (-" + String(exportQty) + ")");
                            sendDataToDashboard(); // Cập nhật Web
                        } else {
                            sendLog("Export Fail: Not enough stock");
                            msgOut.type = EVENT_UPDATE_LCD;
                            snprintf(msgOut.payload, 32, "Err: Low Stock");
                            xQueueSend(g_displayQ, &msgOut, 10);
                        }
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    sendLog("Export Fail: Not found " + targetName);
                }
            }

            // === 3. XỬ LÝ ĐỒNG BỘ ===
            else if (msgIn.type == EVENT_SYNC_CLOUD) {
                sendDataToDashboard();
                msgOut.type = EVENT_UPDATE_LCD;
                snprintf(msgOut.payload, 32, "Syncing...");
                xQueueSend(g_displayQ, &msgOut, 10);
            }
        }
    }
}