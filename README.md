# Search Server

Финальная версия учебного проекта курса "Разработчик C++" Яндекс.Практикум

### Функционал
•	Обработка результатов по статической мере TF-IDF;
•	Обработка стоп-слов;
•	Обработка минус-слов;
•	Поиск и удаление дубликатов документов;
•	Вывод результата поиска на нескольких страницах;
•	Многопоточный поиск.

### Системные требования
1.	C++17 (STL)
2.	GCC (MinGW-W64) 12.2.0

### Использование программы
Создание экземпляра поисковой системы происходит путем загрузки стоп-слов в следующих форматах:
•	Строка (string/steing_view), содержащая слова, разделеные пробелом;
•	Контейнер, содержащий слова.

Добавление документов реализуется посредством метода AddDocument. В метод должны быть переданы следующие данные: id документа, содержимое документа, статус документа, рейтинговые оценки.

Поиск документов осуществляется с помощью метода FindTopDocument. Метод возвращает вектор документов, ранжированный по релевантности запроса, также возможна сортировка по статусу, рейтингу и id.

В поисковой системе реализована функция поиска и удаления дубликатов – RemoveDuplicates, а также удаления отдельных документов RemoveDocument.

### Пример использования программы

```cpp
#include "process_queries.h"
#include "search_server.h"
#include <execution>
#include <iostream>
#include <string>
#include <vector>
using namespace std;
void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}
int main() {
    SearchServer search_server("and with"s);
    int id = 0;
    for (
        const string& text : {
            "white cat and yellow hat"s,
            "curly cat curly tail"s,
            "nasty dog with big eyes"s,
            "nasty pigeon john"s,
        }
    ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }
    cout << "ACTUAL by default:"s << endl;
    // последовательная версия
    for (const Document& document : search_server.FindTopDocuments("curly nasty cat"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    // последовательная версия
    for (const Document& document : search_server.FindTopDocuments(execution::seq, "curly nasty cat"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    // параллельная версия
    for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }
    return 0;
}
```

#### Вывод

  ```
  ACTUAL by default:
{ document_id = 2, relevance = 0.866434, rating = 1 }
{ document_id = 4, relevance = 0.231049, rating = 1 }
{ document_id = 1, relevance = 0.173287, rating = 1 }
{ document_id = 3, relevance = 0.173287, rating = 1 }
BANNED:
Even ids:
{ document_id = 2, relevance = 0.866434, rating = 1 }
{ document_id = 4, relevance = 0.231049, rating = 1 } 
  ```
