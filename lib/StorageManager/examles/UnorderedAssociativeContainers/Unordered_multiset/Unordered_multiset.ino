#include <Arduino.h>
// #define FS_t SPIFFS // LittleFS по умолчанию. Но если нужно SPIFFS то объявляемэтойстрокой
#include "StorageManager.h"
#include <unordered_set>

const char *fileName PROGMEM = "/unordered_multiset.bin";

void print(std::unordered_multiset<int> &data)
{
  Serial.println(F("\n____________UNORDERED_MULTISET_____________"));
  for (auto &e : data)
    Serial.println(e);
}

void setup()
{
  delay(1000);
  Serial.begin(74880);
  std::unordered_multiset<int> unordered_multiset;
  StorageManager storage(unordered_multiset, fileName); // <std::CONTAINER<settings>> можно не указывать
  unordered_multiset.insert(random(0, 10));
  print(unordered_multiset);
  storage.write(unordered_multiset); // Пишем данные в файла
  storage.read(unordered_multiset); // Читаем данные из файла. Перед чтением unordered_multiset очищается.
  print(unordered_multiset);

//storage.deleteFile(); //удаляет свой файл с настройками
// storage.formatFS(); //форматирует файловую систему
// StorageManager<int>::formatFS();//форматирует файловую систему без обращений через экземпляр

delay(5000);
ESP.restart();
}
void loop() { yield(); }