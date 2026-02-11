#ifndef TASK_TEMP_H
#define TASK_TEMP_H
#include <Arduino.h>
#include <DHT.h>
#include "config/SystemConfig.h"
#include "global.h"

// Hàm thực thi chính của Task Temp
// Đọc cảm biến DHT định kỳ và cập nhật vào biến toàn cục
void TaskTempFunc(void *pvParameters);

#endif