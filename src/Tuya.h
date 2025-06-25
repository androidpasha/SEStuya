#pragma once
#include <Arduino.h>
#include <mbedtls/aes.h>
#include <WiFi.h>
#include <lwip/inet.h>
#include <time.h>
#define header_Sizes 31 // Фиксированный размер заголовка
#define TailSize 8    // Фиксированный размер хвоста (crc32 и фиксированный код 0xAA 0x55)

class Tuya
{
private:
  WiFiClient client;
  void payloadConfigure(char *input_string, const uint8_t len, const char *devId, const char *dpsValue);                                   // Создает json запрос с командой
  bool aesEncrypt(uint8_t *output_data, size_t output_data_len, const char *input_string, size_t input_string_len, const char *localKey); // Кодирует запрос
  bool aesDecrypt(const uint8_t *input_data, size_t input_data_len, char *output_data, const char *localKey);
  uint32_t crc32(const uint8_t *data, size_t length);                                                                                      // CRC32
  bool sendCommand(uint8_t *payload, size_t size, const char *devIp);                                                                    // передает данные
public:
  const char *devId;
  const char *localKey; // Ваш ключ AES (16 байтов)
  const char *devIp;
  void set_tuya_ip(const char *devIp){
    char *tmp_ptr = new  char[16]{};
    strcpy(tmp_ptr, devIp);
    this->devIp = tmp_ptr;
  };
void set_devId(const char *devId){
  this->devId = devId;
}
void set_LOCAL_KEY(const char *localKey){
  this->localKey = localKey;
}


  //Tuya(const Tuya&) = delete;
  Tuya& operator=(const Tuya&) = delete;
  ~Tuya(){delete[] this->devIp;}
  //Tuya() = delete;
  Tuya(){}
  Tuya(const char *devIp, const char *devId, const char *localKey)
      : devIp(devIp), devId(devId), localKey(localKey) {}
  bool relay(bool state);
  bool on() { return relay(HIGH); }
  bool off() { return relay(LOW); }
};

bool Tuya::relay(bool state)
{
  const size_t input_string_len = 97; // Длинна сообщения фиксирована для вкл/откл розетки
  char input_string[input_string_len] = {};
  const char *dpsValue = nullptr; // команда включения отключения в запросе (true/folse)
  state ? dpsValue = "true" : dpsValue = "false";
  payloadConfigure(input_string, input_string_len, devId, dpsValue);
  const int padded_len = ((input_string_len / 16) + 1) * 16;
  constexpr size_t output_data_len = header_Sizes + padded_len + TailSize;
  uint8_t output_data[output_data_len] = {};
  aesEncrypt(output_data, output_data_len, input_string, input_string_len, localKey);
  return sendCommand(output_data, output_data_len, devIp); // Отправка данных
}

bool Tuya::aesEncrypt(uint8_t *output_data, size_t output_data_len, const char *input_string, size_t input_string_len, const char *localKey)
{
  {
    // Заголовок
    const uint8_t header[] = {
        0x00, 0x00, 0x55, 0xAA, // Фиксированный заголовок
        0x01, 0x00, 0x00, 0x00, // счетчик, Идентификатор сообщения. Такой же возвращается в ответ.
        0x00, 0x00, 0x00, 0x07, // Команда 0x00..0x0E
        0x00, 0x00, 0x00, 0x00, // Длина зашифрованных данных плюс 8 байт (CRC32 и суффикс)
        0x33, 0x2E, 0x33, 0x00, // Версия 3.3
                                // 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 //ноли
    };
    if (output_data_len < sizeof(header))
      return false;
    memcpy(output_data, header, sizeof(header)); // Копируем заголовок в массив байтов
  }

  uint32_t *p_comandCounter = (uint32_t *)(output_data + 4); // Создаем ссылку на адресс масива где хранится номер команды
  static uint32_t counter = 0;
  *p_comandCounter = htonl(counter++);

  // на потом, команды сообщения в 11м байте
  // enum  Comands {
  //   HEART_BEAT = 0x00,    //Пинг / проверка связи
  //   PRODUCT_INFO = 0x01,  //Информация об устройстве
  //   WORK_MODE = 0x02,     //Рабочий режим
  //   WIFI_STATE = 0x03,    //Состояние Wi-Fi
  //   WIFI_RESET = 0x04,    //Сброс Wi-Fi
  //   WIFI_MODE = 0x05,     //Режим Wi-Fi
  //   DATA = 0x06,          //Передача команд управления (вкл/выкл и др)
  //   STATE = 0x07,         //Получение состояния устройства
  //   STATE_RESPONSE = 0x08,//Ответ на запрос состояния
  //   DP_QUERY = 0x09,      //Запрос значения DP (data point)
  //   DP_QUERY_RESPONSE = 0x0A,//Ответ на DP_QUERY
  //   CONTROL = 0x0B,       //Управление устройством (часто тоже 0x07)
  //   STATUS_REPORT = 0x0C, //Отчет об изменении состояния
  //   STATUS_REPORT_RESPONSE = 0x0D,//Ответ на отчет
  //   RESET_FACTORY = 0x0E  //Сброс на заводские
  // };
  // output_data[11] = DATA;

  const size_t padded_len = ((input_string_len / 16) + 1) * 16; // Делаем длину строки кратной 16
  uint8_t *input_data = new uint8_t[padded_len]{};

  if (output_data_len < input_string_len)
    return false;
  memcpy(input_data, input_string, input_string_len); // Копируем строку в массив байтов
  // Добавляем padding (PKCS7)
  const size_t block_size = 16;
  size_t pad_len = block_size - (input_string_len % block_size);
  for (size_t i = input_string_len; i < input_string_len + pad_len; i++)
  {
    input_data[i] = pad_len; // Заполнение байтами с значением pad_len
  }
  uint32_t *total_payload_len = (uint32_t *)(output_data + 12);       // Создаем ссылку на адресс масива где хранится длина сообщения
  *total_payload_len = htonl(padded_len + header_Sizes - (const int)8); // Расчет длинны сообщения, перевод байт в big-endian и сразу запись в массив

  // Инициализация AES контекста
  mbedtls_aes_context aes_ctx;
  mbedtls_aes_init(&aes_ctx);

  // Инициализация шифратора
  mbedtls_aes_setkey_enc(&aes_ctx, (const unsigned char *)localKey, 128);

  for (size_t i = 0; i < padded_len; i += 16)
  {
    mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_ENCRYPT, input_data + i, output_data + i + header_Sizes);
  }
  mbedtls_aes_free(&aes_ctx); // Очистка контекста
  delete[] input_data;

  uint32_t crc = htonl(crc32(output_data, padded_len + header_Sizes));
  if (padded_len + header_Sizes < sizeof(crc))
    return false;
  memcpy(&output_data[padded_len + header_Sizes], &crc, sizeof(crc));            // Пишем контрольную сумму в хвост
  memcpy(&output_data[padded_len + header_Sizes + TailSize - 2], "\xAA\x55", 2); // фиксированный хвост
  return true;
}

void Tuya::payloadConfigure(char *input_string, const uint8_t len, const char *devId, const char *dpsValue)
{
  struct tm timeinfo;
  getLocalTime(&timeinfo, 100); // Получаем системное время
  sprintf(input_string, "{\"devId\":\"%s\",\"uid\":\"%s\",\"t\":\"%lu\",\"dps\":{\"1\":%s}}", devId, devId, mktime(&timeinfo), dpsValue);
}

uint32_t Tuya::crc32(const uint8_t *data, size_t length)
{
  uint32_t crc = 0xFFFFFFFF;
  for (size_t i = 0; i < length; i++)
  {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++)
    {
      if (crc & 1)
        crc = (crc >> 1) ^ 0xEDB88320;
      else
        crc >>= 1;
    }
  }
  return ~crc;
}

bool Tuya::sendCommand(uint8_t *payload, size_t size, const char *devIp)
{
  if (client.connect(devIp, 6668))
  {
    delay(50);
    Serial.printf("Соединение с Tuya успешно!\n");
    client.write(payload, size);
    client.flush();

    // Чтение ответа
    delay(200);
    uint32_t start = millis();
    bool result = false;
    while (millis() - start < 700) // ждём до 1 сек
    {
      u16_t len = client.available();
      if (len)
      {
        uint8_t *buf = new uint8_t[len];
        //client.readString();
        client.readBytes(buf, len);
        uint32_t *payload_size = (uint32_t *)(buf + 12);
        *payload_size = htonl(*payload_size);
        Serial.printf("\nПринят ответ размером %u байт. Payload_size = %u :\n", len, *payload_size);
        for (size_t i = 0; i < len; i++)
          Serial.printf("%02X ", buf[i]);
        if (*((uint32_t*)(payload + 4))==*((uint32_t*)(buf + 4))){ // сравниваем счетчики сообщений
          result = true;
        }

        if(*payload_size > 12){
            char str[80]={};
            #define START_PAYLOAD_BYTE 35
            aesDecrypt(buf + START_PAYLOAD_BYTE, *payload_size-27, str, localKey);
            Serial.printf("\nPayload: %s. Содержит символов:%u.", str, strlen(str)); //{"devId":"0805180743ddcxxxxxxx","dps":{"1":true},"t":1747670040}
        }
        delete[] buf; 
      }
    }

    client.stop();
    Serial.printf("\nСоединение закрыто. Отправка %s\n", result ? "успешна!" : "с ошибкой(((");
    return result;
  }
  else
  {
    Serial.println("Ошибка подключения к розетке!");
    return false;
  }
}

bool Tuya::aesDecrypt(const uint8_t *input_data, size_t input_data_len, char *output_data, const char *localKey) {
  if (input_data_len % 16 != 0) {
    Serial.println("Ошибка: длина входных данных должна быть кратна 16");
    return false;
  }
  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  // Установка ключа
  if (mbedtls_aes_setkey_dec(&aes, (const unsigned char *)localKey, 128) != 0) {
    Serial.println("Ошибка установки ключа AES");
    mbedtls_aes_free(&aes);
    return false;
  }
  // Расшифровка по 16 байт
  for (size_t i = 0; i < input_data_len; i += 16) {
    if (mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, input_data + i, (unsigned char*)(output_data + i)) != 0) {
      Serial.println("Ошибка расшифровки блока");
      mbedtls_aes_free(&aes);
      return false;
    }
  }
  mbedtls_aes_free(&aes);
  uint8_t pad = output_data[input_data_len - 1];
  //Обрезаем padding в строке 
  output_data[input_data_len - pad] = '\0'; // если это строка
  return true;
}
