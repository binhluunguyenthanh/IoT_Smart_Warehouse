#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// Struct params cho Manager (nhận cả 2 queue để điều phối)
struct ManagerTaskParams {
    QueueHandle_t inputQueue;   // Nhận từ Input Task
    QueueHandle_t displayQueue; // Gửi sang Display Task
};

void TaskManagerFunc(void *pvParameters);

#endif