#include <Arduino.h>
// #define FS_t SPIFFS // LittleFS по умолчанию. Но если нужно SPIFFS то объявляемэтойстрокой
#include "StorageManager.h"
#include <unordered_map>

const char *fileName PROGMEM = "/unordered_multimap.bin";

void print(std::unordered_multimap<const char*, int> &data)
{
  Serial.println(F("\n____________UNORDERED_MULTIMAP_____________"));
  for (const auto &pair : data)
    Serial.printf("\n %s:%d", pair.first, pair.second);
}

void setup()
{
  //StorageManager<int>::formatFS();
  delay(1000);
  Serial.begin(74880);
  std::unordered_multimap<const char*, int> unordered_multimap;
  StorageManager storage(unordered_multimap, fileName); // <std::CONTAINER<settings>> можно не указывать
  print(unordered_multimap);
    const char *names[] = {
    "Pavel",
    "Anastasia",
    "Anna",
    "Oleg"
  };
  auto pair = std::make_pair(names[random(0,4)], (int)random(0, 10));
  unordered_multimap.insert(pair);

  storage.write(unordered_multimap); // Пишем данные в файла
  storage.read(unordered_multimap);  // Читаем данные из файла. Перед чтением unordered_multimap очищается.
  print(unordered_multimap);

  //storage.deleteFile(); //удаляет свой файл с настройками
  //storage.formatFS(); //форматирует файловую систему
  //StorageManager<int>::formatFS();//форматирует файловую систему без обращений через экземпляр

  delay(5000);
  ESP.restart();
}
void loop() { yield(); }