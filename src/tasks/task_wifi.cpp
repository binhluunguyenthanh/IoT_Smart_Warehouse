#include "tasks/task_wifi.h"

// --- BIẾN CỤC BỘ ---
AsyncWebServer configServer(80);
volatile bool hasNewWifiConfig = false; 

// HTML giữ nguyên như cũ của bạn
const char* config_html = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Warehouse Setup</title>
    <style>
        :root {
            --bg-color: #121212;
            --card-color: #1e1e1e;
            --text-color: #ffffff;
            --accent-color: #00d2ff;
            --accent-hover: #3a7bd5;
            --input-bg: #2c2c2c;
        }
        * { margin: 0; padding: 0; box-sizing: border-box; font-family: 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; }
        body {
            background: linear-gradient(135deg, var(--bg-color), #0d0d2b);
            min-height: 100vh; display: flex; align-items: center; justify-content: center; color: var(--text-color); padding: 20px;
        }
        .container {
            background: var(--card-color);
            padding: 40px 30px;
            border-radius: 20px;
            box-shadow: 0 15px 35px rgba(0,0,0,0.5);
            width: 100%; max-width: 400px; text-align: center;
            border: 1px solid rgba(255, 255, 255, 0.05);
        }
        h2 { margin-bottom: 25px; font-weight: 600; letter-spacing: 1px; color: var(--accent-color); text-transform: uppercase; }
        .input-group { margin-bottom: 20px; text-align: left; }
        label { display: block; margin-bottom: 8px; font-size: 0.9rem; color: #aaa; font-weight: 500; }
        input {
            width: 100%; padding: 15px; border: none; border-radius: 10px;
            background: var(--input-bg); color: var(--text-color); font-size: 1rem;
            transition: all 0.3s ease; outline: none;
        }
        input:focus { background: #353535; box-shadow: 0 0 0 2px var(--accent-color); }
        ::placeholder { color: #666; }
        button {
            width: 100%; padding: 15px; border: none; border-radius: 10px;
            background: linear-gradient(45deg, var(--accent-color), var(--accent-hover));
            color: white; font-size: 1.1rem; font-weight: bold; text-transform: uppercase;
            cursor: pointer; transition: all 0.3s ease; box-shadow: 0 10px 20px -10px rgba(0, 210, 255, 0.5);
        }
        button:hover { transform: translateY(-3px); box-shadow: 0 15px 25px -10px rgba(0, 210, 255, 0.7); }
        button:active { transform: scale(0.98); }
        .footer { margin-top: 25px; font-size: 0.8rem; color: #666; }
    </style>
</head>
<body>
    <div class="container">
        <h2>Cấu Hình Wifi</h2>
        <form action="/save" method="POST">
            <div class="input-group">
                <label for="ssid">Tên Wifi (SSID)</label>
                <input type="text" id="ssid" name="ssid" placeholder="Nhập tên Wifi..." required autocomplete="off">
            </div>
            <div class="input-group">
                <label for="pass">Mật khẩu</label>
                <input type="password" id="pass" name="pass" placeholder="Nhập mật khẩu..." required>
            </div>
            <button type="submit">KẾT NỐI NGAY</button>
        </form>
        <div class="footer">Smart Warehouse Demo System</div>
    </div>
</body>
</html>
)rawliteral";

// --- HÀM 1: CHẠY CHẾ ĐỘ AP (Cấu hình) ---
void runAPMode() {
    Serial.println(">>> [WIFI TASK] ENTERING AP MODE FOR DEMO <<<");
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32_SMART_WAREHOUSE", "12345678"); 
    Serial.print(">>> IP Config: "); Serial.println(WiFi.softAPIP());

    // 1. Reset cờ chờ
    hasNewWifiConfig = false;

    // 2. Setup Server Config
    configServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", config_html);
    });

    configServer.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){
        String s = "", p = "";
        if (request->hasParam("ssid", true)) s = request->getParam("ssid", true)->value();
        if (request->hasParam("pass", true)) p = request->getParam("pass", true)->value();

        if (s.length() > 0) {
            WIFI_SSID = s; // Cập nhật vào biến Global
            WIFI_PASS = p;
            
            // Báo hiệu cho vòng lặp biết là đã xong
            hasNewWifiConfig = true; 
            
            request->send(200, "text/html", "<h1>Da nhan! Dang chuyen sang che do ket noi...</h1>");
        } else {
            request->send(400, "text/plain", "Thieu ten Wifi");
        }
    });

    configServer.begin();

    // 3. VÒNG LẶP CHỜ (QUAN TRỌNG: Dùng vTaskDelay để không chặn RTOS)
    while(!hasNewWifiConfig) {
        // Nháy LED hoặc in log chờ ở đây nếu thích
        // Quan trọng: Phải có delay để nhường CPU cho các Task khác (Display, Input...)
        vTaskDelay(200 / portTICK_PERIOD_MS); 
    }

    // 4. Dọn dẹp sau khi nhận xong
    configServer.end();
    WiFi.softAPdisconnect(true);
    Serial.println(">>> [WIFI TASK] CONFIG RECEIVED -> SWITCHING TO STA <<<");
}

// --- HÀM 2: CHẠY CHẾ ĐỘ STA (Kết nối) ---
void runSTAMode() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID.c_str(), WIFI_PASS.c_str());
    Serial.print("Connecting to: "); Serial.println(WIFI_SSID);
    
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(500 / portTICK_PERIOD_MS); // Non-blocking delay
        Serial.print(".");
        retry++;
        
        // Nếu sai pass quá lâu -> Quay lại AP bắt nhập lại
        if(retry > 60) { // 30 giây
            Serial.println("\n❌ Connect Fail! Returning to AP Mode...");
            return; // Thoát hàm này để quay lại vòng lặp chính (Vào lại AP)
        }
    }
    
    Serial.println("\n✅ WIFI CONNECTED!");
    Serial.println(WiFi.localIP()); 
    
    // Mở khóa cho WebServer chính hoạt động
    xSemaphoreGive(xBinarySemaphoreInternet); 

    // Giám sát kết nối
    while(WiFi.status() == WL_CONNECTED) {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    
    // Nếu rớt mạng -> Thoát ra để logic bên dưới xử lý
    Serial.println("⚠️ Wifi Lost!");
}

// --- TASK CHÍNH ---
void TaskWifiFunc(void *pvParameters) {
    (void) pvParameters;

    while(1) {
        // 1. LUÔN LUÔN Bắt đầu bằng AP (Theo yêu cầu Demo)
        // Bất kể đã có file save hay chưa, cứ chạy AP trước.
        runAPMode();

        // 2. Sau khi nhập xong, chuyển sang STA
        runSTAMode();

        // 3. Nếu runSTAMode bị thoát ra (do sai pass hoặc mất mạng)
        // Vòng lặp while(1) sẽ quay lại bước 1 -> Bật lại AP để nhập lại.
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}