#pragma once
#include <Arduino.h>
#include "StorageManager.h"

struct SaveToFsStruct 
{
    bool
        tuyaSwith;
    char
        wifiSsid[32],
        wifiPass[63],
        tuyaDevId[22],
        tuyaLocalKey[18];
    uint8_t
        tuyaIp1,
        tuyaIp2,
        tuyaIp3,
        tuyaIp4,
        timer;
    float
        bataryVoltageMin,
        bataryVoltageMax;
    char
        telegramBotToken[50];
    char *getTuyaIp()
    {
        sprintf(tuyaIp, "%d.%d.%d.%d", tuyaIp1, tuyaIp2, tuyaIp3, tuyaIp4);
        return tuyaIp;
    }

private:
    char tuyaIp[16];
} settings;