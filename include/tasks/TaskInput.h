#ifndef TASK_INPUT_H
#define TASK_INPUT_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "hal/HalRFID.h"

// Struct chứa các tham số cần thiết cho Task hoạt động
struct InputTaskParams {
    QueueHandle_t outputQueue; // Queue để gửi kết quả sang Manager
    HalRFID* rfidReader;       // Pointer tới phần cứng RFID
};

// Prototype hàm task
void TaskInputFunc(void *pvParameters);

#endif