#pragma once
#include <Arduino.h>
#define FSH (const char *)F
#include "AsyncTelegram2.h"
#include <time.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include "struct.h"
#include "VoltmeterESP32.h"
#include "logger.h"
#include "Tuya.h"

WiFiClientSecure client;
AsyncTelegram2 myBot(client);
ReplyKeyboard keyboard;               // reply keyboard object helper
InlineKeyboard inlineKeyboard;        // inline keyboard object helper
bool isKeyboardActive = true;         // store if the reply keyboard is shown
#define SWITH_ON_CALLBACK FSH("SwithON")   // callback data sent when "LIGHT ON" button is pressed
#define SWITH_OFF_CALLBACK FSH("SwithOFF") // callback data sent when "LIGHT OFF" button is pressed
extern RTC_NOINIT_ATTR bool tuyaSwith;
extern time_t startTime, endTime;
extern VoltmetrESP32 battery;
extern Tuya Rozetka;
extern LOGGER Logger;
extern const char *logFile;

const char *getResetReasonStr(esp_reset_reason_t reason)
{
    #define FSH (const char *)F
    switch (reason)
    {
    case ESP_RST_UNKNOWN:
        return FSH("Невизначена");
    case ESP_RST_POWERON:
        return FSH("Після сбросу");
    case ESP_RST_EXT:
        return FSH("Зовнішній сброс");
    case ESP_RST_SW:
        return FSH("Програмний сброс");
    case ESP_RST_PANIC:
        return FSH("Виключення/паніка");
    case ESP_RST_INT_WDT:
        return FSH("Переривання сторожевого таймера");
    case ESP_RST_TASK_WDT:
        return FSH("Задача сторожевого таймера");
    case ESP_RST_WDT:
        return FSH("Інше спрацювання сторожевого таймера");
    case ESP_RST_DEEPSLEEP:
        return FSH("Просипання після глибокого сну");
    case ESP_RST_BROWNOUT:
        return FSH("Сброс через нестабільне живлення");
    case ESP_RST_SDIO:
        return FSH("SDIO сброс");
    default:
        return FSH("Невідомий сброс");
    }
}

bool setupTelegramBot()
{
    if (strlen(settings.telegramBotToken) < 40 or WiFi.getMode() == WIFI_MODE_AP)
        return false;
    client.setCACert(telegram_cert);
    myBot.setUpdateTime(1000);
    myBot.setTelegramToken(settings.telegramBotToken);
    // Check if all things are ok
    if (!myBot.begin())
        return false;
    // Налаштування клавіатур
    keyboard.addButton(FSH("Інформація пристрою"));
    keyboard.addRow();
    keyboard.addButton(FSH("Керування розеткою"));
    keyboard.addRow();
    keyboard.addButton(FSH("Завантажити log.txt"));
    keyboard.addRow();
    keyboard.addButton(FSH("Встановити координати GPS"), KeyboardButtonLocation);
    keyboard.enableResize();
    // Add sample inline keyboard
    inlineKeyboard.addButton(FSH("ON"), SWITH_ON_CALLBACK, KeyboardButtonQuery);
    inlineKeyboard.addButton(FSH("OFF"), SWITH_OFF_CALLBACK, KeyboardButtonQuery);
    char reasonMsg[100];
    sprintf(reasonMsg, FSH("Пристрій завантажився. Причина запуску : %s."), getResetReasonStr(esp_reset_reason()));
    TBMessage msg;
    myBot.sendMessage(msg, reasonMsg, keyboard);
    return true;
}

void loopTelegram()
{
    static int tgtokenLen = strlen(settings.telegramBotToken);
    if (tgtokenLen < 40 or WiFi.getMode() == WIFI_MODE_AP)
        return;

    TBMessage msg;
    if (myBot.getNewMessage(msg))
    {
        MessageType msgType = msg.messageType;
        String msgText = msg.text;

        switch (msgType)
        {
        case MessageText:
            // received a text message

            if (msgText.equalsIgnoreCase(F("/keyboard")) or msgText.equalsIgnoreCase(F("/start")))
            {
                myBot.sendMessage(msg, FSH("Оберіть дію:"), keyboard);
                isKeyboardActive = true;
            }

            // check if the reply keyboard is active
            else if (isKeyboardActive)
            {
                if (msgText.equalsIgnoreCase(FSH("Завантажити log.txt")))
                {
                    File file = LittleFS.open(logFile, "r");
                    if (file)
                    {
                        myBot.sendDocument(msg, file, file.size(), AsyncTelegram2::DocumentType::TEXT, file.name());
                        file.close();
                    }
                    else
                        myBot.sendMessage(msg, FSH("Відсутній log.txt"));
                }
                else if (msgText.equalsIgnoreCase(FSH("Керування розеткою")))
                {
                    myBot.sendMessage(msg, FSH("Керування розеткою:"), inlineKeyboard);
                }
                else if (msgText.equalsIgnoreCase(FSH("Інформація пристрою")))
                {
                    char infoMsg[200];
                    char workTime[30];
                    time(&endTime);
                    time_t elapsed = endTime - startTime;
                    constexpr int len = 50;
                    char timeStr[len];
                    sprintf(timeStr, FSH("%2d д. "), elapsed / (time_t)(60 * 60 * 24));
                    struct tm *timeInfo = gmtime(&elapsed);
                    strftime(timeStr + strlen(timeStr), len, FSH("%H:%M:%S"), timeInfo);
                    size_t FSfree = LittleFS.totalBytes() - LittleFS.usedBytes();
                    sprintf(infoMsg, FSH("Напруга батареї: %.3f В.\nUмакс = %.1f В.\nUмин = %.1f В.\nРозетка %s.\nЧас роботи: %s\nВільна ОЗУ %u б.\nВільна ФС %u б."), battery.getVoltage(), settings.bataryVoltageMax, settings.bataryVoltageMin, tuyaSwith ? "увімкнута" : "вимкнута", timeStr, ESP.getMinFreeHeap(), FSfree);
                    myBot.sendMessage(msg, infoMsg);
                }
                else
                {
                    // print every others messages received
                    myBot.sendMessage(msg, FSH("Хибна команда. Оберіть команду на клавіатурі."), keyboard);
                }
            }

            // the user write anything else and the reply keyboard is not active --> show a hint message
            else
            {
                myBot.sendMessage(msg, FSH("Оберіть дію:"), keyboard);
            }
            break;

        case MessageQuery:
            // received a callback query message
            msgText = msg.callbackQueryData;

            if (msgText.equalsIgnoreCase(SWITH_ON_CALLBACK))
            {
                const char *PROGMEM mesage = FSH("Telegram set Rozetka.on");
                if (Rozetka.on())
                {
                    tuyaSwith = HIGH;
                    settings.tuyaSwith = HIGH;
                    Logger.print(mesage);
                    myBot.endQuery(msg, FSH("Увімкнуто!"));
                }
                else
                {
                    myBot.endQuery(msg, FSH("Не вдалося увімкнути."));
                }
            }
            else if (msgText.equalsIgnoreCase(SWITH_OFF_CALLBACK))
            {
                const char *PROGMEM mesage = FSH("Telegram set Rozetka.off");
                if (Rozetka.off())
                {
                    tuyaSwith = LOW;
                    settings.tuyaSwith = LOW;
                    Logger.print(mesage);
                    myBot.endQuery(msg, FSH("Вимкнуто!"));
                }
                else
                {
                    const char *PROGMEM mesage = FSH("Не вдалося вимкнути.");
                    myBot.endQuery(msg, mesage);
                }
            }
            break;

        case MessageLocation:
        {
            // received a location message
            String reply = F("Отримані координати:\n Довгота: ");
            reply += msg.location.longitude;
            reply += F(";\n Широта: ");
            reply += msg.location.latitude;
            myBot.sendMessage(msg, reply);
            break;
        }
        default:
            break;
        }
    }
}
#undef FSH

