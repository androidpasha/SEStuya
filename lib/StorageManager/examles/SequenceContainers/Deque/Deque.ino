#include <Arduino.h>
#include "StorageManager.h"
#include <deque>

const char *fileName PROGMEM = "/deque.bin";

struct settings
{
  int var1, var2;
  void print() { Serial.printf("var1:%02u\t\t var2:%02u\t\t\n", var1, var2); }
};

void print(std::deque<settings> &data){
  Serial.println(F("\n_____________DEQUE_____________"));
  for (auto &e : data)
    e.print();
}

void setup()
{
  Serial.begin(74880);
  uint8_t capacity = 5;                           // Объём контейнера. Важно при первой загрузке. Дальше размер устанавливаеться из файла
  std::deque<settings> deque(capacity, {0, 0});
  StorageManager<std::deque<settings>> storage(deque, fileName); // <std::deque<settings>> можно не указывать
  print(deque);
  for (auto &e : deque)
    e.var1 = e.var2++;
  storage.write(deque);
  deque.clear();//Очищаем 
  storage.read(deque); // Читаем данные из файла
  print(deque);
  
  // storage.deleteFile(); //удаляет свой файл с настройками
  // storage.formatFS(); //форматирует файловую систему
  // StorageManager<int>::formatFS();//форматирует файловую систему без обращений через экземпляр

  delay(5000);
  ESP.restart();
}
void loop() { yield(); }