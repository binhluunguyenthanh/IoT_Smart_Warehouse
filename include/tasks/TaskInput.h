#ifndef TASK_INPUT_H
#define TASK_INPUT_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "hal/HalRFID.h"

// -------------------------------------------------------------------------
// STRUCT THAM SỐ CHO TASK INPUT
// Chứa các tài nguyên cần thiết để task hoạt động
// -------------------------------------------------------------------------
struct InputTaskParams {
    QueueHandle_t outputQueue; // Queue để gửi kết quả (Mã thẻ) sang Manager
    HalRFID* rfidReader;       // Con trỏ tới phần cứng RFID
};

// Hàm thực thi chính của Task Input
// Vòng lặp vô tận quét thẻ RFID và đẩy sự kiện vào Queue
void TaskInputFunc(void *pvParameters);

#endif