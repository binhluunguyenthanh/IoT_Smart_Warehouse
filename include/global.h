#ifndef GLOBAL_H
#define GLOBAL_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include "config/SystemConfig.h"
#include "hal/HalLCD.h" // Nhúng thư viện LCD vào đây để mọi Task đều thấy

// --- CÁC KIỂU DỮ LIỆU DÙNG CHUNG ---
enum EventType {
    EVENT_IDLE,
    EVENT_SCAN_RFID
    // Đã xóa EVENT_UPDATE_LCD vì không xài Queue cho LCD nữa
};

struct SystemMessage {
    EventType type;
    char payload[32];
    int value;
};

// --- CÁC BIẾN TOÀN CỤC ---
extern String lastScannedRFID; 
extern int currentSystemMode;  // 0=Check, 1=Import, 2=Export

extern float currentTemperature;
extern float currentHumidity;
   
extern void updateLeds();

// --- TÀI NGUYÊN PHẦN CỨNG DÙNG CHUNG ---
extern HalLCD myLCD;                    // Màn hình LCD toàn cục
extern SemaphoreHandle_t xLcdMutex;     // Chìa khóa bảo vệ màn hình LCD
extern SemaphoreHandle_t xSerialMutex;  // Chìa khóa bảo vệ cổng Serial

#endif