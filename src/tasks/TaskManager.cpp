#include "tasks/TaskManager.h"
#include "app/inventory.h"
#include "config/SystemConfig.h"
#include <WiFi.h>
#include <PubSubClient.h>

// --- ĐỊNH NGHĨA CÁC CHẾ ĐỘ ---
enum AppMode {
    MODE_CHECK = 0, // Chỉ kiểm tra (Mặc định - LED Xanh dương)
    MODE_IMPORT = 1, // Nhập kho (Tăng số lượng - LED Xanh lá)
    MODE_SELL = 2    // Bán hàng (Để dự phòng, vì giờ ta xuất hàng qua Web là chính)
};

// --- CẤU HÌNH WIFI / MQTT ---
const char* ssid = "THD";             
const char* password = "hcmutk23@";   
const char* mqtt_server = "192.168.1.3"; // Nhớ check lại IP máy tính
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);
InventoryManager* myWarehouse;

// Biến toàn cục quản lý trạng thái
QueueHandle_t g_inputQ; 
QueueHandle_t g_displayQ;
int currentMode = MODE_CHECK; // Mặc định là chế độ kiểm tra

// --- HÀM HỖ TRỢ GỬI LOG ---
void sendLog(String msg) {
    if (client.connected()) {
        client.publish("warehouse/logs", msg.c_str());
    }
    Serial.println("[LOG] " + msg);
}

// --- HÀM GỬI DỮ LIỆU JSON CHO NODE-RED (Giữ nguyên logic của bạn) ---
void sendDataToDashboard() {
    if (!client.connected()) return;

    // Bắt đầu chuỗi JSON mảng: [
    String json = "[";
    
    for (int i = 0; i < myWarehouse->size(); i++) {
        // 1. Lấy tên và số lượng
        String pName = String(myWarehouse->getProductName(i).c_str());
        int pQty = myWarehouse->getProductQuantity(i);
        
        // 2. Lấy giá tiền (Duyệt attributes)
        double pPrice = 0;
        List1D<InventoryAttribute> attrs = myWarehouse->getProductAttributes(i);
        for(int j=0; j<attrs.size(); j++) {
            if(attrs.get(j).name == "Price") {
                pPrice = attrs.get(j).value;
                break;
            }
        }

        // 3. Đóng gói JSON: {"name":"...", "qty":..., "price":...}
        json += "{\"name\":\"" + pName + "\",";
        json += "\"qty\":" + String(pQty) + ",";
        json += "\"price\":" + String((long)pPrice) + "}"; 
        
        // Nếu không phải phần tử cuối thì thêm dấu phẩy
        if (i < myWarehouse->size() - 1) json += ",";
    }
    
    json += "]";

    // Gửi vào topic "warehouse/data"
    client.publish("warehouse/data", json.c_str(), true);
}

// --- CALLBACK MQTT: ĐÃ SỬA ĐỂ HIỂU JSON TỪ NODE-RED MỚI ---
// Node-RED gửi: {"cmd":"SET_MODE", "val":1} hoặc {"cmd":"EXPORT", "name":"ABC", "qty":5}
void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) message += (char)payload[i];
    Serial.print("[MQTT] Recv: "); Serial.println(message);

    SystemMessage msgOut;

    if (String(topic) == "warehouse/control") {
        
        // === 1. XỬ LÝ LỆNH CHUYỂN CHẾ ĐỘ (SET_MODE) ===
        // Tìm chuỗi "SET_MODE" trong tin nhắn JSON
        if (message.indexOf("SET_MODE") >= 0) {
            // Parse thủ công lấy giá trị "val"
            int valPos = message.indexOf("\"val\":");
            if (valPos > 0) {
                // Lấy ký tự số ngay sau "val":
                int modeVal = message.substring(valPos + 6).toInt();
                
                if (modeVal == 1) {
                    currentMode = MODE_IMPORT;
                    sendLog(">> Chuyen che do: NHAP KHO");
                } else {
                    currentMode = MODE_CHECK;
                    sendLog(">> Chuyen che do: GIAM SAT");
                }
                
                // Cập nhật LCD
                msgOut.type = EVENT_UPDATE_LCD; // Dùng EVENT_UPDATE_DISPLAY thay vì UPDATE_LCD nếu struct task của bạn dùng tên này
                snprintf(msgOut.payload, 32, "Mode: %s", (currentMode == MODE_IMPORT) ? "IMPORT" : "CHECK");
                xQueueSend(g_displayQ, &msgOut, 10);
                
                sendDataToDashboard(); // Cập nhật lại Web
            }
        }

        // === 2. XỬ LÝ LỆNH XUẤT KHO TỪ WEB (EXPORT) ===
        // JSON: {"cmd": "EXPORT", "name": "Noi com dien", "qty": 2}
        else if (message.indexOf("EXPORT") >= 0) {
            
            // a. Tách Tên (Name)
            int nameStart = message.indexOf("\"name\":\"") + 8;
            int nameEnd = message.indexOf("\"", nameStart);
            String pName = message.substring(nameStart, nameEnd);
            
            // b. Tách Số lượng (Qty)
            int qtyStart = message.indexOf("\"qty\":") + 6;
            int qtyEnd = message.indexOf("}", qtyStart);
            // Fix lỗi nếu có dấu phẩy phía sau qty
            int commaPos = message.indexOf(",", qtyStart); 
            if (commaPos != -1 && commaPos < qtyEnd) qtyEnd = commaPos;
            
            int pQty = message.substring(qtyStart, qtyEnd).toInt();

            // c. Gửi lệnh vào Queue để TaskManager xử lý
            if (pName.length() > 0 && pQty > 0) {
                SystemMessage exportMsg;
                exportMsg.type = EVENT_EXPORT_CMD; // Đảm bảo đã define cái này trong SystemConfig.h
                exportMsg.value = pQty; 
                strncpy(exportMsg.payload, pName.c_str(), sizeof(exportMsg.payload) - 1);
                exportMsg.payload[sizeof(exportMsg.payload) - 1] = '\0';

                xQueueSend(g_inputQ, &exportMsg, 10);
                Serial.println("-> Da gui lenh xuat kho vao Queue");
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
        Serial.print("IP: "); Serial.println(WiFi.localIP());
    } else {
        Serial.println("\n[WIFI] Connect Failed!");
    }
}

void reconnectMQTT() {
    if (!client.connected()) {
        String clientId = "ESP32Warehouse-" + String(random(0xffff), HEX);
        if (client.connect(clientId.c_str())) {
            client.subscribe("warehouse/control");
            Serial.println("[MQTT] Connected & Subscribed");
            sendDataToDashboard(); 
        } else {
             vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
    }
}

// Hàm thêm dữ liệu giả lập (Mockup)
void addItem(String hexId, String name, int qty, double price) {
    List1D<InventoryAttribute> attrs;
    unsigned long rfidDec = strtoul(hexId.c_str(), NULL, 16);
    attrs.add(InventoryAttribute("RFID", (double)rfidDec));
    attrs.add(InventoryAttribute("Price", price));
    myWarehouse->addProduct(attrs, name.c_str(), qty);
}

void initMockData() {
    myWarehouse = new InventoryManager();

    // Dữ liệu mẫu phong phú cho Dashboard đẹp
    addItem("19D01AB3", "Noi com dien",    10, 500000);  
    addItem("297124B3", "May xay sinh to", 5,  350000);  
    addItem("191D1BB3", "Quat dung",       15, 250000);  
    addItem("195623B3", "Ban ui hoi nuoc", 8,  180000);  
    addItem("59B4DCC2", "Am sieu toc",     20, 120000);  
    addItem("2D0F7506", "May say toc",     12, 150000);  
    addItem("66E5C901", "Lo vi song",      3,  1200000); 

    Serial.println("[DATA] Mock data initialized.");
}

// --- LOGIC CHÍNH (Loop) ---
void TaskManagerFunc(void *pvParameters) {
    ManagerTaskParams* params = (ManagerTaskParams*)pvParameters;
    g_inputQ = params->inputQueue;
    g_displayQ = params->displayQueue;

    initMockData();
    setupWiFi();
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(mqttCallback);
    
    // Tăng buffer để chứa chuỗi JSON dài
    client.setBufferSize(4096); 

    SystemMessage msgIn, msgOut;

    for (;;) {
        // Duy trì kết nối mạng
        if (WiFi.status() == WL_CONNECTED) {
            if (!client.connected()) reconnectMQTT();
            client.loop();
        }

        // Nhận sự kiện từ hàng đợi
        if (xQueueReceive(g_inputQ, &msgIn, 10) == pdTRUE) {
            
            // === 1. XỬ LÝ QUÉT THẺ RFID ===
            if (msgIn.type == EVENT_SCAN_RFID) { // Hoặc EVENT_RFID_READ tùy define của bạn
                String scannedStr = String(msgIn.payload); // ID thẻ dạng Hex String
                double rfidVal = (double)strtoul(scannedStr.c_str(), NULL, 16);
                
                int foundIndex = -1;
                // Logic tìm kiếm item theo thuộc tính RFID
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

                // Xử lý theo chế độ hiện tại
                switch (currentMode) {
                    case MODE_CHECK: // CHỈ XEM
                        if (foundIndex != -1) {
                            String name = String(myWarehouse->getProductName(foundIndex).c_str());
                            int qty = myWarehouse->getProductQuantity(foundIndex);
                            snprintf(msgOut.payload, 32, "Check: %s", name.c_str());
                            
                            // Log chi tiết hơn lên Web
                            sendLog("KIEM TRA: " + name + " (Ton: " + String(qty) + ")");
                        } else {
                            snprintf(msgOut.payload, 32, "The la!");
                            sendLog("CANH BAO: Phat hien the la: " + scannedStr);
                        }
                        // Check thì không cần update bảng data, đỡ lag
                        break;

                    case MODE_IMPORT: // NHẬP KHO
                        if (foundIndex != -1) {
                            // Hàng cũ -> Tăng số lượng
                            int oldQty = myWarehouse->getProductQuantity(foundIndex);
                            myWarehouse->updateQuantity(foundIndex, oldQty + 1);
                            
                            String name = String(myWarehouse->getProductName(foundIndex).c_str());
                            snprintf(msgOut.payload, 32, "Them: %s", name.c_str());
                            sendLog("DA NHAP: " + name + " (+1)");
                        } else {
                            // Hàng mới -> Tạo mới
                            List1D<InventoryAttribute> newAttrs;
                            newAttrs.add(InventoryAttribute("RFID", rfidVal));
                            newAttrs.add(InventoryAttribute("Price", 0)); 
                            String newName = "SP-" + scannedStr; // Tên tạm
                            myWarehouse->addProduct(newAttrs, newName.c_str(), 1);
                            
                            snprintf(msgOut.payload, 32, "Moi: %s", newName.c_str());
                            sendLog("NHAP MOI: " + newName);
                        }
                        sendDataToDashboard(); // UPDATE NGAY
                        break;
                }
                // Gửi ra màn hình LCD
                xQueueSend(g_displayQ, &msgOut, 10);
            }
            
            // === 2. XỬ LÝ LỆNH XUẤT KHO TỪ DASHBOARD (EXPORT) ===
            // (Code logic xử lý này của bạn đã Rất Tốt, giữ nguyên)
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
                            snprintf(msgOut.payload, 32, "Xuat: -%d %s", exportQty, targetName.c_str());
                            xQueueSend(g_displayQ, &msgOut, 10);
                            
                            sendLog("LENH WEB: Xuat " + String(exportQty) + " " + targetName);
                            sendDataToDashboard(); // Cập nhật lại Web
                        } else {
                            sendLog("LOI: Khong du hang de xuat " + targetName);
                            msgOut.type = EVENT_UPDATE_LCD;
                            snprintf(msgOut.payload, 32, "Loi: Het hang!");
                            xQueueSend(g_displayQ, &msgOut, 10);
                        }
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    sendLog("LOI: Khong tim thay san pham " + targetName);
                }
            }

            // === 3. XỬ LÝ ĐỒNG BỘ ===
            else if (msgIn.type == EVENT_SYNC_CLOUD) {
                sendDataToDashboard();
                sendLog("System Synced.");
            }
        }
    }
}