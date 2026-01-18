#ifndef TASK_DISPLAY_H
#define TASK_DISPLAY_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "hal/HalLCD.h"

struct DisplayTaskParams {
    QueueHandle_t inputQueue; // Nhận lệnh từ Manager
    HalLCD* lcdScreen;        // Pointer tới màn hình
};

void TaskDisplayFunc(void *pvParameters);

#endif