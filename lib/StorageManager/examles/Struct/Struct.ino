#include <Arduino.h>
#include "StorageManager.h"

  struct settings {// Структура, свойства(переменные) которой будем хранить в файле. Метод(print) не записываеться в файл
  int var1 = 0;
  int var2 = 0;
  void print() {
    Serial.printf("\nvar1=%d, var2=%d\n", var1, var2);
  }
};

const char *fileName PROGMEM = "/struct.bin"; //Имя файла для хранения структуры
settings mySettings;//Создаем экземпляр структуры
StorageManager<settings> storage(mySettings, fileName); //Создаем экземпляр класа библиотеки с именем storage. В mySettings будет записано содержание файла fileName при его наличии


void setup() {
  Serial.begin(74880UL);
  mySettings.print();
  mySettings.var1++;
  mySettings.var2++;
  storage.write(mySettings);//Записываем обновленную структуру в файл fileName
  settings mySettings2;
  storage.read(mySettings2); //Читаем из файла данные в произвольную переменную. Тип должен быть одинаковым!!!
  Serial.print("Прочитано из файла значения:");
  mySettings.print();
 
   //storage.deleteFile(); //удаляет свой файл с настройками
   //StorageManager<int>::formatFS();//форматирует файловую систему без обращений через экземпляр
   //storage.formatFS(); //форматирует файловую систему

  delay(5000UL);
  ESP.restart();
}
void loop() {
  yield();
}
