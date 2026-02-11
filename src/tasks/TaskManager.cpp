#include "tasks/TaskManager.h"
#include "app/inventory.h"
#include "config/SystemConfig.h"
#include "global.h"

// Hàm helper thêm dữ liệu (có khóa Mutex để an toàn)
void addItem(String hexId, String name, int qty, double price) {
    if(xSemaphoreTake(xInventoryMutex, portMAX_DELAY) == pdTRUE) {
        myWarehouse->addProduct(name, qty, price, hexId);
        xSemaphoreGive(xInventoryMutex);
    }
}

// Hàm tạo dữ liệu giả lập (Mock Data) khi khởi động
void initMockData() {
    if(xSemaphoreTake(xInventoryMutex, portMAX_DELAY) == pdTRUE) {
        // Chỉ thêm nếu kho đang rỗng
        if(myWarehouse->size() == 0) {
            xSemaphoreGive(xInventoryMutex); // Nhả khóa để addItem dùng lại
            
            // Thêm các sản phẩm mẫu
            addItem("195623B3", "iPhone 15",    5,  25000000); 
            addItem("19D01AB3", "Noi Com Dien", 10, 500000);   
            addItem("297124B3", "May Xay",      8,  350000);   
            addItem("59B4DCC2", "May Khoan",    12, 450000);   
            addItem("66E5C901", "Den Ban",      20, 150000);   
            addItem("191D1BB3", "Quat Dung",    15, 250000);   
            addItem("2D0F7506", "MacBook Pro",  3,  45000000); 
            
            Serial.println("[DATA] Loaded mock items.");
        } else {
            xSemaphoreGive(xInventoryMutex);
        }
    }
}

// --- TASK MANAGER MAIN LOOP ---
void TaskManagerFunc(void *pvParameters) {
    ManagerTaskParams* params = (ManagerTaskParams*)pvParameters;
    QueueHandle_t inputQ = params->inputQueue;     // Queue nhận từ Input
    QueueHandle_t displayQ = params->displayQueue; // Queue gửi sang Display

    initMockData(); // Nạp dữ liệu
    SystemMessage msgIn, msgOut;

    for (;;) {
        // Chờ nhận tin nhắn từ Input Task
        if (xQueueReceive(inputQ, &msgIn, 10) == pdTRUE) {
            if (msgIn.type == EVENT_SCAN_RFID) {
                String uidHex = String(msgIn.payload);
                lastScannedRFID = uidHex; // Lưu lại để WebServer dùng
                hasNewTag = true;         // Bật cờ báo thẻ mới

                String lcdLine1 = "";
                String lcdLine2 = "";
                
                // Khóa Mutex để thao tác với kho hàng
                if(xSemaphoreTake(xInventoryMutex, portMAX_DELAY) == pdTRUE) {
                    
                    // Tìm kiếm sản phẩm trong kho bằng RFID
                    int foundIndex = myWarehouse->findIndexByRFID(uidHex);

                    if (foundIndex != -1) {
                        // --- TRƯỜNG HỢP: TÌM THẤY SẢN PHẨM ---
                        String name = myWarehouse->getProductName(foundIndex);
                        int qty = myWarehouse->getProductQuantity(foundIndex);
                        double price = myWarehouse->getProductPrice(foundIndex);

                        // Xử lý Logic dựa trên chế độ (Mode) hiện tại
                        if (currentSystemMode == 1) { // Mode Import (Nhập kho)
                             qty++;
                             myWarehouse->updateQuantity(foundIndex, qty);
                             lcdLine1 = "IMP: " + name; 
                        } 
                        else if (currentSystemMode == 2) { // Mode Export (Xuất kho)
                             if(qty > 0) qty--; // Chặn số lượng âm
                             myWarehouse->updateQuantity(foundIndex, qty);
                             lcdLine1 = "EXP: " + name;
                        } 
                        else { // Mode Check (Kiểm tra)
                             lcdLine1 = name;
                        }

                        // Dòng 2: Hiện số lượng và giá (Định dạng k)
                        lcdLine2 = "Q:" + String(qty) + " $" + String((int)(price/1000)) + "k";

                    } else {
                        // --- TRƯỜNG HỢP: THẺ LẠ (CHƯA CÓ TRONG KHO) ---
                        if (currentSystemMode == 1) {
                            // Chế độ Import: Tự động thêm sản phẩm mới
                            myWarehouse->addProduct("New Item", 1, 0, uidHex);
                            lcdLine1 = "New Item Added";
                            lcdLine2 = "ID: " + uidHex;
                        } else {
                            // Chế độ khác: Báo lỗi thẻ lạ
                            lcdLine1 = "Unknown Tag!";
                            lcdLine2 = "ID: " + uidHex;
                        }
                    }
                    // Xử lý xong -> Mở khóa Mutex
                    xSemaphoreGive(xInventoryMutex);
                }

                Serial.println("[LCD] " + lcdLine1 + " | " + lcdLine2);

                // Gửi lệnh cập nhật màn hình sang Task Display
                msgOut.type = EVENT_UPDATE_LCD;
                String fullMsg = lcdLine1 + "|" + lcdLine2;
                strncpy(msgOut.payload, fullMsg.c_str(), sizeof(msgOut.payload)-1);
                xQueueSend(displayQ, &msgOut, 10);
            }
        }
        // Nghỉ 10ms
        vTaskDelay(10);
    }
}