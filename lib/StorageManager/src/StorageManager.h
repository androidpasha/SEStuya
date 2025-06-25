#pragma once
#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <string>
#include <vector>
#include <list>
#include <deque>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#ifndef FS_t
#define FS_t LittleFS
#endif

template <typename data_t>
class StorageManager
{
public:
  StorageManager(data_t &data, const char *path);
  bool deleteFile() const { return FS_t.remove(path); }
  static bool formatFS() { return FS_t.format(); }
  bool write(const data_t &data) const;
  bool read(data_t &data);
  size_t size() const;

private:
  const char *path;

  template <typename T>
  struct is_container {
  private:
    template <typename U>//Проверка наличия методов begin, end в типе данных
    static auto test(int) -> decltype(std::begin(std::declval<U>()), std::end(std::declval<U>()), std::true_type());

    template <typename U>
    static auto test(...) -> std::false_type;

  public:
    static constexpr bool value = decltype(test<T>(0))::value;
  };
};

//_________Реализация методов________________

  template <typename data_t>
  size_t StorageManager<data_t>::size() const{
  File file = FS_t.open(path, "r");
  if (!file) return 0;
  size_t size = file.size();
  file.close();
  return size;
  }

  template <typename data_t>
  StorageManager<data_t>::StorageManager(data_t &data, const char *path){
  FS_t.begin();
  this->path = path;
  read(data);
}

  template <typename data_t>
  bool StorageManager<data_t>::write(const data_t &data) const{
    File file = FS_t.open(path, "w");
    if (!file) return false;
    if constexpr (is_container<data_t>::value){ //Если контейнер или строка
      for (const auto &e : data)
        file.write((byte *)&e, sizeof(e) / sizeof(byte));
    } else {//Для примитивов
      file.write((byte *)&data, sizeof(data_t) / sizeof(byte));
    }
    file.close();
    return true;
  }

  template <typename data_t>
  bool StorageManager<data_t>::read(data_t &data){
      File file = FS_t.open(path, "r");
      if (!file) return false;
      if constexpr (is_container<data_t>::value){// Если контейнер
          data.clear();
          typename data_t::value_type e;
          while (file.available()){
              file.read((byte *)&e, sizeof(e));
              data.insert(data.end(), e);
          }
      }else//Примитивы
          if (sizeof(data) == file.size())
              file.read((byte *)&data, file.size());
          else{
              file.close();
              return false;
          }
      file.close();
      return true;
  }

  template <> //Спецификация String
  bool StorageManager<String>::read(String &data){
      File file = FS_t.open(path, "r");
      if (!file) return false;
        data = "";
        char e;
        while (file.available()){
          file.read((byte *)&e, sizeof(e));
          data += e;
        }
      file.close();
      return true;
  }


/*РЕАЛИЗОВАННЫЕ КОНТЕЙНЕРЫ.
Последовательные контейнеры:
                              std::vector: Динамический массив.
                              std::list: Двусвязный список.
                              std::deque: Двусторонняя очередь (двусвязный массив).

Ассоциативные контейнеры:
                              std::set: Отсортированное множество уникальных элементов.
                              std::map: Отсортированное отображение ключ-значение.
                              std::multiset: Множество, позволяющее дублирование элементов.
                              std::multimap: Отображение с возможностью дублирования ключей.

Неупорядоченные ассоциативные контейнеры:
                              std::unordered_set: Множество уникальных элементов без порядка.
                              std::unordered_map: Отображение ключ-значение без порядка.
                              std::unordered_multiset: Неупорядоченное множество с возможностью дублирования элементов.
                              std::unordered_multimap: Неупорядоченное отображение с возможностью дублирования ключей.

Адаптеры контейнеров НЕ РЕАЛИЗОВАНЫ, НЕ ТЕСТИРОВАЛИСЬ:
std::stack: Стек, базирующийся на другом контейнере.
std::queue: Очередь, базирующаяся на другом контейнере.
std::priority_queue: Очередь с приоритетом, базирующаяся на другом контейнере.
Контейнер битов:

std::bitset: Последовательность фиксированного размера битов.
*/

// Заметки:
// if constexpr (std::is_same<data_t, String>::value) //Если строка String?, проверяеться на этапе компиляции