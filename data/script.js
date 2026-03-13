// ==========================================
// CẤU HÌNH ADAFRUIT IO
// ==========================================
const AIO_USERNAME = ""; 
const AIO_KEY = "";    
const BASE_URL = ``;

let inventoryData = [];
let scanCount = 0;
let chartRev, chartPie;

window.onload = () => { 
    // Trạng thái ban đầu
    document.getElementById('connStatus').innerHTML = '<i class="fa-solid fa-spinner fa-spin"></i> Đang kết nối...';
    document.getElementById('connStatus').className = "fw-bold text-warning";
    
    fetchData(); 
   
};

// ==========================================
// CÁC HÀM GIAO TIẾP VỚI CLOUD
// ==========================================
async function getFeed(feedName) {
    try {
        let res = await fetch(`${BASE_URL}/${feedName}/data/last`, { headers: { 'X-AIO-Key': AIO_KEY } });
        if(res.ok) {
            let json = await res.json();
            return json.value;
        }
    } catch(e) { }
    return null;
}

async function postFeed(feedName, value) {
    try {
        await fetch(`${BASE_URL}/${feedName}/data`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json', 'X-AIO-Key': AIO_KEY },
            body: JSON.stringify({ datum: { value: value } })
        });
    } catch(e) { console.error("Lỗi POST data:", e); }
}

async function fetchData() {
    try {
        // 1. Lấy Sensor
        let temp = await getFeed("iot-temp"); // Đã sửa tên feed
        let humi = await getFeed("iot-humi"); // Đã sửa tên feed
        if(temp !== null && humi !== null) {
            updateSensorUI(parseFloat(temp), parseFloat(humi));
        }

        // 2. Lấy RFID
        let rfid = await getFeed("iot-rfid"); // Đã sửa tên feed
        // ... (Giữ nguyên đoạn code xử lý rfid) ...

        // 3. Lấy Mode từ Cloud để đồng bộ UI
        let currentMode = await getFeed("iot-mode"); // Đã sửa tên feed
        if(currentMode !== null) updateModeUI(parseInt(currentMode));

        // 4. Lấy Database kho hàng
        let dbStr = await getFeed("iot-database");
        if(dbStr) {
            let newData = JSON.parse(dbStr);
            // Chỉ vẽ lại Bảng và Biểu đồ nếu dữ liệu thực sự có sự thay đổi
            if(JSON.stringify(newData) !== JSON.stringify(inventoryData)) {
                inventoryData = newData;
                renderTable();
                renderCharts();
            }
        }

        // Báo kết nối thành công
        document.getElementById('connStatus').innerHTML = '<i class="fa-solid fa-cloud"></i> Đã kết nối Cloud';
        document.getElementById('connStatus').className = "fw-bold text-success";
    } catch (error) {
        document.getElementById('connStatus').innerHTML = '<i class="fa-solid fa-wifi"></i> Mất kết nối';
        document.getElementById('connStatus').className = "fw-bold text-danger";
    }
}

// ==========================================
// CÁC HÀM CẬP NHẬT GIAO DIỆN (GIỮ NGUYÊN BẢN GỐC)
// ==========================================
function updateSensorUI(temp, hum) {
    document.getElementById('valTempText').innerText = temp.toFixed(1) + " °C";
    document.getElementById('valHumText').innerText = hum.toFixed(0) + " %";

    // Nhiệt độ (Thang 20 -> 50)
    let tempPercent = Math.min(Math.max((temp - 20) / (50 - 20) * 100, 0), 100);
    let r_t = 255; 
    let g_t = Math.floor(200 - (tempPercent * 2)); if(g_t < 0) g_t = 0;
    let tempColor = `rgb(${r_t}, ${g_t}, 0)`;
    
    document.getElementById('progTemp').style.width = tempPercent + "%";
    document.getElementById('progTemp').style.backgroundColor = tempColor;
    
    let iconBoxT = document.getElementById('iconBoxTemp');
    iconBoxT.style.color = tempColor;
    iconBoxT.style.backgroundColor = `rgba(${r_t}, ${g_t}, 0, 0.15)`;
    if(temp > 40) iconBoxT.classList.add('pulse-active'); else iconBoxT.classList.remove('pulse-active');

    // Độ ẩm
    let humPercent = hum;
    let humColor = `rgb(0, ${150 - humPercent}, ${255})`;
    document.getElementById('progHum').style.width = humPercent + "%";
    document.getElementById('progHum').style.backgroundColor = humColor;
    
    let iconBoxH = document.getElementById('iconBoxHum');
    iconBoxH.style.color = humColor;
    iconBoxH.style.backgroundColor = `rgba(0, 150, 255, 0.1)`;
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

function updateModeUI(mode) {
    document.querySelectorAll('.mode-btn').forEach(b => b.classList.remove('active'));
    let btn = document.getElementById('btn-mode-' + mode);
    if(btn) btn.classList.add('active');
    let txt = ["CHECK", "IMPORT", "EXPORT"];
    let badge = document.getElementById('sysModeBadge');
    if(badge) badge.innerText = "MODE: " + txt[mode];
}

function toggleSidebar() {
    if(window.innerWidth <= 768) {
        document.getElementById('sidebar').classList.toggle('active');
        document.getElementById('overlay').classList.toggle('active');
    }
}

function showSection(id, el) {
    document.querySelectorAll('.page-section').forEach(d => d.classList.add('d-none'));
    let target = document.getElementById(id);
    if(target) target.classList.remove('d-none');
    document.querySelectorAll('.nav-link').forEach(l => l.classList.remove('active'));
    if(el) el.classList.add('active');
}

// ==========================================
// CÁC HÀM GỬI LỆNH ĐIỀU KHIỂN
// ==========================================
function setMode(mode) { 
    postFeed("iot-mode", mode);
    updateModeUI(mode); 
}

function manualAction(type) {
    let name = document.getElementById(type === 'IMPORT' ? 'imName' : 'exName').value;
    let qty = document.getElementById(type === 'IMPORT' ? 'imQty' : 'exQty').value;
    let rfid = type === 'IMPORT' ? (document.getElementById('imRfid').value || "0") : "0";
    
    if(!name) return Swal.fire('Lỗi', 'Nhập tên sản phẩm', 'error');
    
   // Đóng gói chuỗi hành động để gửi lên Cloud (Ví dụ: IMPORT:BanhQuy:5:0)
    let actionStr = `${type}:${name}:${qty}:${rfid}`;
    
    postFeed("iot-action", actionStr).then(() => { // Đã sửa tên feed
        Swal.fire('Thành công', 'Đã gửi lệnh ' + type, 'success'); 
        fetchData(); // Quét lại để cập nhật bảng
    });
}
// ==========================================
// KẾT NỐI MQTT WEBSOCKETS (REAL-TIME)
// ==========================================
const mqttClient = mqtt.connect('wss://io.adafruit.com:443/mqtt', {
    username: AIO_USERNAME,
    password: AIO_KEY
});

mqttClient.on('connect', function () {
    console.log("Đã kết nối MQTT WebSockets!");
    document.getElementById('connStatus').innerHTML = '<i class="fa-solid fa-bolt"></i> Đã kết nối Real-time';
    document.getElementById('connStatus').className = "fw-bold text-success";

    // Đăng ký nhận thông báo từ các feed (Cú pháp của Adafruit: username/f/feedname)
    mqttClient.subscribe(`${AIO_USERNAME}/f/iot-temp`);
    mqttClient.subscribe(`${AIO_USERNAME}/f/iot-humi`);
    mqttClient.subscribe(`${AIO_USERNAME}/f/iot-rfid`);
    mqttClient.subscribe(`${AIO_USERNAME}/f/iot-database`);
    mqttClient.subscribe(`${AIO_USERNAME}/f/iot-mode`);
});

// Sự kiện được kích hoạt NGAY LẬP TỨC khi có dữ liệu mới trên Cloud
mqttClient.on('message', function (topic, message) {
    let val = message.toString();
    let feed = topic.split('/').pop(); // Lấy tên feed ở cuối đoạn URL

    if (feed === 'iot-temp') {
        let currentHum = parseFloat(document.getElementById('valHumText').innerText) || 0;
        updateSensorUI(parseFloat(val), currentHum);
    } 
    else if (feed === 'iot-humi') {
        let currentTemp = parseFloat(document.getElementById('valTempText').innerText) || 0;
        updateSensorUI(currentTemp, parseFloat(val));
    } 
    else if (feed === 'iot-rfid') {
        let rfidEl = document.getElementById('lastRfid');
        if (val && val !== rfidEl.innerText && val !== "0") {
            rfidEl.innerText = val;
            document.getElementById('scanCount').innerText = ++scanCount;
            Swal.fire({ icon: 'info', title: 'Đã quét thẻ', text: val, toast: true, position: 'top-end', timer: 1500, showConfirmButton: false });
        }
    } 
    else if (feed === 'iot-database') {
        let newData = JSON.parse(val);
        if (JSON.stringify(newData) !== JSON.stringify(inventoryData)) {
            inventoryData = newData;
            renderTable();
            renderCharts();
        }
    } 
    else if (feed === 'iot-mode') {
        updateModeUI(parseInt(val));
    }
});