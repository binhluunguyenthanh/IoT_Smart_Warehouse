#ifndef __TASK_WIFI_H__
#define __TASK_WIFI_H__

#include <WiFi.h>
#include <ESPAsyncWebServer.h> 
#include "check_info.h"
#include "global.h"

// Hàm thực thi chính của Task WiFi (giám sát trạng thái kết nối)
void TaskWifiFunc(void *pvParameters);

// Chạy chế độ Access Point (Phát Wifi để cấu hình)
// Dùng khi chưa có thông tin Wifi hoặc không kết nối được
void runAPMode();

// Chạy chế độ Station (Kết nối vào Router Wifi có sẵn)
// Dùng để hoạt động bình thường
void runSTAMode();

#endif