#pragma once
#include <Arduino.h>
#include <mbedtls/aes.h>
#include <WiFi.h>
#include <lwip/inet.h>
#include <time.h>
#include <ArduinoJson.h>
#define FSH (const char *)F

class Tuya
{
public:
  void set_tuya_ip(const char *devIp)
  {
    if (this->devIp != nullptr)
      delete[] this->devIp;
    char *tmp_ptr = new char[16]{};
    strncpy(tmp_ptr, devIp, 15);
    tmp_ptr[15] = '\0';
    this->devIp = tmp_ptr;
  };
  void set_devId(const char *devId) { this->devId = devId; }
  void set_LOCAL_KEY(const char *localKey) { this->localKey = localKey; }
  bool relay(bool state);
  bool on() { return relay(HIGH); }
  bool off() { return relay(LOW); }
  JsonDocument getStateJson();
  String getStateString();
  Tuya(const char *devIp, const char *devId, const char *localKey)
      : devIp(devIp), devId(devId), localKey(localKey) {}
  Tuya &operator=(const Tuya &) = delete;
  ~Tuya() { delete[] this->devIp; }
  Tuya() {}

private:
  WiFiClient client;
  bool sendCommand(uint8_t *payload, size_t size, const char *devIp, JsonDocument &doc); // передает данные
  bool aesDecrypt(const uint8_t *input_data, size_t input_data_len, char *output_data, const char *localKey);
  int aesEncrypt(const char *str, std::vector<uint8_t> &data);
  uint32_t crc32(const uint8_t *data, size_t length); // CRC32
  const char *devId;
  const char *localKey; // Ваш ключ AES (16 байтов)
  const char *devIp;
  uint32_t counter;
};

bool Tuya::relay(bool state)
{
  const uint8_t header[] PROGMEM = {
      0x00, 0x00, 0x55, 0xAA, // Фиксированный заголовок
      0x01, 0x00, 0x00, 0x00, // счетчик, Идентификатор сообщения. Такой же возвращается в ответ.
      0x00, 0x00, 0x00, 0x07, // Команда 0x00..0x0E
      0x00, 0x00, 0x00, 0x00, // Длина зашифрованных данных плюс 8 байт (CRC32 и суффикс)
      0x33, 0x2E, 0x33, 0x00, // Версия 3.3
      0x00, 0x00, 0x00, 0x00, // хз
      0x00, 0x00, 0x00, 0x00, // хз
      0x00, 0x00, 0x00,       // хз
  };
  const uint8_t tail[] PROGMEM = {0x00, 0x00, 0xAA, 0x55}; // 00 00 aa55
  std::vector<uint8_t> data;
  char *payload = nullptr;

  asprintf(&payload, FSH(R"({"devId":"%s","uid":"%s","t":"%lu","dps":{"1":%s}})"), devId, devId, time(nullptr), state == true ? "true" : "false");
  aesEncrypt(payload, data);
  free(payload);

  uint32_t total_payload_len = htonl(data.size() + 23); // Длина зашифрованных данных плюс 8 байт (CRC32 и суффикс)
  data.insert(data.begin(), header, header + sizeof(header));

  uint32_t *pComandCounter = (uint32_t *)(data.data() + 4); // Создаем ссылку на адресс масива где хранится номер команды
  *pComandCounter = htonl(counter++);

  uint32_t *pTotal_payload_len = (uint32_t *)(data.data() + 12);
  *pTotal_payload_len = total_payload_len;

  uint32_t crc = htonl(crc32(data.data(), data.size()));
  data.insert(data.end(), (uint8_t *)&crc, (uint8_t *)&crc + sizeof(crc));
  data.insert(data.end(), tail, tail + sizeof(tail));

  JsonDocument doc;
  sendCommand(data.data(), data.size(), devIp, doc); // Отправка данных

  bool result = false;
  if (doc[F("dps")]["1"].is<bool>())
    result = doc[F("dps")]["1"] == state ? true : false;
  return result;
}

JsonDocument Tuya::getStateJson() // реализовать счетчик, Идентификатор сообщения.
{
  const uint8_t header[] PROGMEM = {
      0x00, 0x00, 0x55, 0xAA, // Фиксированный заголовок
      0x00, 0x00, 0x00, 0x01, // счетчик, Идентификатор сообщения. Такой же возвращается в ответ.
      0x00, 0x00, 0x00, 0x0A, // Команда 0x00..0x0E должно 0A
      0x00, 0x00, 0x00, 0x11, // Длина зашифрованных данных плюс 8 байт (CRC32 и суффикс)
  };
  const uint8_t tail[] PROGMEM = {0x00, 0x00, 0xAA, 0x55}; // 00 00 aa55
  std::vector<uint8_t> data;
  char *payload = nullptr;
  asprintf(&payload, FSH(R"({"gwId":"%s","devId":"%s","uid":"%s","t":"%lu"})"), devId, devId, devId, time(nullptr));
  aesEncrypt(payload, data);
  free(payload);
  uint32_t total_payload_len = htonl(data.size() + sizeof(uint32_t) + sizeof(tail)); // Длина зашифрованных данных плюс 8 байт (CRC32 и суффикс)
  data.insert(data.begin(), header, header + sizeof(header));
  uint32_t *p_comandCounter = (uint32_t *)(data.data() + 4); // Создаем ссылку на адресс масива где хранится номер команды
  *p_comandCounter = htonl(counter++);
  uint32_t *pTotal_payload_len = (uint32_t *)(data.data() + 12);
  *pTotal_payload_len = total_payload_len;
  uint32_t crc = htonl(crc32(data.data(), data.size()));
  data.insert(data.end(), (uint8_t *)&crc, (uint8_t *)&crc + sizeof(crc));
  data.insert(data.end(), tail, tail + sizeof(tail));
  JsonDocument doc;
  sendCommand(data.data(), data.size(), devIp, doc); // Отправка данных
  return doc;
}

int Tuya::aesEncrypt(const char *str, std::vector<uint8_t> &data)
{
  int result = 0;
  // Копируем строку в вектор, увеличиваем размер вектора чтоб был кратный 16 и заполняем хвост data.size() % 16
  data.clear();
  data.insert(data.end(), (uint8_t *)str, (uint8_t *)(str + strlen(str)));
  uint8_t pad_len = 16 - (data.size() % 16);
  // pad_len = (pad_len == 0) ? 16 : pad_len;
  data.insert(data.end(), pad_len, pad_len);
  const size_t padded_len = data.size();
  // Инициализация AES
  mbedtls_aes_context aes_ctx;
  mbedtls_aes_init(&aes_ctx);
  mbedtls_aes_setkey_enc(&aes_ctx, (const unsigned char *)localKey, 128);
  // шифрование

  for (size_t i = 0; i < padded_len; i += 16)
    result += mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_ENCRYPT, (const unsigned char *)(data.data() + i), (unsigned char *)(data.data() + i));

  mbedtls_aes_free(&aes_ctx); // Очистка контекста
  return result;
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

bool Tuya::sendCommand(uint8_t *payload, size_t size, const char *devIp, JsonDocument &doc)
{
  if (client.connect(devIp, 6668))
  {
    Serial.printf(FSH("Соединение с Tuya успешно!\n"));
    client.write(payload, size);
    client.flush();

    // Чтение ответа
    uint32_t start = millis();
    bool result = false;
    while (millis() - start < 900) // ждём до 1 сек
    {
      u16_t len = client.available();
      if (len)
      {
        uint8_t *buf = new uint8_t[len];
        client.readBytes(buf, len);
        uint32_t *payload_size = (uint32_t *)(buf + 12);
        *payload_size = htonl(*payload_size);
        Serial.printf(FSH("\nПринят ответ размером %u байт. Payload_size = %u %s\n"), len, *payload_size, (*payload_size > 12) ? ":" : ".");
        if (*((uint32_t *)(payload + 4)) == *((uint32_t *)(buf + 4))) // сравниваем счетчики сообщений
          result = true;

        if (*payload_size > 12)
        {
          char *str = new char[*payload_size]{};
          int8_t START_PAYLOAD_BYTE = 20,
                 END_PAYLOAD_BYTE = 12;

          if (buf[20] == 0x33 and buf[21] == 0x2E and buf[22] == 0x33) // Проверка на версию 3.3
          {
            START_PAYLOAD_BYTE = 35;
            END_PAYLOAD_BYTE = 27;
          }
          aesDecrypt(buf + START_PAYLOAD_BYTE, *payload_size - END_PAYLOAD_BYTE, str, localKey);
          // Serial.printf("\nPayload: %s. Содержит символов:%u.", str, strlen(str)); //{"devId":"0805180743ddcxxxxxxx","dps":{"1":true},"t":1747670040}

          deserializeJson(doc, str);
          serializeJsonPretty(doc, Serial);
          Serial.println();
          delete[] str;
          delete[] buf;
          break;
        }
        delete[] buf;
      }
    }

    client.stop();
    Serial.printf(FSH("\nСоединение закрыто. Отправка %s\n"), result ? FSH("успешна!") : FSH("с ошибкой((("));
    return result;
  }
  else
  {
    Serial.println(FSH("Ошибка подключения к розетке!"));
    return false;
  }
}

bool Tuya::aesDecrypt(const uint8_t *input_data, size_t input_data_len, char *output_data, const char *localKey)
{
  if (input_data_len % 16 != 0)
  {
    Serial.println(FSH("Ошибка: длина входных данных должна быть кратна 16"));
    return false;
  }
  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  // Установка ключа
  if (mbedtls_aes_setkey_dec(&aes, (const unsigned char *)localKey, 128) != 0)
  {
    Serial.println(FSH("Ошибка установки ключа AES"));
    mbedtls_aes_free(&aes);
    return false;
  }
  // Расшифровка по 16 байт
  for (size_t i = 0; i < input_data_len; i += 16)
  {
    if (mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, input_data + i, (unsigned char *)(output_data + i)) != 0)
    {
      Serial.println(FSH("Ошибка расшифровки блока"));
      mbedtls_aes_free(&aes);
      return false;
    }
  }
  mbedtls_aes_free(&aes);
  uint8_t pad = output_data[input_data_len - 1];
  // Обрезаем padding в строке
  output_data[input_data_len - pad] = '\0'; // если это строка
  return true;
}

String Tuya::getStateString()
{
  String result;
  JsonDocument doc = getStateJson();
  if (doc[F("dps")].is<JsonObject>())
  {
    JsonObject dps = doc["dps"];
    char *payload = nullptr;
    asprintf(&payload, FSH("  Cтан реле: %s;\n  Напруга мережі: %.01f В;\n  Потужність:          %.01f Вт;\n  Струм:                    %u мА."),
             dps["1"] ? F("увімкнуто") : F("вимкнуто"), (int)dps["20"] / 10.0, (int)dps["19"] / 10.0, (int)dps["18"]);
    Serial.println(payload);
    result = String(payload);
    free(payload);
  }
  else
    result = "Помилка зв'язку";
  return result;
}

#undef FSH