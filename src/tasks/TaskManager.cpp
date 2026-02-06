#include "tasks/TaskManager.h"
#include "app/inventory.h"
#include "config/SystemConfig.h"
#include "global.h"

// Hàm helper thêm dữ liệu (Sửa lại cho gọn)
void addItem(String hexId, String name, int qty, double price) {
    if(xSemaphoreTake(xInventoryMutex, portMAX_DELAY) == pdTRUE) {
        myWarehouse->addProduct(name, qty, price, hexId);
        xSemaphoreGive(xInventoryMutex);
    }
}

void initMockData() {
    if(xSemaphoreTake(xInventoryMutex, portMAX_DELAY) == pdTRUE) {
        if(myWarehouse->size() == 0) {
            xSemaphoreGive(xInventoryMutex); 
            // Dữ liệu mẫu
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

void TaskManagerFunc(void *pvParameters) {
    ManagerTaskParams* params = (ManagerTaskParams*)pvParameters;
    QueueHandle_t inputQ = params->inputQueue;
    QueueHandle_t displayQ = params->displayQueue;

    initMockData();
    SystemMessage msgIn, msgOut;

    for (;;) {
        if (xQueueReceive(inputQ, &msgIn, 10) == pdTRUE) {
            if (msgIn.type == EVENT_SCAN_RFID) {
                String uidHex = String(msgIn.payload);
                lastScannedRFID = uidHex;
                hasNewTag = true;

                String lcdLine1 = "";
                String lcdLine2 = "";
                
                if(xSemaphoreTake(xInventoryMutex, portMAX_DELAY) == pdTRUE) {
                    
                    // --- TÌM KIẾM ĐƠN GIẢN (CODE MỚI) ---
                    int foundIndex = myWarehouse->findIndexByRFID(uidHex);

                    if (foundIndex != -1) {
                        // Lấy thông tin
                        String name = myWarehouse->getProductName(foundIndex);
                        int qty = myWarehouse->getProductQuantity(foundIndex);
                        double price = myWarehouse->getProductPrice(foundIndex);

                        // Xử lý Logic nhập/xuất
                        if (currentSystemMode == 1) { // Import
                             qty++;
                             myWarehouse->updateQuantity(foundIndex, qty);
                             lcdLine1 = "IMP: " + name; 
                        } 
                        else if (currentSystemMode == 2) { // Export
                             if(qty > 0) qty--; // Chặn không cho âm
                             myWarehouse->updateQuantity(foundIndex, qty);
                             lcdLine1 = "EXP: " + name;
                        } 
                        else { // Check
                             lcdLine1 = name;
                        }

                        // Dòng 2: Hiện số lượng và giá
                        lcdLine2 = "Q:" + String(qty) + " $" + String((int)(price/1000)) + "k";

                    } else {
                        // Thẻ lạ
                        if (currentSystemMode == 1) {
                            // Tự động thêm sản phẩm mới nếu ở chế độ Import
                            myWarehouse->addProduct("New Item", 1, 0, uidHex);
                            lcdLine1 = "New Item Added";
                            lcdLine2 = "ID: " + uidHex;
                        } else {
                            lcdLine1 = "Unknown Tag!";
                            lcdLine2 = "ID: " + uidHex;
                        }
                    }
                    xSemaphoreGive(xInventoryMutex);
                }

                Serial.println("[LCD] " + lcdLine1 + " | " + lcdLine2);

                // Gửi sang màn hình
                msgOut.type = EVENT_UPDATE_LCD;
                String fullMsg = lcdLine1 + "|" + lcdLine2;
                strncpy(msgOut.payload, fullMsg.c_str(), sizeof(msgOut.payload)-1);
                xQueueSend(displayQ, &msgOut, 10);
            }
        }
        vTaskDelay(10);
    }
}