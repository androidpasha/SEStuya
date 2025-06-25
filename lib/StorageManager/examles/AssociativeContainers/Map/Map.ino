// сохранение map со стоками не работает (строки)!!! Только с типами на стеке

#include <Arduino.h>
#include "StorageManager.h"
#include <map>

const char *fileName PROGMEM = "/map.bin";

void print(std::map<int, int> &data)
{
  Serial.println(F("\n____________map_____________"));
  for (const auto &pair : data)
    Serial.printf("\n %d:%02d", pair.first, pair.second);
}

void setup()
{
  delay(1000UL);
  Serial.begin(74880);
  std::map<int, int> map;// Создаем контейнер
  StorageManager storage(map, fileName); //Заполняем контейнер из файла (при наличии файла)
 print(map);
  for (int i = 0; i < 5; i++)//Заполняем контейнер произвольными значениями
    map[i] = static_cast<int>(random(0L,100L));
  storage.write(map); // Пишем данные в файла
  storage.read(map); // Читаем данные из файла. Перед чтением функция очищает контейнер
  print(map);

  //storage.deleteFile(); //удаляет свой файл с настройками
  //storage.formatFS(); //форматирует файловую систему
  //StorageManager<int>::formatFS();//форматирует файловую систему без обращений через экземпляр

  delay(5000UL);
  ESP.restart();
}
void loop() { yield(); }