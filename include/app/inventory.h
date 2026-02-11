#ifndef INVENTORY_MANAGER_H
#define INVENTORY_MANAGER_H

#include <Arduino.h>
#include <vector>

// -------------------------------------------------------------------------
// CẤU TRÚC DỮ LIỆU SẢN PHẨM
// Mô tả các thuộc tính cơ bản của một sản phẩm trong kho
// -------------------------------------------------------------------------
struct Product {
    String name;    // Tên sản phẩm
    int quantity;   // Số lượng tồn kho
    double price;   // Đơn giá
    String rfid;    // Mã thẻ RFID gắn với sản phẩm

    // Constructor: Khởi tạo 1 Product với đầy đủ 4 thuộc tính
    Product(String n, int q, double p, String id) 
        : name(n), quantity(q), price(p), rfid(id) {}
};

// -------------------------------------------------------------------------
// LỚP QUẢN LÝ KHO (INVENTORY MANAGER)
// Chịu trách nhiệm thao tác trực tiếp với danh sách sản phẩm (vector)
// -------------------------------------------------------------------------
class InventoryManager {
private:
    // Cơ sở dữ liệu lưu trữ danh sách sản phẩm (sử dụng std::vector để linh hoạt kích thước)
    std::vector<Product> database;

public:
    // Hàm khởi tạo mặc định (không cần tham số)
    InventoryManager() {}

    // Trả về tổng số lượng loại sản phẩm đang có trong kho
    int size() const { return database.size(); }

    // --- CÁC HÀM GETTER (Lấy thông tin an toàn) ---
    
    // Lấy tên sản phẩm tại vị trí index (có kiểm tra giới hạn mảng)
    String getProductName(int index) { 
        if(index < 0 || index >= database.size()) return "";
        return database[index].name; 
    }

    // Lấy số lượng sản phẩm tại vị trí index
    int getProductQuantity(int index) { 
        if(index < 0 || index >= database.size()) return 0;
        return database[index].quantity; 
    }

    // Lấy giá sản phẩm tại vị trí index
    double getProductPrice(int index) { 
        if(index < 0 || index >= database.size()) return 0.0;
        return database[index].price; 
    }

    // Lấy mã RFID của sản phẩm tại vị trí index
    String getProductRFID(int index) {
        if(index < 0 || index >= database.size()) return "";
        return database[index].rfid;
    }

    // --- CÁC HÀM SETTER / UPDATE (Cập nhật dữ liệu) ---

    // Cập nhật số lượng mới cho sản phẩm tại index
    // Tính năng an toàn: Tự động chuyển về 0 nếu giá trị nhập vào là số âm
    void updateQuantity(int index, int newQty) {
        if(index >= 0 && index < database.size()) {
            if (newQty < 0) newQty = 0; // Chặn lỗi logic số âm
            database[index].quantity = newQty;
        }
    }

    // Thêm một sản phẩm mới hoàn toàn vào cuối danh sách
    void addProduct(String name, int qty, double price, String rfid) {
        database.push_back(Product(name, qty, price, rfid));
    }

    // --- CÁC HÀM TÌM KIẾM (SEARCH) ---

    // Tìm vị trí (index) của sản phẩm dựa trên mã RFID
    // Trả về: index nếu tìm thấy, -1 nếu không tìm thấy
    int findIndexByRFID(String rfidObj) {
        for(int i = 0; i < database.size(); i++) {
            if(database[i].rfid.equalsIgnoreCase(rfidObj)) {
                return i;
            }
        }
        return -1;
    }
    
    // Tìm vị trí (index) của sản phẩm dựa trên Tên (Dùng cho thao tác trên Web)
    // Trả về: index nếu tìm thấy, -1 nếu không tìm thấy
    int findIndexByName(String nameObj) {
        for(int i = 0; i < database.size(); i++) {
            if(database[i].name.equalsIgnoreCase(nameObj)) {
                return i;
            }
        }
        return -1;
    }
};

#endif