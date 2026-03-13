import sys
import time
import serial
import json
from Adafruit_IO import MQTTClient

# ==========================================
# 1. CẤU HÌNH ADAFRUIT IO VÀ MẢNG FEED
# ==========================================
AIO_USERNAME = ""  
AIO_KEY = ""              

# Khai báo mảng Feed để quản lý tập trung
AIO_FEED_IDs = [
    "iot-temp",  # Index 0
    "iot-humi",     # Index 1
    "iot-rfid",      # Index 2
    "iot-mode",      # Index 3
    "iot-database",  # Index 4
    "iot-action"     # Index 5 (Xử lý các nút bấm Nhập/Xuất thủ công từ Web)
]

# ==========================================
# 2. CƠ SỞ DỮ LIỆU KHO HÀNG
# ==========================================
database = {
    "195623B3": {"name": "iPhone 15", "qty": 5, "price": 25000000},
    "19D01AB3": {"name": "Noi Com Dien", "qty": 10, "price": 500000},
    "297124B3": {"name": "May Xay", "qty": 8, "price": 350000},
    "59B4DCC2": {"name": "May Khoan", "qty": 12, "price": 450000},
    "66E5C901": {"name": "Den Ban", "qty": 20, "price": 150000},
    "191D1BB3": {"name": "Quat Dung", "qty": 15, "price": 250000},
    "2D0F7506": {"name": "MacBook Pro", "qty": 3, "price": 45000000}
}
current_mode = 0 # 0: Check, 1: Import, 2: Export

# ==========================================
# 3. KẾT NỐI SERIAL
# ==========================================
SERIAL_PORT = "COM6" 
BAUD_RATE = 115200
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE)
except Exception as e:
    print(f"[!] Lỗi mở cổng Serial: {e}")
    sys.exit()

# ==========================================
# 4. HÀM XỬ LÝ LÔ-GÍC KHO HÀNG
# ==========================================
def pushDatabaseToWeb():
    # Bắn mảng JSON lên Adafruit IO để Web Frontend vẽ lại bảng
    db_list = [{"rfid": k, **v} for k, v in database.items()]
    client.publish(AIO_FEED_IDs[4], json.dumps(db_list))

def processInventory(rfid):
    lcd_line1 = ""
    lcd_line2 = ""
    
    if rfid in database:
        item = database[rfid]
        if current_mode == 1:   # Import
            item["qty"] += 1
            lcd_line1 = f"IMP: {item['name']}"
        elif current_mode == 2: # Export
            if item["qty"] > 0: item["qty"] -= 1
            lcd_line1 = f"EXP: {item['name']}"
        else:                   # Check
            lcd_line1 = f"{item['name']}"
            
        lcd_line2 = f"Q:{item['qty']} ${int(item['price']/1000)}k"
    else:
        # Thẻ lạ
        if current_mode == 1:
            database[rfid] = {"name": "New Item", "qty": 1, "price": 0}
            lcd_line1 = "New Item Added"
            lcd_line2 = f"ID: {rfid}"
        else:
            lcd_line1 = "Unknown Tag!"
            lcd_line2 = f"ID: {rfid}"

    # Bắn chữ hiển thị xuống LCD của mạch (ID 5)
    ser.write(f"!5:{lcd_line1}|{lcd_line2}#".encode('utf-8'))
    
    pushDatabaseToWeb()
    print(f"[Gateway] Đã cập nhật kho & đẩy lên {AIO_FEED_IDs[4]}")

# ==========================================
# 5. CÁC HÀM MQTT
# ==========================================
def connected(client):
    print("[*] Đã kết nối Adafruit IO!")
    # Subscribe nhận lệnh Mode và lệnh Action thủ công từ Web
    client.subscribe(AIO_FEED_IDs[3]) 
    client.subscribe(AIO_FEED_IDs[5]) 

def message(client, feed_id, payload):
    global current_mode
    
    # Nếu Web yêu cầu đổi Mode
    if feed_id == AIO_FEED_IDs[3]:
        current_mode = int(payload)
        ser.write(f"!4:{payload}#".encode('utf-8'))
        print(f"[Gateway] Đổi Mode thành: {current_mode}")
        
    # Nếu Web yêu cầu Nhập/Xuất thủ công (Không qua quét thẻ)
    elif feed_id == AIO_FEED_IDs[5]:
        parts = payload.split(':')
        if len(parts) == 4:
            act_type, name, qty, rfid_in = parts[0], parts[1], int(parts[2]), parts[3]
            
            target_key = None
            
            # 1. Nếu trên Web có gõ sẵn mã RFID thì ưu tiên tìm theo RFID
            if rfid_in != "0" and rfid_in.strip() != "":
                target_key = rfid_in
            else:
                # 2. Nếu không gõ RFID, dò tìm TÊN xem có trùng món nào trong kho không
                for k, v in database.items():
                    # Dùng .lower() và .strip() để lỡ gõ "iphone 15" hay " iPhone 15 " nó vẫn hiểu là 1
                    if v["name"].strip().lower() == name.strip().lower():
                        target_key = k
                        name = v["name"] # Cập nhật lại tên chuẩn cho đồng bộ chữ hoa/thường
                        break
                
                # 3. Nếu dò hết kho không thấy tên trùng -> Lấy luôn tên làm key mới
                if target_key is None:
                    target_key = name 
            
            # Khởi tạo data nếu là hàng mới hoàn toàn
            if target_key not in database:
                database[target_key] = {"name": name, "qty": 0, "price": 0}
                
            # Xử lý cộng/trừ số lượng
            if act_type == 'IMPORT':
                database[target_key]['qty'] += qty
            elif act_type == 'EXPORT':
                if database[target_key]['qty'] >= qty: 
                    database[target_key]['qty'] -= qty
                else: 
                    database[target_key]['qty'] = 0
                
            pushDatabaseToWeb()
            print(f"[Gateway] Đã xử lý {act_type} thủ công cho: {name}")

client = MQTTClient(AIO_USERNAME, AIO_KEY)
client.on_connect = connected
client.on_message = message
client.connect()
client.loop_background()

# ==========================================
# 6. VÒNG LẶP ĐỌC SERIAL TỪ MẠCH
# ==========================================
mess = ""
latest_temp = None
latest_humi = None
last_sensor_publish = time.time()

# Hàm 1: Chuyên xử lý dữ liệu sau khi đã cắt lớp vỏ ! và #
def processData(data):
    global latest_temp, latest_humi
    # Bỏ dấu ! và # để lấy phần ruột (VD: "1:25.0")
    clean_data = data.replace("!", "").replace("#", "")
    parts = clean_data.split(":")
    
    if len(parts) == 2:
        cmd_id, val = parts[0], parts[1]
        
        if cmd_id == "1": 
            latest_temp = val
            client.publish(AIO_FEED_IDs[0], val)
        elif cmd_id == "2": 
            client.publish(AIO_FEED_IDs[1], val)
            latest_humi = val
        elif cmd_id == "3": 
            client.publish(AIO_FEED_IDs[2], val)
            processInventory(val)

# Hàm 2: Chuyên đọc Serial và bóc tách chuỗi (Giống code thầy)
def readSerial():
    global mess
    bytesToRead = ser.inWaiting()
    if bytesToRead > 0:
        mess += ser.read(bytesToRead).decode("UTF-8")
        
        while "#" in mess and "!" in mess:
            end = mess.find("#")
            # VẪN GIỮ rfind ĐỂ CHỐNG LỖI DÍNH CHÙM TỪ MẠCH
            start = mess.rfind("!", 0, end)
            
            if start != -1 and start < end:
                # Gửi cả chuỗi có chứa ! và # qua hàm xử lý
                processData(mess[start:end+1])
            
            # Cắt bỏ phần đã xử lý theo logic của thầy
            if end == len(mess) - 1:
                mess = ""
            else:
                mess = mess[end+1:]

# ==========================================
# VÒNG LẶP CHÍNH (MAIN LOOP)
# ==========================================
print("[*] Gateway đang chạy...")
while True:
    # 1. Gọi hàm đọc Serial
    readSerial()
    time.sleep(0.01)