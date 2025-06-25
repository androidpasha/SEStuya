#include <Arduino.h>
#include "StorageManager.h"
#include <deque>
#include <vector>
#include <list>

#define CONTAINER vector
//#define CONTAINER list
//#define CONTAINER deque

const char *fileName PROGMEM = "/CONTAINER.bin";

struct settings
{
  int var1, var2;
  void print() { Serial.printf("var1:%02u\t\t var2:%02u\t\t\n", var1, var2); }
};

void print(std::CONTAINER<settings> &data){
  Serial.println(F("\n_____________CONTAINER_____________"));
  for (auto &e : data)
    e.print();
}

void setup()
{
  Serial.begin(74880UL);
  uint8_t capacity = 5;                           // Объём контейнера. Важно при первой загрузке. Дальше размер устанавливаеться из файла
  std::CONTAINER<settings> CONTAINER(capacity, {0, 0}); //Инициализация контейнера и заполнение структурами с нулевыми значениями переменных

  StorageManager<std::CONTAINER<settings>> storage(CONTAINER, fileName);
  print(CONTAINER);
  for (auto &e : CONTAINER)
    e.var1 = e.var2++;

  storage.write(CONTAINER);
  CONTAINER.clear();//Очищаем 
  storage.read(CONTAINER); // Читаем данные из файла в контейнер
  print(CONTAINER);
  
  // storage.deleteFile(); //удаляет свой файл с настройками
  // storage.formatFS(); //форматирует файловую систему
  // StorageManager<int>::formatFS();//форматирует файловую систему без обращений через экземпляр

  delay(5000UL);
  ESP.restart();
}
void loop() { yield(); }