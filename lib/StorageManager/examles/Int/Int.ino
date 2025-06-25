#include <Arduino.h>
#include "StorageManager.h"

const char *fileName PROGMEM = "/int.bin";
int a = 0;//Переменная, которую будем хранить в файле fileName
StorageManager<int> storage(a, fileName);// Тут значение a читается из фс если было ранее сохранено

void setup()
{
  Serial.begin(74880UL);
  Serial.printf("\n\nЗагрузка№ %03d\n", a); //Выводим прочитанную переменную
  a++;
  storage.write(a);//Сохраняем в файл измененную переменную
  int b;//Другая переменная
  storage.read(b); //Читаем из файла данные в произвольную переменную. Тип должен быть одинаковым!!!
  Serial.printf("Прочитано из файла значение %03d\n\n", b);
 
   //storage.deleteFile(); //удаляет свой файл с настройками
   //StorageManager<int>::formatFS();//форматирует файловую систему без обращений через экземпляр
   //storage.formatFS(); //форматирует файловую систему
  
  delay(5000UL);
  ESP.restart();
}
void loop() { yield(); }