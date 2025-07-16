#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "struct.h"
#include "StorageManager.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "Tuya.h"
#include <logger.h>

extern LOGGER Logger;
extern const char *settingsFile;

extern AsyncWebSocket webSocket;
extern bool saveFlagMeasurementDataToFile;
extern Tuya Rozetka;
typedef bool (*TuyaSwitchCallback)(bool state);
// extern TuyaSwitchCallback onTuyaUserEvent;
TuyaSwitchCallback onTuyaUserEvent = nullptr;

extern SaveToFsStruct settings;
extern RTC_DATA_ATTR bool tuyaSwith;
StorageManager<SaveToFsStruct> SaveSettingsToLFS(settings, settingsFile);

void webSocketSendeData()
{
  JsonDocument doc;

  doc["tuyaSwith"] = tuyaSwith;
  doc["wifiSsid"] = settings.wifiSsid;
  doc["wifiPass"] = settings.wifiPass;
  doc["tuyaDevId"] = settings.tuyaDevId;
  doc["tuyaLocalKey"] = settings.tuyaLocalKey;
  doc["tuyaIp1"] = settings.tuyaIp1;
  doc["tuyaIp2"] = settings.tuyaIp2;
  doc["tuyaIp3"] = settings.tuyaIp3;
  doc["tuyaIp4"] = settings.tuyaIp4;
  doc["timer"] = settings.timer;
  doc["bataryVoltageMin"] = settings.bataryVoltageMin;
  doc["bataryVoltageMax"] = settings.bataryVoltageMax;
  doc["telegramBotToken"] = settings.telegramBotToken;
  String jsonData;
  serializeJson(doc, jsonData);
  Serial.println(jsonData);
  webSocket.textAll(jsonData);
}

void webSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  Serial.print("webSocketEvent:");
  if (type == WS_EVT_DATA)
  {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
    {
      data[len] = '\0';
      JsonDocument doc; // 768
      deserializeJson(doc, (char *)data);
      serializeJsonPretty(doc, Serial);

      if (doc["getSettings"].is<bool>())
      {
        webSocketSendeData();
      }
      // else if (doc["tuyaSwithUserPush"].is<bool>()){
      // onTuyaUserEvent(doc["tuyaSwithUserPush"] | false);
      // }
      else
      {
        tuyaSwith = settings.tuyaSwith = doc["tuyaSwith"] | false;

        strlcpy(settings.wifiSsid, doc["wifiSsid"] | "", sizeof(settings.wifiSsid));
        strlcpy(settings.wifiPass, doc["wifiPass"] | "", sizeof(settings.wifiPass));
        strlcpy(settings.tuyaDevId, doc["tuyaDevId"] | "", sizeof(settings.tuyaDevId));
        strlcpy(settings.tuyaLocalKey, doc["tuyaLocalKey"] | "", sizeof(settings.tuyaLocalKey));
        strlcpy(settings.telegramBotToken, doc["telegramBotToken"] | "", sizeof(settings.telegramBotToken));

        settings.tuyaIp1 = doc["tuyaIp1"].as<int>();
        settings.tuyaIp2 = doc["tuyaIp2"].as<int>();
        settings.tuyaIp3 = doc["tuyaIp3"].as<int>();
        settings.tuyaIp4 = doc["tuyaIp4"].as<int>();

        settings.timer = doc["timer"].as<int>();
        settings.bataryVoltageMin = doc["bataryVoltageMin"].as<float>();
        settings.bataryVoltageMax = doc["bataryVoltageMax"].as<float>();
        if (SaveSettingsToLFS.write(settings))
        {
          Logger.print("New settings write to FS. Reload...");
          delay(100);
          ESP.restart();
        }
      }
    }
  }
}

void webSocketEventVoltage(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
}