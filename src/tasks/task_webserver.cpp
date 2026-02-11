#include "tasks/task_webserver.h"

String WIFI_SSID;
String WIFI_PASS;

AsyncWebServer server(80); // WebServer chạy port 80
AsyncWebSocket ws("/ws");  // WebSocket chạy đường dẫn /ws

// Biến lưu thời gian để gửi dữ liệu sensor định kỳ (thay vì delay cứng)
unsigned long lastSensorTime = 0;
const long sensorInterval = 2000; // 2 giây gửi 1 lần

// --- 2. CÁC HÀM XỬ LÝ LOGIC (API HANDLERS) ---

// API: Lấy dữ liệu kho hàng (Trả về JSON) - URL: /data
void handleGetData(AsyncWebServerRequest *request) {
    // Cố gắng lấy Mutex trong 1s để đọc dữ liệu an toàn
    if(xSemaphoreTake(xInventoryMutex, 1000 / portMAX_DELAY) == pdTRUE) { 
        // Tạo JSON Document (Dung lượng 8KB)
        DynamicJsonDocument doc(8192); 
        JsonArray array = doc.to<JsonArray>();

        // Duyệt qua kho và đóng gói thành JSON
        for (int i = 0; i < myWarehouse->size(); i++) {
            JsonObject item = array.createNestedObject();
            item["name"] = myWarehouse->getProductName(i);
            item["qty"]  = myWarehouse->getProductQuantity(i);
            item["price"]= myWarehouse->getProductPrice(i); 
            item["rfid"] = myWarehouse->getProductRFID(i);   
        }
        // Trả lại Mutex sau khi đọc xong
        xSemaphoreGive(xInventoryMutex);

        // Chuyển JSON thành chuỗi và gửi về Client
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    } else {
        // Nếu Mutex đang bận quá lâu -> Báo lỗi Server Busy
        request->send(504, "text/plain", "Server Busy (Mutex Timeout)");
    }
}

// API: Chuyển đổi chế độ (Import/Export/Check) - URL: /setmode?val=X
void handleSetMode(AsyncWebServerRequest *request) {
    if(request->hasParam("val")) {
        int mode = request->getParam("val")->value().toInt();
        currentSystemMode = mode;
        updateLeds(); // Hàm cập nhật đèn LED vật lý ngay lập tức
        
        // Gửi thông báo cho TẤT CẢ client qua WebSocket ngay lập tức để đồng bộ giao diện
        ws.textAll("MODE:" + String(mode));
        
        // Gửi lệnh ra màn hình LCD (nếu Queue LCD đã được khởi tạo)
        if (g_queueManager_to_LCD != NULL) {
            SystemMessage msg;
            msg.type = EVENT_UPDATE_LCD;
            String line1 = "MODE CHANGED:";
            String line2 = (mode == 1) ? ">> IMPORT" : ((mode == 2) ? ">> EXPORT" : ">> CHECK");
            String fullMsg = line1 + "|" + line2;
            
            // Copy chuỗi an toàn
            strncpy(msg.payload, fullMsg.c_str(), sizeof(msg.payload) - 1);
            xQueueSend(g_queueManager_to_LCD, &msg, 10);
        }
        request->send(200, "text/plain", "OK");
    } else {
        request->send(400, "text/plain", "Missing Value");
    }
}

// API: Xử lý Nhập/Xuất kho từ Web - URL: /action?type=...&name=...
void handleAction(AsyncWebServerRequest *request) {
    if(request->hasParam("type") && request->hasParam("name")) {
        String type = request->getParam("type")->value();
        String name = request->getParam("name")->value();
        // Lấy số lượng (mặc định là 1 nếu không truyền)
        int qty = request->hasParam("qty") ? request->getParam("qty")->value().toInt() : 1;
        String rfidHex = request->hasParam("rfid") ? request->getParam("rfid")->value() : "0";
        
        // Khóa Mutex để sửa đổi dữ liệu kho (RẤT QUAN TRỌNG)
        if(xSemaphoreTake(xInventoryMutex, portMAX_DELAY) == pdTRUE) {
            int index = myWarehouse->findIndexByName(name);
            
            // Xử lý logic NHẬP
            if (type == "IMPORT") {
                if (index != -1) {
                    // Sản phẩm đã có -> Cộng dồn số lượng
                    int oldQty = myWarehouse->getProductQuantity(index);
                    myWarehouse->updateQuantity(index, oldQty + qty);
                } else {
                    // Sản phẩm mới -> Thêm mới
                    myWarehouse->addProduct(name, qty, 0.0, rfidHex);
                }
            } 
            // Xử lý logic XUẤT
            else if (type == "EXPORT") {
                if (index != -1) {
                    int oldQty = myWarehouse->getProductQuantity(index);
                    if (oldQty >= qty) {
                        myWarehouse->updateQuantity(index, oldQty - qty);
                    }
                }
            }
            // Mở khóa Mutex
            xSemaphoreGive(xInventoryMutex);
            
            // Tối ưu: Sau khi action xong, báo client load lại data ngay lập tức
            ws.textAll("UPDATE_DATA"); 
            request->send(200, "text/plain", "OK");
        } else {
            request->send(500, "text/plain", "Busy");
        }
    } else {
        request->send(400, "text/plain", "Missing Params");
    }
}

// --- 3. WEBSOCKET EVENTS (NGƯỜI LẮNG NGHE) ---
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("Client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            // Ngay khi kết nối, gửi trạng thái hiện tại cho Client mới biết
            client->text("MODE:" + String(currentSystemMode));
            client->text("SENSOR:" + String(currentTemperature, 1) + ":" + String(currentHumidity, 0));
            break;
            
        case WS_EVT_DISCONNECT:
            Serial.printf("Client #%u disconnected\n", client->id());
            break;
            
    }
}

// --- 4. SETUP SERVER (NGƯỜI QUẢN LÝ) ---
void setupWebServer() {
    // 1. Khởi động File System (LittleFS)
    if(!LittleFS.begin(false)){
        Serial.println("LittleFS Error! Cannot mount.");
        return;
    }
    // Trả về mã 204 cho favicon để trình duyệt không báo lỗi 404
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(204); 
    });
    
    // 2. Định tuyến file tĩnh (HTML, CSS, JS) - Tự động tìm trong LittleFS

    // 3. Định tuyến API (Gắn các hàm ở phần 2 vào đây)
    server.on("/data", HTTP_GET, handleGetData);
    server.on("/setmode", HTTP_GET, handleSetMode);
    server.on("/action", HTTP_GET, handleAction);

    // 4. Kích hoạt WebSocket
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    // 5. Bắt đầu Server
    server.begin();
    Serial.println("Web Server Started on Port 80");
    // Serve file index.html mặc định
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
}

// --- 5. TASK CHÍNH (VÒNG LẶP VÔ TẬN TRONG RTOS) ---
void TaskWebServerFunc(void *pvParameters) {
    // Chờ tín hiệu đã có Internet từ Task Wifi
    Serial.println("Waiting for Internet...");
    xSemaphoreTake(xBinarySemaphoreInternet, portMAX_DELAY); // Chờ vô tận cho đến khi có mạng
    xSemaphoreGive(xBinarySemaphoreInternet); // Trả lại semaphore (nếu cần dùng lại)
    
    vTaskDelay(2000 / portTICK_PERIOD_MS); // Chờ ổn định
    
    // Cài đặt Server
    setupWebServer();
    
    // Vòng lặp duy trì hoạt động
    while(1) {
        // 1. Dọn dẹp các client WebSocket bị mất kết nối (Rất quan trọng để giải phóng RAM)
        ws.cleanupClients();

        // 2. Xử lý sự kiện có RFID mới quét
        if (hasNewTag) {
            ws.textAll("RFID:" + lastScannedRFID); // Bắn mã thẻ lên Web
            hasNewTag = false; // Reset cờ
        }
        
        // 3. Gửi thông số cảm biến định kỳ (Dùng millis để không chặn luồng)
        unsigned long currentMillis = millis();
        if (currentMillis - lastSensorTime > sensorInterval) {
            lastSensorTime = currentMillis;
            String msg = "SENSOR:" + String(currentTemperature, 1) + ":" + String(currentHumidity, 0);
            ws.textAll(msg);
        }

        // Nhường CPU 50ms cho các Task khác (tránh Watchdog Timer reset ESP)
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}