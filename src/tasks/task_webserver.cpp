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
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Hệ Thống Kho Thông Minh</title>
    
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css" rel="stylesheet">
    <link href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css" rel="stylesheet">
    <link href="https://cdn.datatables.net/1.13.4/css/dataTables.bootstrap5.min.css" rel="stylesheet">
    <link href="https://fonts.googleapis.com/css2?family=Poppins:wght@300;400;500;600;700&display=swap" rel="stylesheet">

    <style>
        :root {
            --sidebar-width: 260px;
            --primary-color: #435ebe;
            --bg-body: #f2f7ff;
            --text-main: #25396f;
            --text-muted: #7c8db5;
            --transition-speed: 0.3s;
        }

        body { 
            font-family: 'Poppins', sans-serif; 
            background-color: var(--bg-body); 
            color: var(--text-main); 
            overflow-x: hidden; 
        }

        /* --- SIDEBAR --- */
        #sidebar {
            width: var(--sidebar-width);
            height: 100vh;
            position: fixed; left: 0; top: 0;
            background: #fff; z-index: 1000;
            transition: var(--transition-speed);
            box-shadow: 4px 0 20px rgba(0,0,0,0.05);
        }
        .brand-logo { padding: 30px 24px; font-size: 22px; font-weight: 700; color: var(--primary-color); display: flex; align-items: center; gap: 12px; }
        .nav-link { color: var(--text-muted); padding: 16px 24px; font-weight: 500; border-radius: 10px; margin: 0 10px; transition: all 0.2s; display: flex; align-items: center; gap: 12px; }
        .nav-link:hover { color: var(--primary-color); background: #f0f4ff; }
        .nav-link.active { background: var(--primary-color); color: #fff; box-shadow: 0 4px 12px rgba(67, 94, 190, 0.3); }

        /* --- CONTENT --- */
        #content { margin-left: var(--sidebar-width); padding: 24px; transition: var(--transition-speed); }

        /* --- CARDS --- */
        .stat-card {
            background: #fff; border-radius: 16px; padding: 24px;
            box-shadow: 0 5px 15px rgba(0,0,0,0.03); border: none; height: 100%;
            transition: transform 0.2s; position: relative; overflow: hidden;
        }
        .stat-card:hover { transform: translateY(-5px); }
        .stat-value { font-size: 28px; font-weight: 700; margin-top: 5px; color: #25396f; }
        .stat-label { font-size: 13px; color: var(--text-muted); font-weight: 600; text-transform: uppercase; }

        /* --- SENSOR DYNAMIC STYLE --- */
        .sensor-card { position: relative; overflow: hidden; }
        
        /* Icon background dynamic opacity */
        .sensor-icon-box {
            width: 60px; height: 60px; border-radius: 12px;
            display: flex; align-items: center; justify-content: center;
            font-size: 24px; transition: all 0.5s ease;
        }

        /* Progress Bar Custom */
        .sensor-progress-bg {
            height: 6px; width: 100%; background: #edf2f7;
            border-radius: 10px; margin-top: 15px; overflow: hidden;
        }
        .sensor-progress-fill {
            height: 100%; width: 0%; border-radius: 10px;
            transition: width 0.5s ease, background-color 0.5s ease;
        }

        /* Animation Pulse cho khi giá trị cao */
        @keyframes pulse-red { 0% { box-shadow: 0 0 0 0 rgba(220, 53, 69, 0.4); } 70% { box-shadow: 0 0 0 10px rgba(220, 53, 69, 0); } 100% { box-shadow: 0 0 0 0 rgba(220, 53, 69, 0); } }
        .pulse-active { animation: pulse-red 2s infinite; }

        /* --- MOBILE --- */
        .sidebar-toggle { display: none; font-size: 24px; cursor: pointer; color: var(--text-main); margin-right: 15px; }
        .overlay { display: none; position: fixed; top: 0; left: 0; width: 100%; height: 100%; background: rgba(0,0,0,0.5); z-index: 999; }
        @media (max-width: 768px) {
            #sidebar { left: -260px; } #sidebar.active { left: 0; }
            #content { margin-left: 0; padding: 15px; }
            .sidebar-toggle { display: block; } .overlay.active { display: block; }
        }
        
        /* Table & Utils */
        table.dataTable thead th { border-bottom: 2px solid #edf2f7 !important; color: var(--text-muted); font-weight: 600; }
        table.dataTable td { border-bottom: 1px solid #edf2f7; padding: 16px 10px !important; }
        .mode-btn { background: #fff; border: 2px solid #edf2f7; padding: 20px; border-radius: 16px; text-align: center; cursor: pointer; transition: 0.2s; height: 100%; }
        .mode-btn.active { border-color: var(--primary-color); background: #f0f4ff; }
    </style>
</head>
<body>

    <div class="overlay" id="overlay" onclick="toggleSidebar()"></div>

    <nav id="sidebar">
        <div class="brand-logo"><i class="fa-solid fa-cube text-primary"></i> SMART WAREHOUSE</div>
        <ul class="nav flex-column mt-3">
            <li class="nav-item mb-2"><a href="#" class="nav-link active" onclick="showSection('dashboard', this); toggleSidebar()"><i class="fa-solid fa-chart-pie"></i> Tổng Quan</a></li>
            <li class="nav-item mb-2"><a href="#" class="nav-link" onclick="showSection('inventory', this); toggleSidebar()"><i class="fa-solid fa-boxes-stacked"></i> Kho Hàng</a></li>
            <li class="nav-item mb-2"><a href="#" class="nav-link" onclick="showSection('operations', this); toggleSidebar()"><i class="fa-solid fa-gamepad"></i> Điều Khiển</a></li>
        </ul>
        <div style="position:absolute; bottom:20px; width:100%; padding:20px;">
            <div class="p-3 text-center bg-light rounded-3">
                <small id="connStatus" class="fw-bold text-danger"><i class="fa-solid fa-wifi"></i> Mất kết nối</small>
            </div>
        </div>
    </nav>

    <div id="content">
        <div class="d-flex justify-content-between align-items-center mb-4 bg-white p-3 rounded-4 shadow-sm">
            <div class="d-flex align-items-center">
                <i class="fa-solid fa-bars sidebar-toggle" onclick="toggleSidebar()"></i>
                <div>
                    <h4 class="fw-bold m-0 text-primary">Dashboard</h4>
                </div>
            </div>
            <span class="badge bg-primary bg-opacity-10 text-primary fs-6 px-4 py-2 rounded-pill" id="sysModeBadge">MODE: CHECK</span>
        </div>

        <div id="dashboard" class="page-section">
            <div class="row g-4 mb-4">
                <div class="col-md-6">
                    <div class="stat-card sensor-card">
                        <div class="d-flex justify-content-between align-items-center mb-2">
                            <div>
                                <div class="stat-label">Nhiệt Độ Kho</div>
                                <div class="stat-value" id="valTempText" style="transition: color 0.5s">-- °C</div>
                            </div>
                            <div class="sensor-icon-box" id="iconBoxTemp" style="background: #fff5f5; color: #ff6b6b;">
                                <i class="fa-solid fa-temperature-half"></i>
                            </div>
                        </div>
                        <div class="sensor-progress-bg">
                            <div class="sensor-progress-fill" id="progTemp" style="width: 0%; background: #ff6b6b;"></div>
                        </div>
                    </div>
                </div>

                <div class="col-md-6">
                    <div class="stat-card sensor-card">
                        <div class="d-flex justify-content-between align-items-center mb-2">
                            <div>
                                <div class="stat-label">Độ Ẩm Không Khí</div>
                                <div class="stat-value" id="valHumText" style="transition: color 0.5s">-- %</div>
                            </div>
                            <div class="sensor-icon-box" id="iconBoxHum" style="background: #f0f7ff; color: #4dabf7;">
                                <i class="fa-solid fa-droplet"></i>
                            </div>
                        </div>
                        <div class="sensor-progress-bg">
                            <div class="sensor-progress-fill" id="progHum" style="width: 0%; background: #4dabf7;"></div>
                        </div>
                    </div>
                </div>
            </div>

            <div class="row g-4 mb-4">
                <div class="col-6 col-xl-3">
                    <div class="stat-card">
                        <div class="stat-label">Tổng Giá Trị</div>
                        <div class="stat-value text-primary" id="totalValue">0 ₫</div>
                    </div>
                </div>
                <div class="col-6 col-xl-3">
                    <div class="stat-card">
                        <div class="stat-label">Tổng Số Lượng</div>
                        <div class="stat-value text-success" id="totalQty">0</div>
                    </div>
                </div>
                <div class="col-6 col-xl-3">
                    <div class="stat-card">
                        <div class="stat-label">Lượt Quét RFID</div>
                        <div class="stat-value text-warning" id="scanCount">0</div>
                    </div>
                </div>
                <div class="col-6 col-xl-3">
                    <div class="stat-card">
                        <div class="stat-label">Thẻ Gần Nhất</div>
                        <div class="stat-value fs-5 text-dark" id="lastRfid">---</div>
                    </div>
                </div>
            </div>
            
            <div class="row g-4">
                <div class="col-md-8">
                    <div class="stat-card">
                        <h5 class="fw-bold mb-4 text-secondary">Biểu Đồ Tồn Kho</h5>
                        <div id="revenueChart" style="height: 300px;"></div>
                    </div>
                </div>
                <div class="col-md-4">
                    <div class="stat-card">
                        <h5 class="fw-bold mb-4 text-secondary">Cơ Cấu Hàng Hóa</h5>
                        <div id="donutChart" style="height: 300px;"></div>
                    </div>
                </div>
            </div>
        </div>

        <div id="inventory" class="page-section d-none">
            <div class="stat-card">
                <div class="d-flex justify-content-between mb-4 align-items-center">
                    <h5 class="fw-bold m-0">Danh Sách Hàng Hóa</h5>
                    <button class="btn btn-primary rounded-pill px-4 btn-sm" onclick="fetchData()"><i class="fa-solid fa-rotate me-2"></i> Làm mới</button>
                </div>
                <div class="table-responsive">
                    <table id="invTable" class="table table-hover w-100 align-middle">
                        <thead class="bg-light">
                            <tr><th>Tên Sản Phẩm</th><th>Số Lượng</th><th>Đơn Giá</th><th>Thành Tiền</th><th>Mã RFID</th></tr>
                        </thead>
                        <tbody id="invTableBody"></tbody>
                    </table>
                </div>
            </div>
        </div>

        <div id="operations" class="page-section d-none">
            <div class="row mb-5 g-3">
                <div class="col-md-4"><div class="mode-btn active" onclick="setMode(0)" id="btn-mode-0"><i class="fa-solid fa-magnifying-glass text-primary"></i><h5 class="fw-bold">CHẾ ĐỘ KIỂM TRA</h5></div></div>
                <div class="col-md-4"><div class="mode-btn" onclick="setMode(1)" id="btn-mode-1"><i class="fa-solid fa-file-import text-success"></i><h5 class="fw-bold">CHẾ ĐỘ NHẬP KHO</h5></div></div>
                <div class="col-md-4"><div class="mode-btn" onclick="setMode(2)" id="btn-mode-2"><i class="fa-solid fa-file-export text-danger"></i><h5 class="fw-bold">CHẾ ĐỘ XUẤT KHO</h5></div></div>
            </div>
            <div class="row g-4">
                <div class="col-md-6">
                    <div class="stat-card h-100 border-start border-4 border-success">
                        <h5 class="mb-3 fw-bold text-success">Nhập Kho Thủ Công</h5>
                        <input type="text" id="imName" class="form-control mb-3" placeholder="Tên sản phẩm">
                        <div class="row mb-3">
                            <div class="col-6"><input type="number" id="imQty" class="form-control" value="1"></div>
                            <div class="col-6"><input type="text" id="imRfid" class="form-control" placeholder="RFID (Option)"></div>
                        </div>
                        <button class="btn btn-success w-100 fw-bold" onclick="manualAction('IMPORT')">NHẬP KHO</button>
                    </div>
                </div>
                <div class="col-md-6">
                    <div class="stat-card h-100 border-start border-4 border-danger">
                        <h5 class="mb-3 fw-bold text-danger">Xuất Kho Thủ Công</h5>
                        <input type="text" id="exName" class="form-control mb-3" placeholder="Tên sản phẩm">
                        <input type="number" id="exQty" class="form-control mb-3" value="1">
                        <button class="btn btn-danger w-100 fw-bold mt-auto" onclick="manualAction('EXPORT')">XUẤT KHO</button>
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
                document.getElementById('connStatus').className = "fw-bold text-success";
            };
            websocket.onclose = () => {
                document.getElementById('connStatus').innerHTML = '<i class="fa-solid fa-wifi"></i> Mất kết nối';
                document.getElementById('connStatus').className = "fw-bold text-danger";
                setTimeout(initWebSocket, 2000);
            };
            websocket.onmessage = (event) => {
                let msg = event.data;
                if(msg.startsWith("RFID:")) {
                    let id = msg.substring(5);
                    document.getElementById('lastRfid').innerText = id;
                    document.getElementById('scanCount').innerText = ++scanCount;
                    Swal.fire({ icon: 'info', title: 'Đã quét thẻ', text: id, toast: true, position: 'top-end', timer: 1500, showConfirmButton: false });
                    fetchData();
                } else if (msg.startsWith("MODE:")) {
                    updateModeUI(parseInt(msg.substring(5)));
                } else if (msg.startsWith("SENSOR:")) {
                    let parts = msg.split(":");
                    updateSensorUI(parseFloat(parts[1]), parseFloat(parts[2]));
                }
            };
        }

        // --- XỬ LÝ MÀU SẮC ĐỘNG (LOGIC MỚI) ---
        function updateSensorUI(temp, hum) {
            // Cập nhật text
            document.getElementById('valTempText').innerText = temp.toFixed(1) + " °C";
            document.getElementById('valHumText').innerText = hum.toFixed(0) + " %";

            // 1. Xử lý Nhiệt độ (Thang 20 -> 50)
            let tempPercent = Math.min(Math.max((temp - 20) / (50 - 20) * 100, 0), 100); // Quy đổi ra %
            
            // Màu từ Vàng (thấp) -> Cam -> Đỏ đậm (cao)
            let r_t = 255; 
            let g_t = Math.floor(200 - (tempPercent * 2)); // Giảm xanh lá để chuyển sang đỏ
            if(g_t < 0) g_t = 0;
            let tempColor = `rgb(${r_t}, ${g_t}, 0)`;
            
            // Cập nhật giao diện Temp
            document.getElementById('progTemp').style.width = tempPercent + "%";
            document.getElementById('progTemp').style.backgroundColor = tempColor;
            
            // Icon thay đổi
            let iconBoxT = document.getElementById('iconBoxTemp');
            iconBoxT.style.color = tempColor;
            iconBoxT.style.backgroundColor = `rgba(${r_t}, ${g_t}, 0, 0.15)`; // Nền nhạt cùng tông
            
            // Hiệu ứng Pulse khi nóng (>40 độ)
            if(temp > 40) iconBoxT.classList.add('pulse-active');
            else iconBoxT.classList.remove('pulse-active');

            // 2. Xử lý Độ ẩm (0 -> 100)
            let humPercent = hum;
            // Màu từ Xanh nhạt -> Xanh đậm
            // Blue value giữ nguyên, giảm Red/Green để đậm hơn
            let intensity = 255 - (humPercent * 2); // Càng ẩm càng tối
            if(intensity < 0) intensity = 0;
            let humColor = `rgb(0, ${150 - humPercent}, ${255})`; // Xanh dương biến thiên

            document.getElementById('progHum').style.width = humPercent + "%";
            document.getElementById('progHum').style.backgroundColor = humColor;

            let iconBoxH = document.getElementById('iconBoxHum');
            iconBoxH.style.color = humColor;
            iconBoxH.style.backgroundColor = `rgba(0, 150, 255, 0.1)`;
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
                totalQ += item.qty; totalV += item.qty * item.price;
                tbody.innerHTML += `<tr><td class="fw-bold text-primary">${item.name}</td><td><span class="badge bg-primary bg-opacity-10 text-primary">${item.qty}</span></td><td>${item.price.toLocaleString()}</td><td class="fw-bold text-success">${(item.qty * item.price).toLocaleString()}</td><td><span class="badge bg-secondary font-monospace">${item.rfid}</span></td></tr>`;
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
                chart: { type: 'area', height: 300, toolbar: { show: false } },
                colors: ['#435ebe'], fill: { type: 'gradient', gradient: { opacityFrom: 0.7, opacityTo: 0.1 } },
                xaxis: { categories: names }, stroke: { curve: 'smooth', width: 3 }
            });
            chartRev.render();

            if(chartPie) chartPie.destroy();
            chartPie = new ApexCharts(document.querySelector("#donutChart"), {
                series: qtys, labels: names, chart: { type: 'donut', height: 300 },
                colors: ['#435ebe', '#57caeb', '#5ddab4', '#9694ff'], legend: { position: 'bottom' }
            });
            chartPie.render();
        }

        function setMode(mode) { fetch('/setmode?val=' + mode); updateModeUI(mode); }
        function updateModeUI(mode) {
            document.querySelectorAll('.mode-btn').forEach(b => b.classList.remove('active'));
            document.getElementById('btn-mode-' + mode).classList.add('active');
            let txt = ["CHECK", "IMPORT", "EXPORT"];
            document.getElementById('sysModeBadge').innerText = "MODE: " + txt[mode];
        }

        function manualAction(type) {
            let name = document.getElementById(type === 'IMPORT' ? 'imName' : 'exName').value;
            let qty = document.getElementById(type === 'IMPORT' ? 'imQty' : 'exQty').value;
            let rfid = type === 'IMPORT' ? (document.getElementById('imRfid').value || "0") : "0";
            if(!name) return Swal.fire('Lỗi', 'Nhập tên sản phẩm', 'error');
            fetch(`/action?type=${type}&name=${name}&qty=${qty}&rfid=${rfid}`).then(() => {
                Swal.fire('Thành công', 'Đã gửi lệnh ' + type, 'success'); fetchData();
            });
        }
        
        function toggleSidebar() {
            if(window.innerWidth <= 768) {
                document.getElementById('sidebar').classList.toggle('active');
                document.getElementById('overlay').classList.toggle('active');
            }
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