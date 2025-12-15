# 📦 IoT Smart Warehouse
(Hệ thống Quản lý Kho Thông minh)

IoT Smart Warehouse là một dự án minh họa hệ thống quản lý kho (Inventory Management System) triển khai trên nền tảng ESP32 + Arduino. Dự án vừa phục vụ mục đích học tập (các cấu trúc dữ liệu và thuật toán), vừa làm ví dụ ứng dụng thực tế cho thiết bị nhúng.

---

## ✅ Tính năng chính
- Thêm, xem, tìm kiếm, cập nhật và xóa sản phẩm qua giao diện CLI (Serial Monitor).
- Lưu trữ dữ liệu bằng cấu trúc dữ liệu tự triển khai (Doubly Linked List, Hash Map, Heap, Array List...).
- Hỗ trợ nén dữ liệu (Huffman) cho một số thao tác nội bộ (mô-đun nén).
- Tương thích với ESP32 (framework Arduino) và build bằng PlatformIO.

---

## Yêu cầu
- VS Code (khuyến nghị)
- PlatformIO extension cho VS Code
- Board: ESP32 (ví dụ: esp32doit-devkit-v1)
- Baud rate serial monitor: 115200

---

## Cài đặt & Build
1. Mở thư mục dự án bằng VS Code.
2. PlatformIO sẽ tự động tải các thư viện cần thiết.
3. Đảm bảo `platformio.ini` được cấu hình để sử dụng C++17.

Ví dụ `platformio.ini`:
```ini
[env:esp32doit-devkit-v1]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
monitor_speed = 115200

lib_deps =
    marcoschwartz/LiquidCrystal_I2C @ ^1.1.4

build_unflags = -std=gnu++11
build_flags = -std=gnu++17
```

---

## Hướng dẫn sử dụng (CLI qua Serial Monitor)
Mở Serial Monitor với baudrate 115200 sau khi nạp code. Các lệnh CLI (ví dụ):

1. Thêm sản phẩm (ADD)  
   Cú pháp: `ADD <Tên> <SốLượng> [<ThuộcTính1> <GiáTrị1> ...]`  
   Ví dụ:
   ```
   ADD ESP32 100 Voltage 3.3 Flash 4
   ```

2. Hiển thị toàn bộ kho (SHOW)  
   ```
   SHOW
   ```

3. Tìm kiếm thông minh (FIND)  
   - Tìm theo tên: `FIND <tên>`  
   - Tìm theo giá trị thuộc tính: `FIND <giá trị>`  
   Ví dụ:
   ```
   FIND ESP32
   FIND 3.3
   ```

4. Cập nhật sản phẩm (UPDATE)  
   Cú pháp: `UPDATE <ID|Tên> <ThuộcTính> <GiáTrị>`  
   Ví dụ:
   ```
   UPDATE ESP32 Quantity 120
   ```

5. Xóa sản phẩm (DELETE)  
   Cú pháp: `DELETE <ID|Tên>`  
   Ví dụ:
   ```
   DELETE ESP32
   ```

6. Xóa toàn bộ kho (CLEAR)  
   ```
   CLEAR
   ```

Ghi chú: CLI có thể hỗ trợ các biến thể cú pháp khác; kiểm tra log trên Serial để biết phản hồi chi tiết.

---

## Cấu trúc dự án
```
IoT_Smart_Warehouse/
├── include/
│   ├── app/
│   │   ├── inventory.h               # Quản lý kho (InventoryManager)
│   │   └── inventory_compressor.h    # Nén Huffman
│   ├── hash/
│   │   ├── IMap.h
│   │   └── xMap.h                    # Hash map implementation
│   ├── heap/
│   │   ├── IHeap.h
│   │   └── Heap.h                    # Heap implementation
│   ├── list/
│   │   ├── IList.h
│   │   ├── DLinkedList.h             # Doubly linked list
│   │   └── XArrayList.h              # Array list
│   └── util/                         # Các tiện ích (Point class, helpers...)
├── src/
│   └── main.cpp                      # Chương trình chính (Arduino sketch)
├── platformio.ini                    # Cấu hình build & thư viện
└── README.md
```

---

## Các cấu trúc dữ liệu & thuật toán đã triển khai
- DLinkedList (Doubly Linked List)
- XArrayList (Array List)
- Hash Map (xMap)
- Heap (min/max heap)
- Huffman Compressor (Cho mục đích nén dữ liệu mẫu)
- InventoryManager (logic nghiệp vụ quản lý kho)

Mục tiêu của dự án là kết hợp lý thuyết cấu trúc dữ liệu với ứng dụng IoT thực tế.

---

## Góp ý & Phát triển
Nếu bạn muốn:
- Thêm tính năng lưu persistent (SPIFFS/LittleFS/EEPROM)
- Kết nối giao diện web hoặc MQTT để quản lý từ xa
- Hoàn thiện bộ lệnh CLI và parse chính xác hơn

Hãy mở issue hoặc gửi pull request lên repository.

---

## License
Miễn trách nhiệm / Tùy chọn: thêm file LICENSE nếu bạn muốn cấp phép cụ thể.

---

Nếu bạn muốn, mình có thể:
- Thêm hướng dẫn chi tiết cho từng lệnh CLI dựa trên code hiện tại.
- Tạo ví dụ test inputs/outputs.
- Hoàn thiện phần "Contributing" và mẫu issue/PR templates.
