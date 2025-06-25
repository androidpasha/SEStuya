#pragma once
#include <Arduino.h>
#include <LittleFS.h>

class LOG
{
private:
    const char *logFile;
    void timeString(char *timeStr, size_t size);
    const char *getResetReasonStr(esp_reset_reason_t reason);
    const char *getWiFiStatusStr(wl_status_t statatus);
    void printToFile(const char *filename, const char *str);

public:
    LOG() = delete;
    LOG(const char *logFile) : logFile(logFile) { LittleFS.begin(); }
    ~LOG() {};
    void wifi_disconect();
    void reset_reason();
    void user_text(const char *msg);
    bool deleteLog() { return LittleFS.remove(logFile); }
};

void LOG::user_text(const char *msg)
{
    printToFile(logFile, msg);
}

void LOG::wifi_disconect()
{
    static bool oneWriteConected = false, oneWriteDisconected = false;

    if (!oneWriteConected and WiFi.status() == WL_CONNECTED)
    {
        printToFile(logFile, "Wifi conected.");
        oneWriteConected = true;
        oneWriteDisconected = false;
    }

    if (!oneWriteDisconected and WiFi.status() != WL_CONNECTED)
    {
        char buf[50]{};
        sprintf(buf, "Wifi disconected. WiFi.status = %s", getWiFiStatusStr(WiFi.status())); //wl_status_t statatus = WiFi.status();
        printToFile(logFile, buf);
        oneWriteDisconected = true;
        oneWriteConected = false;
    }
}

void LOG::printToFile(const char *filename, const char *str)
{
    char timeStr[20];
    timeString(timeStr, sizeof(timeStr));
    if (LittleFS.totalBytes() - LittleFS.usedBytes() < 100)
        LittleFS.remove(filename);
    File file = LittleFS.open(filename, "a");
    file.printf("%s %s\n", timeStr, str);
    file.close();
}

// Логируем причину перезагрузки
void LOG::reset_reason()
{
    esp_reset_reason_t reason = esp_reset_reason();
    const char *reasonStr = getResetReasonStr(reason);
    printToFile(logFile, reasonStr);
}

void LOG::timeString(char *timeStr, size_t size)
{
    struct tm timeinfo;
    while (!getLocalTime(&timeinfo, 1000))
    {
        Serial.println("Подключение к NTP...");
    }
    // Форматируем время для логирования
    strftime(timeStr, size, "%Y-%m-%d %H:%M:%S", &timeinfo);
}

const char *LOG::getResetReasonStr(esp_reset_reason_t reason)
{
    switch (reason)
    {
    case ESP_RST_UNKNOWN:
        return "ESP_RST_UNKNOWN";
    case ESP_RST_POWERON:
        return "Power-on reset";
    case ESP_RST_EXT:
        return "External reset";
    case ESP_RST_SW:
        return "Software reset";
    case ESP_RST_PANIC:
        return "Exception/panic reset";
    case ESP_RST_INT_WDT:
        return "Interrupt watchdog reset";
    case ESP_RST_TASK_WDT:
        return "Task watchdog reset";
    case ESP_RST_WDT:
        return "Other watchdog reset";
    case ESP_RST_DEEPSLEEP:
        return "Wake from deep sleep";
    case ESP_RST_BROWNOUT:
        return "Brownout reset";
    case ESP_RST_SDIO:
        return "SDIO reset";
    default:
        return "Unknown reset";
    }
}

const char *LOG::getWiFiStatusStr(wl_status_t statatus)
{
    switch (statatus)
    {
    case WL_NO_SHIELD:
        return "WL NO SHIELD";
    case WL_IDLE_STATUS:
        return "WL IDLE STATUS";
    case WL_NO_SSID_AVAIL:
        return "WL NO SSID AVAIL";
    case WL_SCAN_COMPLETED:
        return "WL SCAN COMPLETED";
    case WL_CONNECTED:
        return "WL CONNECTED";
    case WL_CONNECT_FAILED:
        return "WL CONNECT FAILED";
    case WL_CONNECTION_LOST:
        return "WL CONNECTION LOST";
    case WL_DISCONNECTED:
        return "WL DISCONNECTED";
    default:
        return "Unknown WL status";
    }
}