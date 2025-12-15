📦 IoT Smart Warehouse (Hệ thống Quản lý Kho Thông minh)Dự án này là một hệ thống quản lý kho hàng (Inventory Management System) được xây dựng trên nền tảng vi điều khiển ESP32, kết hợp mô phỏng các thuật toán và cấu trúc dữ liệu nâng cao (Data Structures & Algorithms).Hệ thống cho phép người dùng tương tác qua giao diện dòng lệnh (CLI) trên Serial Monitor để nhập, xuất, tìm kiếm hàng hóa và hiển thị trạng thái trực quan trên màn hình LCD I2C. Đặc biệt, dự án tích hợp thuật toán N-ary Huffman Coding để mô phỏng khả năng nén dữ liệu tối ưu cho các thiết bị IoT.🚀 Tính Năng ChínhQuản lý Kho Tổng Quát: Lưu trữ đa dạng các loại sản phẩm (Linh kiện, Quần áo, Nội thất...) với số lượng thuộc tính động (không giới hạn số lượng thuộc tính như Size, Color, Voltage...).Giao diện CLI Mạnh Mẽ: Tương tác trực tiếp qua Serial Monitor với bộ phân tích cú pháp lệnh (Command Parser) linh hoạt.Tìm Kiếm Thông Minh (Smart Search): Tìm kiếm sản phẩm theo tên (từ khóa) hoặc theo giá trị thuộc tính bất kỳ.Hiển thị LCD: Thông báo trạng thái, kết quả tìm kiếm và thao tác nhập kho lên màn hình LCD 1602.Nén Dữ Liệu Huffman: Tích hợp thuật toán nén Huffman (hệ cơ số N tùy chỉnh) để tối ưu hóa không gian lưu trữ và băng thông truyền tải.Cấu trúc Dữ liệu Tự xây dựng: Toàn bộ các cấu trúc dữ liệu lõi (DLinkedList, XArrayList, HashMap, Heap) được viết từ đầu (from scratch) bằng C++, không sử dụng thư viện STL có sẵn (ngoại trừ std::string, vector cho logic ứng dụng).🛠️ Yêu Cầu Phần CứngVi điều khiển: ESP32 (ESP32-WROOM-32 / NodeMCU-32S / ESP32 DevKit V1).Hiển thị: Màn hình LCD 1602 kèm module I2C.Kết nối: Cáp Micro USB để nạp code và debug.Sơ đồ đấu dây (Pinout)Thiết bịChân (Pin)ESP32 PinGhi chúLCD I2CGNDGNDVCCVIN (5V)SDAGPIO 21Chân I2C DataSCLGPIO 22Chân I2C Clock💻 Yêu Cầu Phần Mềm & Cài ĐặtCông cụVS CodePlatformIO IDE (Extension trong VS Code)Wokwi Simulator (Tùy chọn: Để mô phỏng nếu không có mạch thật)Cài đặtClone repository này về máy:Bashgit clone https://github.com/binhluunguyenthanh/IoT_Smart_Warehouse.git
Mở thư mục dự án bằng VS Code.Chờ PlatformIO tự động tải các thư viện cần thiết.Đảm bảo file platformio.ini đã được cấu hình chuẩn C++17 (Bắt buộc để chạy được các tính năng if constexpr, auto...):Ini, TOML[env:esp32doit-devkit-v1]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
monitor_speed = 115200

lib_deps =
    marcoschwartz/LiquidCrystal_I2C @ ^1.1.4

build_unflags = -std=gnu++11
build_flags = -std=gnu++17
📖 Hướng Dẫn Sử Dụng (CLI)Sau khi nạp code và mở Serial Monitor (Baudrate 115200), bạn có thể sử dụng các lệnh sau:1. Thêm sản phẩm (ADD)Cú pháp: ADD <Tên> <SốLượng> [ThuộcTính] [GiáTrị] ...Lưu ý: Tên thuộc tính viết liền, Giá trị thuộc tính hiện tại hỗ trợ kiểu số thực.Ví dụ:Plaintext>> ADD AoThun 50 Size 40 Color 1
>> ADD ESP32 100 Voltage 3.3 Flash 4
2. Xem danh sách (SHOW)Hiển thị toàn bộ kho hàng hiện có.Plaintext>> SHOW
3. Tìm kiếm (FIND)Tìm kiếm thông minh theo Tên hoặc theo Giá trị thuộc tính.Tìm theo thông số: FIND 3.3 (Sẽ ra ESP32)Tìm theo tên: FIND Thun (Sẽ ra AoThun)Tìm theo size: FIND 404. Test nén Huffman (TEST)Chạy demo nén dữ liệu cho sản phẩm đầu tiên trong kho và hiển thị tỷ lệ tiết kiệm dung lượng.Plaintext>> TEST
5. Xóa kho (CLEAR)Xóa toàn bộ dữ liệu trong kho.Plaintext>> CLEAR
📂 Cấu Trúc Dự ÁnPlaintextIoT_Smart_Warehouse/
├── include/
│   ├── app/                # Logic nghiệp vụ chính
│   │   ├── inventory.h     # Quản lý kho (InventoryManager)
│   │   └── inventory_compressor.h # Nén Huffman
│   ├── hash/               # Hash Map implementation
│   │   ├── IMap.h
│   │   └── xMap.h
│   ├── heap/               # Heap implementation
│   │   ├── IHeap.h
│   │   └── Heap.h
│   ├── list/               # Linked List & Array List implementation
│   │   ├── IList.h
│   │   ├── DLinkedList.h
│   │   └── XArrayList.h
│   └── util/               # Các tiện ích bổ trợ (Point class...)
├── src/
│   └── main.cpp            # Chương trình chính (Arduino Sketch)
├── platformio.ini          # Cấu hình Build & Library
└── README.md
🧠 Data Structures ImplementedDự án này là minh chứng cho việc áp dụng các cấu trúc dữ liệu phức tạp vào thực tế:DLinkedList (Doubly Linked List): Dùng để lưu trữ danh sách va chạm trong Hash Map và quản lý danh sách sản phẩm linh hoạt.XArrayList (Dynamic Array): Dùng trong cấu trúc cây Huffman để quản lý các node con.xMap (Hash Map): Dùng để ánh xạ ký tự sang mã Huffman và quản lý tần xuất xuất hiện.Heap (Min-Heap): Dùng trong thuật toán xây dựng cây Huffman (Priority Queue).N-ary Huffman Tree: Cây Huffman đa nhánh dùng cho giải thuật nén.👨‍💻 Tác GiảBinh Luu Nguyen ThanhProject: Data Structures & Algorithms AssignmentContact: Github ProfileDự án được phát triển với mục đích học tập và nghiên cứu.
