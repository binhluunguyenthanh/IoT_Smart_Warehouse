#ifndef __TASK_CHECK_INFO_H__
#define __TASK_CHECK_INFO_H__

#include <ArduinoJson.h>
#include "LittleFS.h"
#include "global.h"
#include "tasks/task_wifi.h"

// Kiểm tra sự tồn tại và tính hợp lệ của file cấu hình wifi
// check: true để in debug, false để kiểm tra im lặng
bool check_info_File(bool check);

// Đọc thông tin Wifi (SSID, Pass) từ file và nạp vào biến toàn cục
void Load_info_File();

// Xóa file cấu hình (Reset Wifi về mặc định/Chế độ cài đặt)
void Delete_info_File();

// Lưu thông tin Wifi mới vào bộ nhớ Flash (LittleFS)
void Save_info_File(String WIFI_SSID, String WIFI_PASS);

#endif