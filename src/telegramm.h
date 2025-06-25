#pragma once
#include <Arduino.h>
#include "AsyncTelegram2.h"
#include <time.h>
#define MYTZ "EET-2EEST,M3.5.0/3,M10.5.0/4" // Киев https://remotemonitoringsystems.ca/time-zone-abbreviations.php
#include <LittleFS.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include "struct.h"

WiFiClientSecure client;
AsyncTelegram2 myBot(client);
// const char *token = "1897971493:AAGpIQy5WUlBXfdANURukWPJBgL2QcXvFjE"; // Telegram token
// https://api.telegram.org/bot1897971493:AAGpIQy5WUlBXfdANURukWPJBgL2QcXvFjE/getUpdates
// int64_t userid = 426464015;
ReplyKeyboard keyboard;             // reply keyboard object helper
InlineKeyboard inlineKeyboard;           // inline keyboard object helper
bool isKeyboardActive;                // store if the reply keyboard is shown
#define SWITH_ON_CALLBACK "SwithON"   // callback data sent when "LIGHT ON" button is pressed
#define SWITH_OFF_CALLBACK "SwithOFF" // callback data sent when "LIGHT OFF" button is pressed
extern RTC_NOINIT_ATTR bool tuyaSwith;
extern time_t startTime, endTime;
extern VoltmetrESP32 battery;
void setupTelegramBot()
{
    if (strlen(settings.telegramBotToken) < 40 or WiFi.getMode() == WIFI_MODE_AP)
        return;
    // configTzTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
    client.setCACert(telegram_cert);
    // Set the Telegram bot properties
    myBot.setUpdateTime(1000);

    myBot.setTelegramToken(settings.telegramBotToken);
    // Check if all things are ok
    Serial.print("\nTest Telegram connection... ");
    myBot.begin() ? Serial.println("OK") : Serial.println("NOK");
    Serial.print("Bot name: @");
    Serial.println(myBot.getBotName());

    // Add reply keyboard
    isKeyboardActive = false;
    keyboard.addButton("Керування розеткою");
    keyboard.addRow();
    keyboard.addButton("Інформація пристрою");
    keyboard.addRow();
    keyboard.addButton("Завантажити log.txt");
    keyboard.addRow();
    keyboard.addButton("Встановити координати GPS", KeyboardButtonLocation);
    keyboard.enableResize();

    // Add sample inline keyboard
    inlineKeyboard.addButton("ON", SWITH_ON_CALLBACK, KeyboardButtonQuery);
    inlineKeyboard.addButton("OFF", SWITH_OFF_CALLBACK, KeyboardButtonQuery);
}

void loopTelegram()
{
    static int tgtokenLen = strlen(settings.telegramBotToken);
    if (tgtokenLen < 40 or WiFi.getMode() == WIFI_MODE_AP)
        return;
    // local variable to store telegram message data
    TBMessage msg;
    if (myBot.getNewMessage(msg))
    {

        MessageType msgType = msg.messageType;
        String msgText = msg.text;

        switch (msgType)
        {
        case MessageText:
            // received a text message
            Serial.print("\nText message received: ");
            Serial.println(msgText);
            if (msgText.equalsIgnoreCase("/log"))
            {
                File file = LittleFS.open("/log.txt", "r");
                if (file)
                {
                    myBot.sendDocument(msg, file, file.size(), AsyncTelegram2::DocumentType::TEXT, file.name());
                    file.close();
                }
                else
                    myBot.sendMessage(msg, "Відсутній log.txt");
            }
            else if (msgText.equalsIgnoreCase("Інформація пристрою"))
            {

                char infoMsg[200];
                char workTime[30];
                time(&endTime);
                time_t elapsed = endTime - startTime;
                const int len = 50;
                char timeStr[len];
                sprintf(timeStr, "%2d д. ", elapsed / 86400);
                struct tm *timeInfo = gmtime(&elapsed);
                strftime(timeStr + strlen(timeStr), len, "%H:%M:%S", timeInfo);
                sprintf(infoMsg, "Напруга батареї: %.3f.\nUmax = %.3f\nUmin = %.3f.\nРозетка %s.\nЧас роботи: %s", battery.getVoltage(), settings.bataryVoltageMax, settings.bataryVoltageMin, tuyaSwith ? "увімкнута" : "вимкнута", timeStr);
                myBot.sendMessage(msg, infoMsg);
            }

            else if (msgText.equalsIgnoreCase("/keyboard"))
            {
                // the user is asking to show the reply keyboard --> show it
                myBot.sendMessage(msg, "Оберіть дію:", keyboard);
                isKeyboardActive = true;
            }
            else if (msgText.equalsIgnoreCase("Керування розеткою"))
            {
                myBot.sendMessage(msg, "Керування розеткою:", inlineKeyboard);
            }
            else if (msgText.equalsIgnoreCase("Завантажити log.txt"))
            {
                File file = LittleFS.open("/log.txt", "r");
                if (file)
                {
                    myBot.sendDocument(msg, file, file.size(), AsyncTelegram2::DocumentType::TEXT, file.name());
                    file.close();
                }
                else
                    myBot.sendMessage(msg, "Відсутній log.txt");
            }

            // check if the reply keyboard is active
            else if (isKeyboardActive)
            {
                Serial.printf("isKeyboardActive msg: %s", msgText);
                if (msgText.equalsIgnoreCase("Завантажити log.txt"))
                {
                    Serial.println("Завантажити log!!!!");
                    File file = LittleFS.open("/log.txt", "r");
                    if (file)
                    {
                        myBot.sendDocument(msg, file, file.size(), AsyncTelegram2::DocumentType::TEXT, file.name());
                        file.close();
                    }
                    else
                        myBot.sendMessage(msg, "Відсутній log.txt");
                }
                else if (msgText.equalsIgnoreCase("Інформація пристрою"))
                {
                    char infoMsg[200];
                    char workTime[30];
                    time(&endTime);
                    time_t elapsed = endTime - startTime;
                    const int len = 50;
                    char timeStr[len];
                    sprintf(timeStr, "%2d д. ", elapsed / 86400);
                    struct tm *timeInfo = gmtime(&elapsed);
                    strftime(timeStr + strlen(timeStr), len, "%H:%M:%S", timeInfo);
                    sprintf(infoMsg, "Напруга батареї: %.3f.\nUmax = %.3f\nUmin = %.3f.\nРозетка %s.\nЧас роботи: %s", battery.getVoltage(), settings.bataryVoltageMax, settings.bataryVoltageMin, tuyaSwith ? "увімкнута" : "вимкнута", timeStr);
                    myBot.sendMessage(msg, infoMsg);
                }
                else
                {
                    // print every others messages received
                    myBot.sendMessage(msg, "Клавіатура: /keyboard\n\nКерування розеткою: /Management\n\nінформація: /info\n\nСкачати лог: /log");
                }
            }

            // the user write anything else and the reply keyboard is not active --> show a hint message
            else
            {
                myBot.sendMessage(msg, "Оберіть дію:", keyboard);
                // myBot.sendMessage(msg, "Клавіатура: /reply_keyboard\n\nКерування розеткою: /Management\n\nінформація: /info\n\nСкачати лог: /log");
            }
            break;

        case MessageQuery:
            // received a callback query message
            msgText = msg.callbackQueryData;
            Serial.print("\nCallback query message received: ");
            Serial.println(msg.callbackQueryData);

            if (msgText.equalsIgnoreCase(SWITH_ON_CALLBACK))
            {
                const char *PROGMEM mesage = "Telegram set Rozetka.on";
                Serial.println(mesage);
                if (Rozetka.on())
                {
                    tuyaSwith = HIGH;
                    settings.tuyaSwith = HIGH;
                    Logger.print(mesage);
                    myBot.endQuery(msg, "Увімкнуто!");
                }
                else
                {
                    myBot.endQuery(msg, "Не вдалося увімкнути.");
                }
            }
            else if (msgText.equalsIgnoreCase(SWITH_OFF_CALLBACK))
            {
                const char *PROGMEM mesage = "Telegram set Rozetka.off";
                Serial.println(mesage);
                if (Rozetka.off())
                {
                    tuyaSwith = LOW;
                    settings.tuyaSwith = LOW;
                    Logger.print(mesage);
                    myBot.endQuery(msg, "Вимкнуто!");
                }
                else
                {
                    myBot.endQuery(msg, "Не вдалося вимкнути.");
                }
            }
            break;

        case MessageLocation:
        {
            // received a location message
            String reply = "Отримані координати:\n Довгота: ";
            reply += msg.location.longitude;
            reply += ";\n Широта: ";
            reply += msg.location.latitude;
            Serial.println(reply);
            myBot.sendMessage(msg, reply);
            break;
        }
        default:
            break;
        }
    }
}
