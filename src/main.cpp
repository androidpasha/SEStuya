// Код подключается к умной розетке Tuya  local по протоколу 3.3
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include "Tuya.h"
#include "VoltmeterESP32.h"
#include "webSocketFunction.h"
#include <ArduinoJson.h>
#include <algorithm>
#include "vector"
#include <log.h>

const char *ApSsid PROGMEM = "netis_0466AE_2"; // Имя вашей Wi-Fi сети
const char *ApPassword PROGMEM = "10021970";   // Пароль от вашей Wi-Fi сети
constexpr u16_t localPort PROGMEM = 8888;      // Порт для приема широковещательных пакетов
constexpr float vRef = 3.271;                  // 3.3 вольта по умолчанию. Меняется между екземплярами есп.
constexpr u16_t R1 = 100, R2 = 20;             // Резистивный делитель напряжения батареи. R1 верхний.
const char *logFile PROGMEM = "/log.txt";
const char *settingsFile PROGMEM = "/settings.bin";
const char *tuyaSwithFile PROGMEM = "/tuyaSwithState.bin";
// float LONGITUDE = 35.186532, LATITUDE = 47.794765;// GPS координаты Запорожье-1 для определение восхода/заката на местности
// byte TimeZone = 2;

LOG Log(logFile);
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
// struct tm startTime;
time_t startTime, endTime;
bool tuyaSwith;
StorageManager<bool> tuyaSwithStorage(tuyaSwith, tuyaSwithFile);

// Прототипы
bool compareVmax(float V, float Vmax, bool *tuyaState, u32_t delayMin);
bool compareVmin(float V, float Vmin, bool *tuyaState, u32_t delayMin);

void setup()
{
  Serial.begin(115200);
 // if (SaveSettingsToLFS.read(settings)) // Читаем файл настроек
  {
    // Применяем прочитанные настройки
    // if (esp_reset_reason() == ESP_RST_POWERON)
    //   tuyaSwith = settings.tuyaSwith;
    // else
    settings.tuyaSwith = tuyaSwith; // Берем значение из RTC.
    Rozetka.devIp = settings.getTuyaIp();
    Rozetka.devId = settings.tuyaDevId;
    Rozetka.localKey = settings.tuyaLocalKey;
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(settings.wifiSsid, settings.wifiPass);
    while (WiFi.status() != WL_CONNECTED && millis() < 5000)
      yield();
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.printf("Вдалє з'єднання з Wi-Fi, ip: %s\n", WiFi.localIP().toString().c_str()); // const Printable& x
    configTime(2 * 3600, 3600, "pool.ntp.org", "time.nist.gov");
  }
  else
  {
    delay(100);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ApSsid, ApPassword);
    Serial.printf("Підключення до %s не вдалося. Запущена точка доступу %s. Перейдіть на esp.local або 192.168.4.1 для налаштування.\n", settings.wifiSsid, ApSsid);
    Log.user_text("WIFI_AP start.");
  }

  Log.reset_reason();

  MDNS.begin("esp");                                             // esp.local
  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) // откр index.html при запросе корня сервера
               { request->send(LittleFS, "/settings.html", "text/html"); });

  webServer.on("/tuyaSwithUserPush", HTTP_GET, [](AsyncWebServerRequest *request)
               {    if (!request->hasParam("state")) {request->send(400, "text/plain", "Missing 'state' parameter"); return;}
    String stateStr = request->getParam("state")->value();
    bool state = (stateStr == "1" || stateStr.equalsIgnoreCase("true"));
    // Возвращаем ответ

    if (Rozetka.relay(state)) {
        tuyaSwith = state;
        tuyaSwithStorage.write(tuyaSwith);
        settings.tuyaSwith = state;
        char buf[25];
        sprintf(buf,"User push relay: %s", state ? "On" : "Off");
        Log.user_text(buf);
        request->send(200, "text/plain", state ? "1" : "0");
    } else {
        request->send(500, "text/plain", "Failed to switch Tuya");
    } });

  // http://192.168.1.252/deletelog?deletelog=true
  webServer.on("/deletelog", HTTP_GET, [](AsyncWebServerRequest *request)
               {    if (!request->hasParam("deletelog")) {request->send(400, "text/plain", "Missing 'state' parameter"); return;}
    String stateStr = request->getParam("deletelog")->value();
    bool state = (stateStr == "1" || stateStr.equalsIgnoreCase("true"));
    // Возвращаем ответ

    if (state) {
        request->send(200, "text/plain", Log.deleteLog() ? "Log file deleted." : "Failed to delete log file");
    } else {
        request->send(500, "text/plain", "Incorrect deletion log request. tap command:  /deletelog?deletelog=true");
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
  Log.wifi_disconect();
  if (WiFi.getMode() != WIFI_MODE_AP and WiFi.status() != WL_CONNECTED)
  {
    static byte attempt = 0;
    if (attempt++ >= 50) // 5 минут
    {
      Log.user_text("WiFi.status() != WL_CONNECTED -> ESP.restart()");
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
      Log.user_text("WIFI_MODE_AP > 5 min without client`s -> ESP.restart()");
      ESP.restart();
    }
  }

  static u32_t prevMillisLog = 0;
  if ((millis() - prevMillisLog) > (60UL * 60UL * 1000UL))
  {
    Log.user_text("Working... Ok.");
    prevMillisLog = millis();
  }

  static u32_t prevMillis = 0;
  if ((millis() - prevMillis) > 500) // Время обновления
  {
    battery.measure();
    JsonDocument doc;
    doc["bat4"] = battery.getVoltage();
    time(&endTime);
    time_t elapsed = endTime - startTime;
    doc["sistemTime"] = elapsed;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["FSfree"] = LittleFS.totalBytes() - LittleFS.usedBytes();
    String jsonData;
    serializeJson(doc, jsonData);
    if (webSocketVoltage.count() and webSocketVoltage.availableForWriteAll())
      webSocketVoltage.textAll(jsonData);
    if (compareVmax(battery.getVoltage(), settings.bataryVoltageMax, &tuyaSwith, settings.timer) or
        compareVmin(battery.getVoltage(), settings.bataryVoltageMin, &tuyaSwith, settings.timer))
    {
      tuyaSwithStorage.write(tuyaSwith);
      settings.tuyaSwith = tuyaSwith;
    }

    prevMillis = millis();
  }
  yield();
}

bool compareVmax(float V, float Vmax, bool *tuyaState, u32_t delayMin)
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
      Serial.print("Rozetka.on по Vmax");
      startTimer = false;
      Log.user_text("Rozetka.on  Vmax");
      return true;
    }
  }
  return false;
}

bool compareVmin(float V, float Vmin, bool *tuyaState, u32_t delayMin)
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
      Serial.print("Rozetka.off по Vmin");
      startTimer = false;
      Log.user_text("Rozetka.off Vmin");
      return true;
    }
  }
  return false;
}