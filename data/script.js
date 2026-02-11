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