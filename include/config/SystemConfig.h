// include/config/SystemConfig.h
#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include <Arduino.h>

// --- PIN DEFINITIONS (Phù hợp với ESP32 DevKit V1) ---
#define PIN_SERVO       13
#define PIN_LCD_SDA     33
#define PIN_LCD_SCL     32
// RFID Pins (SPI)
#define PIN_RFID_SDA    5
#define PIN_RFID_SCK    18
#define PIN_RFID_MOSI   23
#define PIN_RFID_MISO   19
#define PIN_RFID_RST    4

// --- SYSTEM SETTINGS ---
#define LCD_ADDRESS     0x27
#define LCD_COLS        16
#define LCD_ROWS        2
#define SERIAL_BAUD     115200

// --- QUEUE SETTINGS ---
#define QUEUE_LENGTH    10
#define QUEUE_WAIT_MS   10

// --- DATA TYPES ---
enum EventType {
    EVENT_IDLE,
    EVENT_SCAN_RFID,    // Quét được thẻ
    EVENT_UPDATE_LCD,   // Cần vẽ lại màn hình
    EVENT_SYNC_CLOUD,    // Cần nén và gửi MQTT
    EVENT_EXPORT_CMD
};

struct SystemMessage {
    EventType type;
    char payload[32];   // Dùng char array thay vì String để an toàn bộ nhớ trong RTOS
    int value;
};

#endif