#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// -------------------------------------------------------------------------
// STRUCT THAM SỐ CHO TASK MANAGER
// Manager cần nắm giữ cả 2 đầu mối Queue để nhận và gửi lệnh
// -------------------------------------------------------------------------
struct ManagerTaskParams {
    QueueHandle_t inputQueue;   // Nhận dữ liệu từ Input Task (VD: Mã thẻ vừa quẹt)
    QueueHandle_t displayQueue; // Gửi lệnh hiển thị sang Display Task
};

// Hàm thực thi chính của Task Manager
// Xử lý logic nghiệp vụ: Check mã thẻ -> Tra cứu Inventory -> Quyết định hiển thị
void TaskManagerFunc(void *pvParameters);

#endif