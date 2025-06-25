#include <Arduino.h>
#include "StorageManager.h"
#include <list>

const char *fileName PROGMEM = "/list.bin";

struct settings
{
  int var1, var2;
  void print() { Serial.printf("var1:%02u\t\t var2:%02u\t\t\n", var1, var2); }
};

void printList(std::list<settings> &list)
{
  Serial.println(F("\n______________LIST______________"));
  for (auto &e : list)
    e.print();
}

void setup()
{
  Serial.begin(74880UL);
  uint8_t capacity = 5;                                            // Объём списка. Важно при первой загрузке. Дальше размер устанавливаеться из файла
  std::list<settings> list(capacity, {0, 0}); 
  StorageManager<std::list<settings>> storage(list, fileName); // <std::list<settings>> можно не указывать
  printList(list);//Выводим данные прочитанные из файла

  for (auto &e : list)
    e.var1 = e.var2++;

  storage.write(list);
  list.clear(); // Очищаем
  list.resize(0);
  storage.read(list); // Читаем данные из файла
  printList(list);
  //storage.deleteFile(); //удаляет свой файл с настройками
  // storage.formatFS(); //форматирует файловую систему
  // StorageManager<int>::formatFS();//форматирует файловую систему без обращений через экземпляр

  delay(5000UL);
  ESP.restart();
}
void loop() { yield(); }