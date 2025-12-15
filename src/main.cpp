#include <Arduino.h>
#include <string>
#include <vector>
#include <sstream> // Thư viện quan trọng để tách chuỗi
#include <Wire.h> 
#include <LiquidCrystal_I2C.h> 

// Include các file logic của bạn
#include "app/inventory.h"
#include "util/Point.h"
#include "app/inventory_compressor.h"

// --- CẤU HÌNH ---
// Địa chỉ LCD 0x27, 16 cột 2 dòng
LiquidCrystal_I2C lcd(0x27, 16, 2);
InventoryManager myWarehouse;
String inputString = "";         

// Hàm in ra LCD (rút gọn)
void logToLCD(String line1, String line2 = "") {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    if (line2 != "") {
        lcd.setCursor(0, 1);
        lcd.print(line2);
    }
}

void setup() {
    Serial.begin(115200);
    lcd.init();
    lcd.backlight();

    logToLCD("KHO TONG HOP", "Ready...");
    
    Serial.println("\n\n==========================================");
    Serial.println("--- HE THONG QUAN LY KHO (FULL MODE) ---");
    Serial.println("Cu phap lenh:");
    Serial.println("1. ADD <Ten> <SL> <Attr1> <Val1> <Attr2> <Val2> ...");
    Serial.println("   VD: ADD AoThun 10 Size 39 Color 1");
    Serial.println("2. FIND <Attr> <Val>");
    Serial.println("   VD: FIND Size 39");
    Serial.println("3. SHOW");
    Serial.println("4. TEST");
    Serial.println("5. CLEAR");
    Serial.println("==========================================\n");
    Serial.print(">> "); 
}

void processCommand(String commandArduino) {
    // Chuyển String Arduino sang std::string để dùng stringstream
    std::string cmdStd = commandArduino.c_str();
    std::stringstream ss(cmdStd);
    
    std::string action;
    ss >> action; // Đọc từ đầu tiên (ADD, SHOW, FIND...)
    
    // Chuyển action thành chữ hoa để so sánh
    for (auto & c: action) c = toupper(c);

    if (action == "HELP") {
        Serial.println("\n--- HUONG DAN ---");
        Serial.println("Nhap: ADD <Ten> <SL> [ThuocTinh] [GiaTri]...");
        Serial.println("VD:   ADD GheGo 20 Cao 100 Rong 50");
        
    } else if (action == "SHOW") {
        Serial.println("\n[DANH SACH KHO HANG]");
        if (myWarehouse.size() == 0) {
            Serial.println("(Kho rong)");
            logToLCD("Kho dang rong", "0 san pham");
        } else {
            // In ra Serial chi tiết
            Serial.println(myWarehouse.toString().c_str());
            // In ra LCD tóm tắt
            logToLCD("Hien thi kho", String(myWarehouse.size()) + " loai SP");
        }

    } else if (action == "ADD") {
        // Cấu trúc: ADD <Name> <Qty> <Attr1> <Val1> <Attr2> <Val2> ...
        std::string name;
        int qty;
        
        // Cố gắng đọc tên và số lượng
        if (ss >> name >> qty) {
            List1D<InventoryAttribute> attrs;
            
            // Đọc tiếp các cặp Thuộc tính - Giá trị (nếu có)
            std::string attrName;
            double attrVal;
            while (ss >> attrName >> attrVal) {
                attrs.add(InventoryAttribute(attrName, attrVal));
            }
            
            myWarehouse.addProduct(attrs, name, qty);
            
            Serial.print("\n-> Da them: ");
            Serial.print(name.c_str());
            Serial.print(" (SL: ");
            Serial.print(qty);
            Serial.println(")");
            Serial.print("   Voi cac thuoc tinh: ");
            Serial.println(attrs.toString().c_str());

            logToLCD("Da them:", String(name.c_str()));
        } else {
            Serial.println("\nLoi: Thieu Ten hoac So luong!");
            Serial.println("Dung: ADD <Ten> <SL> ...");
        }

    } else if (action == "FIND") {
        // Cấu trúc: FIND <Attr> <Val>
        std::string attrName;
        double val;
        
        if (ss >> attrName >> val) {
            Serial.print("\n-> Dang tim: ");
            Serial.print(attrName.c_str());
            Serial.print(" = ");
            Serial.println(val);
            
            // Tìm biên độ +/- 0.01 để xử lý sai số số thực
            List1D<string> results = myWarehouse.query(attrName, val - 0.01, val + 0.01, 1, true);
            
            Serial.print("Ket qua: ");
            Serial.println(results.toString().c_str());
            
            if (results.size() > 0) {
                // Lấy tên sản phẩm đầu tiên tìm thấy đưa lên LCD
                String firstItem = results.get(0).c_str();
                logToLCD("Thay " + String(results.size()) + " SP:", firstItem);
            } else {
                logToLCD("Tim kiem:", "Khong thay!");
            }
        } else {
            Serial.println("\nLoi: Thieu Ten thuoc tinh hoac Gia tri!");
            Serial.println("Dung: FIND <TenThuocTinh> <GiaTri>");
        }

    } else if (action == "TEST") {
        Serial.println("\n[DEMO NEN DU LIEU THUC TE]");
        
        // 1. Lấy một sản phẩm mẫu để test
        if (myWarehouse.size() == 0) {
            Serial.println("Loi: Kho rong, hay ADD san pham truoc de test nien!");
            return;
        }
        
        // Xây dựng cây từ dữ liệu hiện tại
        InventoryCompressor<4> compressor(&myWarehouse);
        compressor.buildHuffman();
        
        // Lấy sản phẩm đầu tiên
        List1D<InventoryAttribute> attrs = myWarehouse.getProductAttributes(0);
        string name = myWarehouse.getProductName(0);
        
        // Chuỗi gốc (mô phỏng dữ liệu thô)
        string rawString = compressor.productToString(attrs, name);
        
        // Nén
        string encoded = compressor.encodeHuffman(attrs, name);
        
        // --- TÍNH TOÁN HIỆU QUẢ ---
        // Kích thước gốc (tính bằng bit): Mỗi ký tự ASCII tốn 8 bit
        int originalBits = rawString.length() * 8;
        
        // Kích thước nén (tính bằng bit): 
        // Với TreeOrder = 4 (Hệ cơ số 4), mỗi ký tự trong chuỗi encoded (0,1,2,3) tốn 2 bit.
        // (Nếu TreeOrder = 2 thì tốn 1 bit/ký tự, TreeOrder = 16 thì tốn 4 bit/ký tự)
        int compressedBits = encoded.length() * 2; 
        
        float saving = 100.0 * (originalBits - compressedBits) / originalBits;

        // --- HIỂN THỊ RA MÀN HÌNH ---
        Serial.println("---------------------------------");
        Serial.print("San pham: "); Serial.println(name.c_str());
        Serial.print("Du lieu tho: "); Serial.println(rawString.c_str());
        Serial.print("-> Original Size: "); Serial.print(originalBits); Serial.println(" bits");
        
        Serial.print("Ma Huffman (He 4): "); Serial.println(encoded.c_str());
        Serial.print("-> Compressed Size: "); Serial.print(compressedBits); Serial.println(" bits");
        
        Serial.print("=> TIET KIEM DUOC: "); Serial.print(saving); Serial.println("% DUNG LUONG!");
        Serial.println("---------------------------------");

        // Hiển thị LCD tóm tắt
        logToLCD("Nen Huffman", "Giam " + String(saving, 1) + "%");
        
        // Test giải nén để đảm bảo toàn vẹn dữ liệu
        List1D<InventoryAttribute> outAttrs;
        string outName;
        compressor.decodeHuffman(encoded, outAttrs, outName);
        if (outName == name) {
            Serial.println("Kiem tra toan ven: PASS (Giai nen dung)");
        } else {
            Serial.println("Kiem tra toan ven: FAIL");
        }

    } else if (action == "CLEAR") {
        myWarehouse = InventoryManager(); 
        Serial.println("\n-> Da xoa sach kho hang!");
        logToLCD("Da xoa kho", "Empty!");

    } else {
        Serial.println("\nLenh khong hop le!");
    }

    Serial.print("\n>> "); 
}

void loop() {
    if (Serial.available()) {
        char inChar = (char)Serial.read();

        // 1. Xử lý phím Enter (\n hoặc \r) -> Gửi lệnh
        if (inChar == '\n' || inChar == '\r') {
            if (inputString.length() > 0) {
                Serial.println(); // Xuống dòng
                processCommand(inputString);
                inputString = ""; // Xóa bộ nhớ đệm để chờ lệnh mới
            }
        }
        // 2. Xử lý phím Xóa (Backspace = 8 hoặc Delete = 127)
        else if (inChar == 8 || inChar == 127) {
            if (inputString.length() > 0) {
                // Xóa ký tự cuối cùng trong chuỗi lưu trữ
                inputString.remove(inputString.length() - 1);
                
                // Xóa hiển thị trên màn hình (Thủ thuật: Lùi - Cách - Lùi)
                Serial.print("\b \b"); 
            }
        }
        // 3. Xử lý ký tự thường (Chữ, số, khoảng trắng)
        else {
            Serial.print(inChar);   // In ra cho người dùng thấy (Echo)
            inputString += inChar;  // Lưu vào chuỗi
        }
    }
}