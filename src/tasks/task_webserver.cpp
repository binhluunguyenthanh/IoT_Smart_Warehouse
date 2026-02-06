#include "tasks/task_webserver.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include "global.h"
#include "config/SystemConfig.h"

AsyncWebServer server(80);
AsyncWebSocket myWS("/ws"); 

// --- GIAO DIỆN QUẢN LÝ KHO CHUYÊN NGHIỆP VỚI CẢM BIẾN ---
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi" data-bs-theme="dark">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Hệ Thống Kho Thông Minh</title>
    
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css" rel="stylesheet">
    <link href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css" rel="stylesheet">
    <link href="https://cdn.datatables.net/1.13.4/css/dataTables.bootstrap5.min.css" rel="stylesheet">
    <link href="https://fonts.googleapis.com/css2?family=Roboto:wght@300;400;500;700&display=swap" rel="stylesheet">

    <style>
        :root {
            --sidebar-width: 260px;
            --primary-color: #0d6efd; 
            --dark-bg: #111827;
            --card-bg: #1f2937;
            --text-main: #f9fafb;
        }

        body { font-family: 'Roboto', sans-serif; background-color: var(--dark-bg); color: var(--text-main); overflow-x: hidden; }
        
        /* SIDEBAR */
        #sidebar {
            width: var(--sidebar-width); height: 100vh; position: fixed; left: 0; top: 0;
            background: #1f2937; border-right: 1px solid #374151; z-index: 1000;
        }
        .brand-logo {
            padding: 25px 20px; font-size: 20px; font-weight: 700; color: #fff; border-bottom: 1px solid #374151;
            display: flex; align-items: center; gap: 10px; letter-spacing: 1px;
        }
        .nav-link {
            color: #9ca3af; padding: 15px 20px; font-weight: 500; display: flex; align-items: center; gap: 12px; transition: 0.3s;
        }
        .nav-link:hover, .nav-link.active { background: #374151; color: #fff; border-left: 4px solid var(--primary-color); }
        .nav-link i { width: 20px; text-align: center; }

        /* CONTENT */
        #content { margin-left: var(--sidebar-width); padding: 30px; }
        
        /* CARDS */
        .stat-card {
            background: var(--card-bg); border-radius: 15px; padding: 25px; 
            border: 1px solid #374151; height: 100%; transition: transform 0.3s;
        }
        .stat-card:hover { transform: translateY(-5px); border-color: var(--primary-color); }
        .stat-value { font-size: 32px; font-weight: 700; margin: 10px 0; }
        .stat-label { font-size: 14px; color: #9ca3af; text-transform: uppercase; letter-spacing: 0.5px; }

        /* SENSOR CARDS STYLE */
        .sensor-card { position: relative; overflow: hidden; }
        .sensor-icon-bg { position: absolute; right: 10px; bottom: -10px; font-size: 80px; opacity: 0.1; }

        /* TABLE */
        .glass-table {
            background: var(--card-bg); border-radius: 15px; padding: 25px;
            border: 1px solid #374151;
        }
        table.dataTable tbody tr { background-color: transparent !important; }
        table.dataTable td, table.dataTable th { border-color: #374151; color: #e5e7eb; padding: 15px 10px; }

        /* MODES */
        .mode-btn {
            background: var(--card-bg); border: 2px solid #374151; padding: 30px 20px; border-radius: 15px;
            text-align: center; cursor: pointer; transition: 0.3s;
        }
        .mode-btn:hover { border-color: var(--primary-color); transform: scale(1.02); }
        .mode-btn.active { border-color: var(--primary-color); background: rgba(13, 110, 253, 0.1); }
        .mode-btn i { font-size: 40px; margin-bottom: 15px; display: block; }
    </style>
</head>
<body>

    <nav id="sidebar">
        <div class="brand-logo">
            <i class="fa-solid fa-cube text-primary"></i> SMART WAREHOUSE
        </div>
        <ul class="nav flex-column mt-4">
            <li class="nav-item"><a href="#" class="nav-link active" onclick="showSection('dashboard', this)"><i class="fa-solid fa-chart-pie"></i> Tổng Quan</a></li>
            <li class="nav-item"><a href="#" class="nav-link" onclick="showSection('inventory', this)"><i class="fa-solid fa-boxes-stacked"></i> Kho Hàng</a></li>
            <li class="nav-item"><a href="#" class="nav-link" onclick="showSection('operations', this)"><i class="fa-solid fa-gamepad"></i> Điều Khiển</a></li>
        </ul>
        <div style="position:absolute; bottom:30px; width:100%; padding:0 20px;">
            <div class="stat-card p-3 text-center bg-dark border-0">
                <small id="connStatus" style="color:#ef4444"><i class="fa-solid fa-wifi"></i> Mất kết nối</small>
            </div>
        </div>
    </nav>

    <div id="content">
        <div class="d-flex justify-content-between align-items-center mb-5">
            <div>
                <h3 class="fw-bold m-0">Dashboard Giám Sát</h3>
                <small class="text-muted">Cập nhật thời gian thực</small>
            </div>
            <span class="badge bg-primary fs-6 px-4 py-2 rounded-pill" id="sysModeBadge">Chế độ: CHECK</span>
        </div>

        <div id="dashboard" class="page-section">
            
            <div class="row g-4 mb-4">
                <div class="col-md-6">
                    <div class="stat-card sensor-card d-flex justify-content-between align-items-center">
                        <div>
                            <div class="stat-label mb-2"><i class="fa-solid fa-temperature-half me-2"></i>Nhiệt Độ Kho</div>
                            <div class="stat-value" style="color: #ff6384"><span id="valTemp">--</span> °C</div>
                            <div class="mt-2">
                                <span class="badge rounded-pill bg-dark border border-danger text-danger" id="stsTemp">Đang tải...</span>
                            </div>
                        </div>
                        <div class="sensor-icon-bg" style="color: #ff6384"><i class="fa-solid fa-fire"></i></div>
                    </div>
                </div>
                <div class="col-md-6">
                    <div class="stat-card sensor-card d-flex justify-content-between align-items-center">
                        <div>
                            <div class="stat-label mb-2"><i class="fa-solid fa-droplet me-2"></i>Độ Ẩm Không Khí</div>
                            <div class="stat-value" style="color: #36a2eb"><span id="valHum">--</span> %</div>
                            <div class="mt-2">
                                <span class="badge rounded-pill bg-dark border border-info text-info" id="stsHum">Đang tải...</span>
                            </div>
                        </div>
                        <div class="sensor-icon-bg" style="color: #36a2eb"><i class="fa-solid fa-cloud-showers-heavy"></i></div>
                    </div>
                </div>
            </div>
            <div class="row g-4 mb-4">
                <div class="col-md-3">
                    <div class="stat-card">
                        <div class="stat-label">Tổng Giá Trị</div>
                        <div class="stat-value text-primary" id="totalValue">0 ₫</div>
                    </div>
                </div>
                <div class="col-md-3">
                    <div class="stat-card">
                        <div class="stat-label">Tổng Số Lượng</div>
                        <div class="stat-value text-success" id="totalQty">0</div>
                    </div>
                </div>
                <div class="col-md-3">
                    <div class="stat-card">
                        <div class="stat-label">Lượt Quét RFID</div>
                        <div class="stat-value text-warning" id="scanCount">0</div>
                    </div>
                </div>
                <div class="col-md-3">
                    <div class="stat-card">
                        <div class="stat-label">Thẻ Gần Nhất</div>
                        <div class="stat-value fs-5" id="lastRfid">---</div>
                    </div>
                </div>
            </div>
            
            <div class="row g-4">
                <div class="col-md-8">
                    <div class="stat-card">
                        <h5 class="mb-4">Biểu Đồ Tồn Kho</h5>
                        <div id="revenueChart" style="height: 300px;"></div>
                    </div>
                </div>
                <div class="col-md-4">
                    <div class="stat-card">
                        <h5 class="mb-4">Cơ Cấu Hàng Hóa</h5>
                        <div id="donutChart" style="height: 300px;"></div>
                    </div>
                </div>
            </div>
        </div>

        <div id="inventory" class="page-section d-none">
            <div class="glass-table">
                <div class="d-flex justify-content-between mb-4 align-items-center">
                    <h4 class="m-0">Danh Sách Hàng Hóa Chi Tiết</h4>
                    <button class="btn btn-primary rounded-pill px-4" onclick="fetchData()"><i class="fa-solid fa-rotate me-2"></i> Cập nhật</button>
                </div>
                <table id="invTable" class="table table-dark table-hover w-100 align-middle">
                    <thead>
                        <tr>
                            <th>Tên Sản Phẩm</th>
                            <th>Số Lượng</th>
                            <th>Đơn Giá</th>
                            <th>Thành Tiền</th>
                            <th>Mã RFID</th>
                        </tr>
                    </thead>
                    <tbody id="invTableBody"></tbody>
                </table>
            </div>
        </div>

        <div id="operations" class="page-section d-none">
            <div class="row mb-5">
                <div class="col-12 text-center">
                    <h4 class="mb-4">Bảng Điều Khiển Chế Độ Hoạt Động</h4>
                </div>
                <div class="col-md-4">
                    <div class="mode-btn active" onclick="setMode(0)" id="btn-mode-0">
                        <i class="fa-solid fa-magnifying-glass text-info"></i>
                        <h5>CHẾ ĐỘ KIỂM TRA</h5>
                        <small class="text-muted">Quét thẻ để xem thông tin sản phẩm</small>
                    </div>
                </div>
                <div class="col-md-4">
                    <div class="mode-btn" onclick="setMode(1)" id="btn-mode-1">
                        <i class="fa-solid fa-file-import text-success"></i>
                        <h5>CHẾ ĐỘ NHẬP KHO</h5>
                        <small class="text-muted">Tự động tăng số lượng khi quét thẻ</small>
                    </div>
                </div>
                <div class="col-md-4">
                    <div class="mode-btn" onclick="setMode(2)" id="btn-mode-2">
                        <i class="fa-solid fa-file-export text-danger"></i>
                        <h5>CHẾ ĐỘ XUẤT KHO</h5>
                        <small class="text-muted">Tự động giảm số lượng khi quét thẻ</small>
                    </div>
                </div>
            </div>

            <div class="row g-4">
                <div class="col-md-6">
                    <div class="stat-card">
                        <h5 class="mb-3"><i class="fa-solid fa-plus-circle me-2"></i> Nhập Kho Thủ Công</h5>
                        <hr class="border-secondary">
                        <input type="text" id="imName" class="form-control bg-dark text-white border-secondary mb-3" placeholder="Tên sản phẩm (VD: Laptop)">
                        <div class="row">
                            <div class="col-6">
                                <input type="number" id="imQty" class="form-control bg-dark text-white border-secondary mb-3" value="1">
                            </div>
                            <div class="col-6">
                                <input type="text" id="imRfid" class="form-control bg-dark text-white border-secondary mb-3" placeholder="Mã RFID (Tùy chọn)">
                            </div>
                        </div>
                        <button class="btn btn-success w-100 py-2" onclick="manualAction('IMPORT')">THỰC HIỆN NHẬP</button>
                    </div>
                </div>
                <div class="col-md-6">
                    <div class="stat-card">
                        <h5 class="mb-3"><i class="fa-solid fa-minus-circle me-2"></i> Xuất Kho Thủ Công</h5>
                        <hr class="border-secondary">
                        <input type="text" id="exName" class="form-control bg-dark text-white border-secondary mb-3" placeholder="Tên sản phẩm cần xuất">
                        <input type="number" id="exQty" class="form-control bg-dark text-white border-secondary mb-3" value="1">
                        <button class="btn btn-danger w-100 py-2 mt-auto" onclick="manualAction('EXPORT')">THỰC HIỆN XUẤT</button>
                    </div>
                </div>
            </div>
        </div>

    </div>

    <script src="https://code.jquery.com/jquery-3.7.0.min.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/apexcharts"></script>
    <script src="https://cdn.datatables.net/1.13.4/js/jquery.dataTables.min.js"></script>
    <script src="https://cdn.datatables.net/1.13.4/js/dataTables.bootstrap5.min.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/sweetalert2@11"></script>

    <script>
        let gateway = `ws://${window.location.hostname}/ws`;
        let websocket;
        let inventoryData = [];
        let scanCount = 0;
        let chartRev, chartPie;

        window.onload = () => { initWebSocket(); fetchData(); };

        function initWebSocket() {
            websocket = new WebSocket(gateway);
            websocket.onopen = () => {
                document.getElementById('connStatus').innerHTML = '<i class="fa-solid fa-wifi"></i> Đã kết nối';
                document.getElementById('connStatus').style.color = '#00ff88';
            };
            websocket.onclose = () => {
                document.getElementById('connStatus').innerHTML = '<i class="fa-solid fa-wifi"></i> Mất kết nối';
                document.getElementById('connStatus').style.color = '#ef4444';
                setTimeout(initWebSocket, 2000);
            };
            websocket.onmessage = (event) => {
                let msg = event.data;
                if(msg.startsWith("RFID:")) {
                    let id = msg.substring(5);
                    document.getElementById('lastRfid').innerText = id;
                    document.getElementById('scanCount').innerText = ++scanCount;
                    Swal.fire({ icon: 'info', title: 'Đã quét thẻ', text: id, toast: true, position: 'top-end', timer: 1500, showConfirmButton: false, background: '#1f2937', color: '#fff' });
                    fetchData();
                } else if (msg.startsWith("MODE:")) {
                    updateModeUI(parseInt(msg.substring(5)));
                } else if (msg.startsWith("SENSOR:")) {
                    // XỬ LÝ DỮ LIỆU SENSOR
                    let parts = msg.split(":");
                    let temp = parseFloat(parts[1]);
                    let hum = parseFloat(parts[2]);
                    
                    // Cập nhật số
                    document.getElementById('valTemp').innerText = temp.toFixed(1);
                    document.getElementById('valHum').innerText = hum.toFixed(0);
                    
                    // Logic cảnh báo nhiệt độ
                    let stsT = document.getElementById('stsTemp');
                    if(temp > 40) {
                        stsT.innerText = "CẢNH BÁO: QUÁ NHIỆT!";
                        stsT.className = "badge rounded-pill bg-danger blink-anim";
                    } else {
                        stsT.innerText = "Hoạt động ổn định";
                        stsT.className = "badge rounded-pill bg-dark border border-danger text-danger";
                    }
                    
                    document.getElementById('stsHum').innerText = "Độ ẩm bình thường";
                }
            };
        }

        function fetchData() {
            fetch('/data').then(res => res.json()).then(data => {
                inventoryData = data;
                renderTable();
                renderCharts();
            });
        }

        function renderTable() {
            if ($.fn.DataTable.isDataTable('#invTable')) $('#invTable').DataTable().destroy();
            let tbody = document.getElementById("invTableBody");
            tbody.innerHTML = "";
            let totalQ = 0, totalV = 0;

            inventoryData.forEach(item => {
                totalQ += item.qty;
                totalV += item.qty * item.price;
                tbody.innerHTML += `<tr>
                    <td class="fw-bold">${item.name}</td>
                    <td><span class="badge bg-primary fs-6">${item.qty}</span></td>
                    <td>${item.price.toLocaleString()}</td>
                    <td class="text-success fw-bold">${(item.qty * item.price).toLocaleString()}</td>
                    <td><span class="badge bg-secondary font-monospace">${item.rfid}</span></td>
                </tr>`;
            });

            document.getElementById("totalQty").innerText = totalQ;
            document.getElementById("totalValue").innerText = totalV.toLocaleString() + " ₫";

            $('#invTable').DataTable({ pageLength: 5, language: { search: "Tìm kiếm:", paginate: { next: ">", previous: "<" } } });
        }

        function renderCharts() {
            let names = inventoryData.map(i => i.name);
            let qtys = inventoryData.map(i => i.qty);

            if(chartRev) chartRev.destroy();
            chartRev = new ApexCharts(document.querySelector("#revenueChart"), {
                series: [{ name: 'Số lượng', data: qtys }],
                chart: { type: 'area', height: 300, background: 'transparent', toolbar: { show: false } },
                theme: { mode: 'dark' },
                colors: ['#0d6efd'],
                stroke: { curve: 'smooth', width: 3 },
                fill: { type: 'gradient', gradient: { shadeIntensity: 1, opacityFrom: 0.7, opacityTo: 0.3 } },
                xaxis: { categories: names, labels: { style: { colors: '#9ca3af' } } },
                yaxis: { labels: { style: { colors: '#9ca3af' } } },
                grid: { borderColor: '#374151' }
            });
            chartRev.render();

            if(chartPie) chartPie.destroy();
            chartPie = new ApexCharts(document.querySelector("#donutChart"), {
                series: qtys,
                labels: names,
                chart: { type: 'donut', height: 300, background: 'transparent' },
                theme: { mode: 'dark' },
                legend: { position: 'bottom' },
                stroke: { show: false }
            });
            chartPie.render();
        }

        function setMode(mode) {
            fetch('/setmode?val=' + mode);
            updateModeUI(mode);
        }

        function updateModeUI(mode) {
            document.querySelectorAll('.mode-btn').forEach(b => b.classList.remove('active'));
            document.getElementById('btn-mode-' + mode).classList.add('active');
            let txt = ["CHECK (Kiểm tra)", "IMPORT (Nhập)", "EXPORT (Xuất)"];
            document.getElementById('sysModeBadge').innerText = "Chế độ: " + txt[mode];
        }

        function manualAction(type) {
            let name = document.getElementById(type === 'IMPORT' ? 'imName' : 'exName').value;
            let qty = document.getElementById(type === 'IMPORT' ? 'imQty' : 'exQty').value;
            let rfid = type === 'IMPORT' ? (document.getElementById('imRfid').value || "0") : "0";
            
            if(!name) return Swal.fire('Lỗi', 'Vui lòng nhập tên sản phẩm', 'error');

            fetch(`/action?type=${type}&name=${name}&qty=${qty}&rfid=${rfid}`)
                .then(res => {
                    Swal.fire('Thành công', 'Đã gửi lệnh ' + type, 'success');
                    fetchData();
                });
        }

        function showSection(id, el) {
            document.querySelectorAll('.page-section').forEach(d => d.classList.add('d-none'));
            document.getElementById(id).classList.remove('d-none');
            document.querySelectorAll('.nav-link').forEach(l => l.classList.remove('active'));
            el.classList.add('active');
        }
    </script>
</body>
</html>
)rawliteral";

// --- WEBSOCKET EVENT ---
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        client->text("MODE:" + String(currentSystemMode));
        // Gửi ngay dữ liệu sensor khi vừa kết nối
        String msg = "SENSOR:" + String(currentTemperature, 1) + ":" + String(currentHumidity, 0);
        client->text(msg);
    }
}

void setupWebServer() {
    // 1. Root
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html);
    });

    // 2. Data API (Trả về JSON) - ĐÃ SỬA LOGIC C++
    server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
        if(xSemaphoreTake(xInventoryMutex, portMAX_DELAY) == pdTRUE) {
            DynamicJsonDocument doc(8192);
            JsonArray array = doc.to<JsonArray>();

            for (int i = 0; i < myWarehouse->size(); i++) {
                JsonObject item = array.createNestedObject();
                item["name"] = myWarehouse->getProductName(i);
                item["qty"] = myWarehouse->getProductQuantity(i);
                item["price"] = myWarehouse->getProductPrice(i); // Lấy giá trực tiếp
                item["rfid"] = myWarehouse->getProductRFID(i);   // Lấy RFID trực tiếp
            }
            xSemaphoreGive(xInventoryMutex);
            String response;
            serializeJson(doc, response);
            request->send(200, "application/json", response);
        } else {
            request->send(500, "text/plain", "Busy");
        }
    });

    // 3. Set Mode API
    server.on("/setmode", HTTP_GET, [](AsyncWebServerRequest *request){
        if(request->hasParam("val")) {
            int mode = request->getParam("val")->value().toInt();
            currentSystemMode = mode;
            updateLeds();
            myWS.textAll("MODE:" + String(mode));
            
            // Gửi lệnh cho LCD
            if (g_queueManager_to_LCD != NULL) {
                SystemMessage msg;
                msg.type = EVENT_UPDATE_LCD;
                String line1 = "MODE CHANGED:";
                String line2 = (mode == 1) ? ">> IMPORT" : ((mode == 2) ? ">> EXPORT" : ">> CHECK");
                String fullMsg = line1 + "|" + line2;
                strncpy(msg.payload, fullMsg.c_str(), sizeof(msg.payload) - 1);
                xQueueSend(g_queueManager_to_LCD, &msg, 10);
            }
        }
        request->send(200, "text/plain", "OK");
    });

    // 4. Generic Action API (Manual Import/Export) - ĐÃ SỬA LOGIC C++
    server.on("/action", HTTP_GET, [](AsyncWebServerRequest *request){
        if(request->hasParam("type") && request->hasParam("name")) {
            String type = request->getParam("type")->value();
            String name = request->getParam("name")->value();
            int qty = request->hasParam("qty") ? request->getParam("qty")->value().toInt() : 1;
            String rfidHex = request->hasParam("rfid") ? request->getParam("rfid")->value() : "0";
            
            if(xSemaphoreTake(xInventoryMutex, portMAX_DELAY) == pdTRUE) {
                // Dùng hàm tìm kiếm mới trong InventoryManager
                int index = myWarehouse->findIndexByName(name);
                
                if (type == "IMPORT") {
                    if (index != -1) {
                        // Đã có -> Cộng dồn
                        int oldQty = myWarehouse->getProductQuantity(index);
                        myWarehouse->updateQuantity(index, oldQty + qty);
                    } else {
                        // Chưa có -> Thêm mới (Giá mặc định 0)
                        myWarehouse->addProduct(name, qty, 0.0, rfidHex);
                    }
                } 
                else if (type == "EXPORT") {
                    if (index != -1) {
                        int oldQty = myWarehouse->getProductQuantity(index);
                        if (oldQty >= qty) {
                            myWarehouse->updateQuantity(index, oldQty - qty);
                        }
                    }
                }
                xSemaphoreGive(xInventoryMutex);
            }
            request->send(200, "text/plain", "OK");
        } else {
            request->send(400, "text/plain", "Missing Params");
        }
    });

    myWS.onEvent(onEvent);
    server.addHandler(&myWS);
    server.begin();
    Serial.println("Web Server Started");
}

void TaskWebServerFunc(void *pvParameters) {
    xSemaphoreTake(xBinarySemaphoreInternet, portMAX_DELAY);
    xSemaphoreGive(xBinarySemaphoreInternet); 
    setupWebServer();
    
    // Loop chính của WebServer Task
    static unsigned long lastSensorTime = 0;
    
    while(1) {
        // Gửi RFID nếu có thẻ mới
        if (hasNewTag) {
            myWS.textAll("RFID:" + lastScannedRFID);
            hasNewTag = false;
        }
        
        // --- GỬI DỮ LIỆU SENSOR MỖI 2 GIÂY ---
        if (millis() - lastSensorTime > 2000) {
            lastSensorTime = millis();
            // Format tin nhắn: SENSOR:30.5:60 (Temp:Humid)
            String msg = "SENSOR:" + String(currentTemperature, 1) + ":" + String(currentHumidity, 0);
            myWS.textAll(msg);
        }
        // -------------------------------------

        myWS.cleanupClients();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}