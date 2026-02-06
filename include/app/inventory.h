#ifndef INVENTORY_MANAGER_H
#define INVENTORY_MANAGER_H

#include <Arduino.h>
#include <vector>

// Cấu trúc sản phẩm đơn giản hóa
struct Product {
    String name;
    int quantity;
    double price;
    String rfid; // Lưu mã thẻ dạng Hex (VD: "195623B3")

    Product(String n, int q, double p, String id) 
        : name(n), quantity(q), price(p), rfid(id) {}
};

class InventoryManager {
private:
    std::vector<Product> database;

public:
    InventoryManager() {}

    // Lấy số lượng sản phẩm
    int size() const { return database.size(); }

    // Các hàm lấy dữ liệu (Getter)
    String getProductName(int index) { 
        if(index < 0 || index >= database.size()) return "";
        return database[index].name; 
    }

    int getProductQuantity(int index) { 
        if(index < 0 || index >= database.size()) return 0;
        return database[index].quantity; 
    }

    double getProductPrice(int index) { 
        if(index < 0 || index >= database.size()) return 0.0;
        return database[index].price; 
    }

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