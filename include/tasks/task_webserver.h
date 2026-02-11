#ifndef TASK_WEBSERVER_H
#define TASK_WEBSERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include "LittleFS.h" 
#include "global.h"
#include "check_info.h"

// --- CÁC HÀM XỬ LÝ LOGIC (API HANDLERS) ---

// Xử lý request lấy dữ liệu (thường là JSON danh sách sản phẩm hoặc trạng thái)
void handleGetData(AsyncWebServerRequest *request);

// Cấu hình các route (đường dẫn) cho WebServer
void setupWebServer();

// Hàm thực thi chính của Task WebServer (chạy trong FreeRTOS)
void TaskWebServerFunc(void *pvParameters);

// Xử lý request chuyển đổi chế độ hoạt động (Ví dụ: Chuyển Mode nhập/xuất kho)
void handleSetMode(AsyncWebServerRequest *request);

// Xử lý các hành động điều khiển cụ thể (Update, Delete, Add sản phẩm từ Web)
void handleAction(AsyncWebServerRequest *request);

#endif