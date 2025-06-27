#pragma once
#include <Arduino.h>
#include <LittleFS.h>
// #include "telegramm.h"
#include "AsyncTelegram2.h"
#define TIMEFORMAT "%d.%m.%y %H:%M"
extern AsyncTelegram2 myBot;

class LOGGER : public Print
{
private:
    const char *logFile;
    void timeString(char *timeStr, size_t size);
    const char *getResetReasonStr(esp_reset_reason_t reason);
    const char *getWiFiStatusStr(wl_status_t statatus);
    void printToFile(const char *filename, const char *str);

public:
    LOGGER() = delete;
    LOGGER(const char *logFile) : logFile(logFile) { LittleFS.begin(); }
    ~LOGGER() {};
    size_t write(uint8_t ch) override;
    size_t write(const uint8_t *buffer, size_t size) override;
    void wifi_disconect();
    void reset_reason();
    bool deleteLog() { return LittleFS.remove(logFile); }
};

void LOGGER::wifi_disconect()
{
    static bool oneWriteConected = false, oneWriteDisconected = false;

    if (!oneWriteConected and WiFi.status() == WL_CONNECTED)
    {
        print(F("Wifi conected."));
        oneWriteConected = true;
        oneWriteDisconected = false;
    }

    if (!oneWriteDisconected and WiFi.status() != WL_CONNECTED)
    {
        printf("Wifi disconected. WiFi.status = %s", getWiFiStatusStr(WiFi.status())); // wl_status_t statatus = WiFi.status();
        oneWriteDisconected = true;
        oneWriteConected = false;
    }
}

void LOGGER::printToFile(const char *filename, const char *str)
{
    char timeStr[20];
    timeString(timeStr, sizeof(timeStr));
    if (LittleFS.totalBytes() - LittleFS.usedBytes() < 100)
    {
        TBMessage msg;
        File file = LittleFS.open(filename, "a");
        if (file)
        {
            file.print(F("Скінчилося місце у ФС."));
            myBot.sendMessage(msg, F("Скінчилося місце у ФС. Файл логу видалений."));
            myBot.sendDocument(msg, file, file.size(), AsyncTelegram2::DocumentType::TEXT, file.name());
            file.close();
        }
        LittleFS.remove(filename);
    }
    File file = LittleFS.open(filename, "a");
    file.printf("%s %s\n", timeStr, str);
    file.close();
}

// Логируем причину перезагрузки
void LOGGER::reset_reason()
{
    esp_reset_reason_t reason = esp_reset_reason();
    const char *reasonStr = getResetReasonStr(reason);
    print(reasonStr);
}

void LOGGER::timeString(char *timeStr, size_t size)
{
    static u32_t prevMillis = millis();
    struct tm timeinfo;
    while (!getLocalTime(&timeinfo, 1000) and (millis() - prevMillis) < 15000)
    {

        Serial.println(F("Подключение к NTP..."));
    }
    // Форматируем время для логирования
    strftime(timeStr, size, TIMEFORMAT, &timeinfo);
}

const char *LOGGER::getResetReasonStr(esp_reset_reason_t reason)
{
    #define FSH (const char *)F
    switch (reason)
    {
    case ESP_RST_UNKNOWN:
        return FSH("ESP_RST_UNKNOWN");
    case ESP_RST_POWERON:
        return FSH ("Power-on reset");
    case ESP_RST_EXT:
        return FSH("External reset");
    case ESP_RST_SW:
        return FSH("Software reset");
    case ESP_RST_PANIC:
        return FSH("Exception/panic reset");
    case ESP_RST_INT_WDT:
        return FSH("Interrupt watchdog reset");
    case ESP_RST_TASK_WDT:
        return FSH("Task watchdog reset");
    case ESP_RST_WDT:
        return FSH("Other watchdog reset");
    case ESP_RST_DEEPSLEEP:
        return FSH("Wake from deep sleep");
    case ESP_RST_BROWNOUT:
        return FSH("Brownout reset");
    case ESP_RST_SDIO:
        return FSH("SDIO reset");
    default:
        return FSH("Unknown reset");
    }
    #undef FSH
}

const char *LOGGER::getWiFiStatusStr(wl_status_t statatus)
{
    #define FSH (const char *)F
    switch (statatus)
    {
    case WL_NO_SHIELD:
        return FSH("WL NO SHIELD");
    case WL_IDLE_STATUS:
        return FSH("WL IDLE STATUS");
    case WL_NO_SSID_AVAIL:
        return FSH("WL NO SSID AVAIL");
    case WL_SCAN_COMPLETED:
        return FSH("WL SCAN COMPLETED");
    case WL_CONNECTED:
        return FSH("WL CONNECTED");
    case WL_CONNECT_FAILED:
        return FSH("WL CONNECT FAILED");
    case WL_CONNECTION_LOST:
        return FSH("WL CONNECTION LOST");
    case WL_DISCONNECTED:
        return FSH("WL DISCONNECTED");
    default:
        return FSH("Unknown WL status");
    }
     #undef FSH
}

size_t LOGGER::write(uint8_t ch)
{
    static String str;
    if ((char)ch == '\n')
    {
        printToFile(logFile, str.c_str());
        str = "";
        return 1;
    }
    str += (char)ch;
    return 1;
}

size_t LOGGER::write(const uint8_t *buffer, size_t size)
{
    printToFile(logFile, (const char *)buffer);
    return strlen((const char *)buffer);
}
