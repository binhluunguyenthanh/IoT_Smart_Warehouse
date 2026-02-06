// include/config/SystemConfig.h
#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include <Arduino.h>

// --- PIN DEFINITIONS ---
#define PIN_SERVO       13
#define PIN_LCD_SDA     33
#define PIN_LCD_SCL     32
// RFID Pins (SPI)
#define PIN_RFID_SDA    5
#define PIN_RFID_SCK    18
#define PIN_RFID_MOSI   23
#define PIN_RFID_MISO   19
#define PIN_RFID_RST    4
//LED MODE
#define PIN_LED_CHECK   2 //GREEN
#define PIN_LED_IMPORT  15 //YELLOW
#define PIN_LED_SELL    13 //RED 
// --- SYSTEM SETTINGS ---
#define LCD_ADDRESS     0x27
#define LCD_COLS        16
#define LCD_ROWS        2
#define SERIAL_BAUD     115200

// --- QUEUE SETTINGS ---
#define QUEUE_LENGTH    10
#define QUEUE_WAIT_MS   10
// --- SENSOR SETTINGS ---
#define PIN_DHT         16
#define DHT_TYPE        DHT22
// --- DATA TYPES ---
enum EventType {
    EVENT_IDLE,         // Trạng thái nhàn rỗi
    EVENT_SCAN_RFID,    // Đọc thẻ RFID
    EVENT_UPDATE_LCD,   // Cập nhật hiển thị LCD
    EVENT_SYNC_CLOUD,   // Đồng bộ dữ liệu lên Cloud
};

struct SystemMessage {
    EventType type;
    char payload[32];   
    int value;
};

#endif