#ifndef INVENTORY_MANAGER_H
#define INVENTORY_MANAGER_H

#include <Arduino.h>
#include <vector>

// Cấu trúc của một sản phẩm trong kho
struct Product {
    String name;
    int quantity;
    double price;
    String rfid; 
    // Khởi tạo 1 Product bất kỳ đảm bảo 4 thuộc tính
    Product(String n, int q, double p, String id) 
        : name(n), quantity(q), price(p), rfid(id) {}
};
// Cấu trúc quản lý kho
class InventoryManager {
private:
// Cơ sở dữ liêu của Product
    std::vector<Product> database;

public:
    // Hàm khởi tạo kho không cần điều kiện
    InventoryManager() {}

    // Lấy số lượng sản phẩm
    int size() const { return database.size(); }

    // Lấy tên của Product tại chỉ số index
    String getProductName(int index) { 
        if(index < 0 || index >= database.size()) return "";
        return database[index].name; 
    }
    // Lấy số lượng của Product tại chỉ số index
    int getProductQuantity(int index) { 
        if(index < 0 || index >= database.size()) return 0;
        return database[index].quantity; 
    }
    // Lấy giá của Product tại chỉ số index
    double getProductPrice(int index) { 
        if(index < 0 || index >= database.size()) return 0.0;
        return database[index].price; 
    }
    // Lấy RFID của Product tại chỉ số index
    String getProductRFID(int index) {
        if(index < 0 || index >= database.size()) return "";
        return database[index].rfid;
    }

    // Cập nhật số lượng (An toàn, không cho phép số âm)
    void updateQuantity(int index, int newQty) {
        if(index >= 0 && index < database.size()) {
            if (newQty < 0) newQty = 0; // Chặn lỗi số âm tại đây
            database[index].quantity = newQty;
        }
    }

    // Thêm sản phẩm mới
    void addProduct(String name, int qty, double price, String rfid) {
        database.push_back(Product(name, qty, price, rfid));
    }

    // Tìm kiếm theo RFID
    int findIndexByRFID(String rfidObj) {
        for(int i = 0; i < database.size(); i++) {
            if(database[i].rfid.equalsIgnoreCase(rfidObj)) {
                return i;
            }
        }
        return -1;
    }
    
    // Tìm kiếm theo Tên (cho Web)
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