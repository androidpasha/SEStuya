// Код подключается к умной розетке Tuya  local по протоколу 3.3
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include "Tuya.h"
#include "VoltmeterESP32.h"
#include "webSocketFunction.h"
#include <ArduinoJson.h>
#include <algorithm>
#include <vector>
#include "logger.h"
#include "telegramm.h"
#include "Ticker.h"
#define FSH (const char *)F
#define WIFISSID FSH("Tuya")
#define WIFIPASS FSH("1234567890")
const char *MYTZ PROGMEM = "EET-2EEST,M3.5.0/3,M10.5.0/4"; // Киев https://remotemonitoringsystems.ca/time-zone-abbreviations.php
constexpr float vRef = 3.271;                              // 3.3 вольта по умолчанию. Меняется между екземплярами есп.
constexpr u16_t R1 = 100, R2 = 20;                         // Резистивный делитель напряжения батареи. R1 верхний.
const char *logFile PROGMEM = "/web/log.txt";
const char *settingsFile PROGMEM = "/settings.bin";
//  float LONGITUDE = 35.186532, LATITUDE = 47.794765;// GPS координаты Запорожье-1 для определение восхода/заката на местности
//  byte TimeZone = 2;

LOGGER Logger(logFile);
AsyncWebServer webServer(80);
AsyncWebSocket webSocket("/ws");
AsyncWebSocket webSocketVoltage("/ws2");
Tuya Rozetka;
#ifdef ARDUINO_ESP32S3_DEV
#define ADCpin GPIO_NUM_11
#elif ARDUINO_ESP32_DEV
#define ADCpin GPIO_NUM_32
#endif
VoltmetrESP32 battery(ADCpin, R1, R2, vRef); // GPIO_NUM_32 для esp32 и GPIO_NUM_11 для ESP32S3
Ticker ewery30minTicker;
// ewery30minTicker([](){}, 30*60e3,0,1);
time_t startTime;
RTC_NOINIT_ATTR bool tuyaSwith;

// Прототипы
bool compareV(float V, float Vmin, float Vmax, bool *tuyaState, u32_t delayMin, Tuya *rozetka);
// void logWriteBatarryVoltag(){Logger.printf("%.3f", battery.getVoltage());};
void setup()
{
  Serial.begin(115200);
  delay(5000);
  __cplusplus; // Версия С++ 201703L
  // Применяем прочитанные настройки
  if (esp_reset_reason() == ESP_RST_POWERON)
    tuyaSwith = settings.tuyaSwith;
  else
    settings.tuyaSwith = tuyaSwith; // Берем значение из RTC.

  Serial.printf(FSH("\nTuyaSwith = %s.\n"), tuyaSwith ? FSH("true") : FSH("false"));
  Rozetka.set_tuya_ip(settings.getTuyaIp());
  Rozetka.set_devId(settings.tuyaDevId);
  Rozetka.set_LOCAL_KEY(settings.tuyaLocalKey);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  // WiFi.setHostname("esp");
  WiFi.begin(settings.wifiSsid, settings.wifiPass);
  Serial.printf(FSH("Спроба підключення до %s, пароль %s\n"), settings.wifiSsid, settings.wifiPass);
  while (WiFi.status() != WL_CONNECTED && millis() < 10000)
  {
    Serial.print(".");
    delay(100);
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.printf(FSH("Вдалє з'єднання з Wi-Fi, ip: %s\n"), WiFi.localIP().toString().c_str()); // const Printable& x
    configTzTime(MYTZ, FSH("time.google.com"), FSH("time.windows.com"), FSH("pool.ntp.org"));
    setupTelegramBot();
  }
  else
  {
    delay(100);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFISSID, WIFIPASS);
    Serial.printf(FSH("Підключення до %s не вдалося. Запущена точка доступу %s, пароль %s. Перейдіть на esp.local або 192.168.4.1 для налаштування.\n"), settings.wifiSsid, settings.wifiSsid, settings.wifiPass);
    Logger.print(F("WIFI_AP start."));
  }

  Logger.reset_reason();

  webServer.on(FSH("/tuyaSwithUserPush"), HTTP_GET, [](AsyncWebServerRequest *request)
               {    if (!request->hasParam(F("state"))) {request->send(400, F("text/plain"), F("Missing 'state' parameter")); return;}
    String stateStr = request->getParam(F("state"))->value();
    bool state = (stateStr == "1" || stateStr.equalsIgnoreCase(F("true")));
    // Возвращаем ответ
    if (Rozetka.relay(state)) {
        tuyaSwith = state;
        settings.tuyaSwith = state;
        Logger.printf(FSH("User push relay: %s"), state ? "On" : "Off");
        request->send(200, F("text/plain"), state ? "1" : "0");
    } else {
        request->send(500, F("text/plain"), F("Failed to switch Tuya"));
    } });

  // http://192.168.1.254/deletelog?deletelog=true
  webServer.on(FSH("/deletelog"), HTTP_GET, [](AsyncWebServerRequest *request)
               {    if (!request->hasParam(F("deletelog"))) {request->send(400, F("text/plain"), F("Missing 'state' parameter")); return;}
    String stateStr = request->getParam(F("deletelog"))->value();
    bool state = (stateStr == "1" || stateStr.equalsIgnoreCase(F("true")));
    // Возвращаем ответ
    if (state) {
        request->send(200, F("text/plain"), Logger.deleteLog() ? F("Log file deleted.") : F("Failed to delete log file"));
    } else {
        request->send(500, F("text/plain"), F("Incorrect deletion log request."));
    } });

  webServer.serveStatic("/", LittleFS, "/web") // відправляємо файли на HTTP запити з ФС
      .setDefaultFile(FSH("index.html"));

  webServer.begin();

  webServer.addHandler(&webSocket);
  webServer.addHandler(&webSocketVoltage);
  webSocket.onEvent(webSocketEvent);
  webSocketVoltage.onEvent(webSocketEventVoltage);
  // onTuyaUserEvent = TuyaSwithWeb_cb;                              // Назначаем callback при изменении слайдера в вебинтерфейсе

  JsonDocument doc = Rozetka.getStateJson();
  if (doc[F("dps")].is<JsonObject>())
    settings.tuyaSwith = tuyaSwith = doc[F("dps")]["1"];
  else
    while (!Rozetka.relay(tuyaSwith) and millis() < 10000) // Возвращаем розетку в сохраненное состояние
      yield();
  battery.begin(); // Включаем вольтметр
  time(&startTime);

  setCpuFrequencyMhz(80);
  ewery30minTicker.attach(30 * 60, []
                          { Logger.printf("%.3f", battery.getVoltage()); });
}

void loop()
{
  Logger.wifi_disconect();
  if (WiFi.getMode() != WIFI_MODE_AP and WiFi.status() != WL_CONNECTED)
  {
    static byte attempt = 0;
    if (attempt++ >= 50) // 5 минут
    {
      Logger.print(F("WiFi.status() != WL_CONNECTED -> ESP.restart()"));
      ESP.restart();
    }
    if (!WiFi.reconnect())
    {
      esp_wifi_disconnect();
      esp_wifi_stop();
      delay(100);
      esp_wifi_start();
      delay(100);
      esp_wifi_connect();
    }
    delay(6000);
  }

  // Ожидание клиентов в режиме WIFI_MODE_AP 5 мин
  static u32_t prevMillisAP = 0;
  if (WiFi.getMode() == WIFI_MODE_AP and (millis() - prevMillisAP) > (5UL * 60UL * 1000UL))
  { // 5 минут ждем пользователя
    if (WiFi.softAPgetStationNum() > 0)
      prevMillisAP = millis();
    else
    {
      Logger.print(F("WIFI_MODE_AP > 5 min without client`s -> ESP.restart()"));
      ESP.restart();
    }
  }

  // static u32_t prevMillisLog = 0;
  // if ((millis() - prevMillisLog) > (30UL * 60UL * 1000UL))
  // {
  //   prevMillisLog = millis();
  //   Logger.printf("%.3f", battery.getVoltage());
  // }

  static u32_t prevMillis = 0;
  if ((millis() - prevMillis) > 500) // Время обновления
  {
    prevMillis = millis();
    battery.measure();
    JsonDocument doc;
    doc["bat4"] = battery.getVoltage();
    time_t elapsed = time(nullptr) - startTime;
    doc["sistemTime"] = elapsed;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["FSfree"] = LittleFS.totalBytes() - LittleFS.usedBytes();
    doc["tuyaSwith"] = tuyaSwith;
    String jsonData;
    serializeJson(doc, jsonData);
    if (webSocketVoltage.count() and webSocketVoltage.availableForWriteAll())
      webSocketVoltage.textAll(jsonData);

    if (compareV(battery.getVoltage(), settings.bataryVoltageMin, settings.bataryVoltageMax, &tuyaSwith, settings.timer, &Rozetka))
    {
      settings.tuyaSwith = tuyaSwith;
      Logger.printf("Tuya.%s Ubat=%.3fV", tuyaSwith ? "on" : "off", battery.getVoltage());
    }
  }
  yield();
  loopTelegram();
}

inline bool compareV(float V, float Vmin, float Vmax, bool *tuyaState, u32_t delayMin, Tuya *rozetka = nullptr)
{
  if (rozetka == nullptr)
    return false;
  static u32_t prevMillis = 0;
  static bool startTimer = false;
  static bool nextState = LOW;

  if (startTimer == false and (V >= Vmax or V <= Vmin))
  {
    prevMillis = millis();
    startTimer = true;
    nextState = (V >= Vmax) ? HIGH : LOW;
  }

  if (startTimer == true and ((millis() - prevMillis) > (delayMin * 60 * 1000 + 1000)) and *tuyaState != nextState)
  {
    if (rozetka->relay(nextState))
    {
      *tuyaState = nextState;
      startTimer = false;
      return true;
    }
  }
  return false;
}

/*
{
  "devId": "08051804bcddc2542c4e",
  "dps": {
    "1": true,
    "9": 0,
    "18": 42,
    "19": 58,
    "20": 2360,
    "21": 1,
    "22": 647,
    "23": 31875,
    "24": 18486,
    "25": 1128
  }
}
*/