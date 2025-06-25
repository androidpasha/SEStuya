#include <Arduino.h>
// #define FS_t SPIFFS // LittleFS по умолчанию. Но если нужно SPIFFS то объявляемэтойстрокой
#include "StorageManager.h"
#include <unordered_set>

const char *fileName PROGMEM = "/unordered_set.bin";

void print(std::unordered_set<int> &data)
{
  Serial.println(F("\n____________UNORDERED_SET_____________"));
  for (auto &e : data)
    Serial.println(e);
}

void setup()
{
  Serial.begin(74880);
  std::unordered_set<int> unordered_set;
  StorageManager storage(unordered_set, fileName); // <std::CONTAINER<settings>> можно не указывать
  unordered_set.insert(random(0, 10));
  print(unordered_set);
  storage.write(unordered_set); // Пишем данные в файла
  storage.read(unordered_set); // Читаем данные из файла. Перед чтением unordered_set очищается.
  print(unordered_set);

// storage.deleteFile(); //удаляет свой файл с настройками
// storage.formatFS(); //форматирует файловую систему
// StorageManager<int>::formatFS();//форматирует файловую систему без обращений через экземпляр

delay(5000);
ESP.restart();
}
void loop() { yield(); }