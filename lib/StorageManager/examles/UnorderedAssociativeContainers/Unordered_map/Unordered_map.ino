#include <Arduino.h>
// #define FS_t SPIFFS // LittleFS по умолчанию. Но если нужно SPIFFS то объявляемэтойстрокой
#include "StorageManager.h"
#include <unordered_map>

const char *fileName PROGMEM = "/unordered_map.bin";

void print(std::unordered_map<int, int> &data)
{
  Serial.println(F("\n____________unordered_map_____________"));
  for (const auto &pair : data)
    Serial.printf("\n %d:%d", pair.first, pair.second);
}

void setup()
{
  delay(1000);
  Serial.begin(74880);
  std::unordered_map<int, int> unordered_map;
  StorageManager storage(unordered_map, fileName); // <std::CONTAINER<settings>> можно не указывать
 print(unordered_map);
  for (int i = 0; i < 5; i++)
    unordered_map[i] = random(0,100);
  storage.write(unordered_map); // Пишем данные в файла
  storage.read(unordered_map); // Читаем данные из файла. Перед чтением unordered_map очищается.
  print(unordered_map);

  //storage.deleteFile(); //удаляет свой файл с настройками
  //storage.formatFS(); //форматирует файловую систему
  //StorageManager<int>::formatFS();//форматирует файловую систему без обращений через экземпляр

  delay(5000);
  ESP.restart();
}
void loop() { yield(); }