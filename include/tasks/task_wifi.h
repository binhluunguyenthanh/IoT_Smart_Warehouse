#ifndef TASK_WIFI_TIME_H
#define TASK_WIFI_TIME_H

#include <Arduino.h>

// Hàm thực thi chính của Task Wifi & Time (chạy trong FreeRTOS)
// Nhiệm vụ: Kết nối Wifi, lấy giờ chuẩn quốc tế (NTP) và cập nhật hệ thống
void TaskWifiTimeFunc(void *pvParameters);

#endif