#include <Arduino.h>
#include "StorageManager.h"

const char *fileName PROGMEM = "/string.bin";//Путь к файлау хранения данных строки
void setup()
{
  Serial.begin(74880UL);
  String str; //Создаем строку

  StorageManager<String> storage(str, fileName); //Создаем экземпляр класа библиотеки с именем storage. В str будет записано содержание файла fileName при его наличии
  Serial.println(str);//Выведет сохраненную строку в файле после включения

  const char *names[] = {// Массив имен
      "Pavel",
      "Anastasia",
      "Anna",
      "Oleg"};

  str = names[random(0, 4)];//Заполняем строку случайным именем из массива
  storage.write(str); //Записываем обновленную строку в файл fileName
  storage.read(str); // Читаем данные из файла fileName в строку 
  Serial.println(str);// Выведет строку прочитанную из файла

  //storage.deleteFile(); //удаляет свой файл с настройками
  //storage.formatFS(); //форматирует файловую систему
  //StorageManager<int>::formatFS();//форматирует файловую систему без обращений через экземпляр класа

  delay(5000UL);
  ESP.restart();
}
void loop() {}