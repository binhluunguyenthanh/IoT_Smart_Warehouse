#ifndef TASK_DISPLAY_H
#define TASK_DISPLAY_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "hal/HalLCD.h"

// -------------------------------------------------------------------------
// STRUCT THAM SỐ CHO TASK DISPLAY
// Dùng để truyền các đối tượng cần thiết vào Task khi khởi tạo
// -------------------------------------------------------------------------
struct DisplayTaskParams {
    QueueHandle_t inputQueue; // Hàng đợi nhận lệnh hiển thị từ Manager
    HalLCD* lcdScreen;        // Con trỏ tới đối tượng màn hình thực tế
};

// Hàm thực thi chính của Task Display
// Vòng lặp vô tận chờ dữ liệu từ Queue và cập nhật LCD
void TaskDisplayFunc(void *pvParameters);

#endif