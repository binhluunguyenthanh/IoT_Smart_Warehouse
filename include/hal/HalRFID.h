#ifndef HAL_RFID_H
#define HAL_RFID_H

#include <SPI.h>
#include <MFRC522.h>
#include "config/SystemConfig.h"

class HalRFID {
private:
    MFRC522* mfrc522; 
public:
    HalRFID();
    void init();
    bool checkTag();
    String getTagUID();
};

#endif