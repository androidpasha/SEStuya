// Код подключается к умной розетке Tuya  local по протоколу 3.3
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <ESPmDNS.h>
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

const char *ApSsid PROGMEM = "SESTuya";        // Имя точки доступа
const char *ApPassword PROGMEM = "1234567890"; // Пароль точки доступа
#define MYTZ "EET-2EEST,M3.5.0/3,M10.5.0/4"    // Киев https://remotemonitoringsystems.ca/time-zone-abbreviations.php
constexpr u16_t localPort PROGMEM = 8888;      // Порт для приема широковещательных пакетов
constexpr float vRef = 3.271;                  // 3.3 вольта по умолчанию. Меняется между екземплярами есп.
constexpr u16_t R1 = 100, R2 = 20;             // Резистивный делитель напряжения батареи. R1 верхний.
const char *logFile PROGMEM = "/log.txt";
const char *settingsFile PROGMEM = "/settings.bin";
// float LONGITUDE = 35.186532, LATITUDE = 47.794765;// GPS координаты Запорожье-1 для определение восхода/заката на местности
// byte TimeZone = 2;

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
time_t startTime, endTime;

RTC_NOINIT_ATTR bool tuyaSwith;

// Прототипы
bool compareVmax(float V, float Vmax, bool *tuyaState, u32_t delayMin);
bool compareVmin(float V, float Vmin, bool *tuyaState, u32_t delayMin);

void setup()
{
  Serial.begin(115200);
  delay(5000);
  __cplusplus; // Версия С++
  // Применяем прочитанные настройки
  if (esp_reset_reason() == ESP_RST_POWERON)
    tuyaSwith = settings.tuyaSwith;
  else
    settings.tuyaSwith = tuyaSwith; // Берем значение из RTC.

  Serial.printf("\nTuyaSwith = %s.\n", tuyaSwith ? "true" : "false");
  Rozetka.devIp = settings.getTuyaIp();
  Rozetka.devId = settings.tuyaDevId;
  Rozetka.localKey = settings.tuyaLocalKey;
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(settings.wifiSsid, settings.wifiPass);
  Serial.printf("Спроба підключення до %s, пароль %s\n", settings.wifiSsid, settings.wifiPass);
  while (WiFi.status() != WL_CONNECTED && millis() < 10000)
  {
    Serial.print(".");
    delay(100);
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.printf("Вдалє з'єднання з Wi-Fi, ip: %s\n", WiFi.localIP().toString().c_str()); // const Printable& x
    configTzTime(MYTZ, "time.google.com"), "time.windows.com", "pool.ntp.org";
    setupTelegramBot();
  }
  else
  {
    delay(100);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ApSsid, ApPassword);
    Serial.printf("Підключення до %s не вдалося. Запущена точка доступу %s, пароль %s. Перейдіть на esp.local або 192.168.4.1 для налаштування.\n", settings.wifiSsid, ApSsid, ApPassword);
    Logger.print(F("WIFI_AP start."));
  }

  Logger.reset_reason();

  MDNS.begin("esp");                                             // esp.local
  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) // откр index.html при запросе корня сервера
               { request->send(LittleFS, F("/settings.html"), F("text/html")); });

  webServer.on("/tuyaSwithUserPush", HTTP_GET, [](AsyncWebServerRequest *request)
               {    if (!request->hasParam("state")) {request->send(400, F("text/plain"), F("Missing 'state' parameter")); return;}
    String stateStr = request->getParam("state")->value();
    bool state = (stateStr == "1" || stateStr.equalsIgnoreCase(F("true")));
    // Возвращаем ответ

    if (Rozetka.relay(state)) {
        tuyaSwith = state;
        settings.tuyaSwith = state;
        Logger.printf("User push relay: %s", state ? "On" : "Off");
        request->send(200, F("text/plain"), state ? "1" : "0");
    } else {
        request->send(500, F("text/plain"), F("Failed to switch Tuya"));
    } });

  // http://192.168.1.254/deletelog?deletelog=true
  webServer.on("/deletelog", HTTP_GET, [](AsyncWebServerRequest *request)
               {    if (!request->hasParam("deletelog")) {request->send(400, F("text/plain"), F("Missing 'state' parameter")); return;}
    String stateStr = request->getParam("deletelog")->value();
    bool state = (stateStr == "1" || stateStr.equalsIgnoreCase("true"));
    // Возвращаем ответ

    if (state) {
        request->send(200, F("text/plain"), Logger.deleteLog() ? F("Log file deleted.") : F("Failed to delete log file"));
    } else {
        request->send(500, F("text/plain"), F("Incorrect deletion log request. tap command:  /deletelog?deletelog=true"));
    } });

  webServer.serveStatic("/", LittleFS, "/"); // відправляємо файли на HTTP запити з ФС
  webServer.begin();
  webServer.addHandler(&webSocket);
  webServer.addHandler(&webSocketVoltage);
  webSocket.onEvent(webSocketEvent);
  webSocketVoltage.onEvent(webSocketEventVoltage);
  // onTuyaUserEvent = TuyaSwithWeb_cb;                              // Назначаем callback при изменении слайдера в вебинтерфейсе
  while (!Rozetka.relay(tuyaSwith) and millis() < 10000) // Возвращаем розетку в сохраненное состояние
    yield();
  battery.begin(); // Включаем вольтметр
  startTime = time(&startTime);
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

  static u32_t prevMillisLog = 0;
  if ((millis() - prevMillisLog) > (30UL * 60UL * 1000UL))
  {
    prevMillisLog = millis();
    Logger.printf("Ubat = %.3f", battery.getVoltage());
  }

  static u32_t prevMillis = 0;
  if ((millis() - prevMillis) > 500) // Время обновления
  {
    prevMillis = millis();
    battery.measure();
    JsonDocument doc;
    doc["bat4"] = battery.getVoltage();
    time(&endTime);
    time_t elapsed = endTime - startTime;
    doc["sistemTime"] = elapsed;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["FSfree"] = LittleFS.totalBytes() - LittleFS.usedBytes();
    doc["tuyaSwith"] = tuyaSwith;
    String jsonData;
    serializeJson(doc, jsonData);
    if (webSocketVoltage.count() and webSocketVoltage.availableForWriteAll())
      webSocketVoltage.textAll(jsonData);
    if (compareVmax(battery.getVoltage(), settings.bataryVoltageMax, &tuyaSwith, settings.timer) or
        compareVmin(battery.getVoltage(), settings.bataryVoltageMin, &tuyaSwith, settings.timer))
    {
      settings.tuyaSwith = tuyaSwith;
    }
  }
  yield();
  loopTelegram();
}

inline bool compareVmax(float V, float Vmax, bool *tuyaState, u32_t delayMin)
{
  static u32_t prevMillis = 0;
  static bool startTimer = false;
  if (V >= Vmax and *tuyaState == LOW and startTimer == false)
  {
    prevMillis = millis();
    startTimer = true;
  }
  if (startTimer == true and ((millis() - prevMillis) > (delayMin * 60 * 1000 + 1000)))
  {
    if (Rozetka.on())
    {
      *tuyaState = HIGH;
      startTimer = false;
      Logger.printf("Rozetka.on Ubat=%.3fV => Vmax = %.3fV", battery.getVoltage(), settings.bataryVoltageMax);
      return true;
    }
  }
  return false;
}

inline bool compareVmin(float V, float Vmin, bool *tuyaState, u32_t delayMin)
{
  static u32_t prevMillis = 0;
  static bool startTimer = false;
  if (V <= Vmin and *tuyaState == HIGH and startTimer == false)
  {
    prevMillis = millis();
    startTimer = true;
  }
  if (startTimer == true and ((millis() - prevMillis) > (delayMin * 60 * 1000 + 1000)))
  {
    if (Rozetka.off())
    {
      *tuyaState = LOW;
      startTimer = false;
      Logger.printf("Rozetka.off  %.3fV <= Vmin = %.3fV.", battery.getVoltage(), settings.bataryVoltageMin);
      return true;
    }
  }
  return false;
}

// Как настроить роутер:
// Зайди в интерфейс роутера:
// 👉 Обычно это http://192.168.0.1 или http://192.168.1.1

// Найди раздел:
// Port Forwarding, Virtual Server, NAT, или Advanced > NAT Forwarding

// Добавь новое правило:

// Поле	Значение
// Имя/Описание	ESP32
// Внешний порт	12345 (любой свободный порт)
// Внутренний IP	192.168.1.123 (IP ESP32)
// Внутренний порт	80 (если ESP32 на порту 80)
// Протокол	TCP (или TCP/UDP)

// Введи данные No-IP
// Поле	Что указать
// Сервис	No-IP
// Имя хоста	partizanska.ddns.net
// Имя пользователя	Email, который ты указал в No-IP nesj5kp@ddnskey.com
// Пароль	Пароль от No-IP DiT9EztcyARr
// Интерфейс	WAN / Интернет
// Обновление	Автоматическое (роутер сам обновляет IP)

// ✅ Включи опцию «Enable DDNS» или «Обновлять автоматически»

// Пробросить порт:
// Например:

// Внешний порт: 12345

// Внутренний IP: 192.168.1.123 (ESP32)

// Внутренний порт: 80

// 🔗 Получить доступ к ESP32 извне:
// arduino
// Копировать
// Редактировать
// http://partizanska.ddns.net:12345