#include <Arduino.h>
// #define FS_t SPIFFS // LittleFS по умолчанию. Но если нужно SPIFFS то объявляемэтойстрокой
#include "StorageManager.h"
#include <vector>

const char *fileName PROGMEM = "/vector.bin";

struct settings
{
  int var1, var2;
  char name[10];
  void print() { Serial.printf("var1:%02u\t\t var2:%02u\t\t Name:%s\n", var1, var2, name); }
};

void printVector(std::vector<settings> &vector)
{
  Serial.println(F("\n_____________VECTOR_____________"));
  for (auto &e : vector)
    e.print();
}

void setup()
{
  Serial.begin(74880);
  uint8_t capacity = 5;                   // Объём вектора. Важно при первой загрузке. Дальше размер устанавливаеться из файла
  std::vector<settings> vector(capacity); // vector.resize(capacity);

  StorageManager<std::vector<settings>> storageVector(vector, fileName); // <std::vector<settings>> можно не указывать
  printVector(vector);
  const char *names[] = {
    "Pavel",
    "Anastasia",
    "Anna",
    "Oleg"
  };

  for (auto &e : vector){
    e.var1 = e.var2++;
    strcpy(e.name, names[random(0,4)]);
  }

  storageVector.write(vector);

  vector.clear();             // Очищаем вектор
  storageVector.read(vector); // Читаем данные из файла в вектор
  printVector(vector);

  //storageVector.deleteFile(); //удаляет свой файл с настройками
  // storageVector.formatFS(); //форматирует файловую систему
  // StorageManager<int>::formatFS();//форматирует файловую систему без обращений через экземпляр

  delay(5000);
  ESP.restart();
}
void loop() { yield(); }