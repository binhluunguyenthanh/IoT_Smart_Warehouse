#include "global.h"

String lastScannedRFID = "";
int currentSystemMode = 0; 

float currentTemperature = 0.0;
float currentHumidity = 0.0;

// Khởi tạo các đối tượng và Mutex
HalLCD myLCD; 
SemaphoreHandle_t xLcdMutex = NULL;
SemaphoreHandle_t xSerialMutex = NULL;

void updateLeds(){
    digitalWrite(PIN_LED_CHECK, LOW);
    digitalWrite(PIN_LED_IMPORT, LOW);      
    digitalWrite(PIN_LED_SELL, LOW);

    switch(currentSystemMode){
        case 0: digitalWrite(PIN_LED_CHECK, HIGH); break;  
        case 1: digitalWrite(PIN_LED_IMPORT, HIGH); break; 
        case 2: digitalWrite(PIN_LED_SELL, HIGH); break;   
    }
}