#include <algorithm>
#include <cassert>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
#include <cstdlib>
#include <iomanip>
#include <iostream>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

constexpr double MIN = 1e-6;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status,
        const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query,
        DocumentPredicate document_predicate) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < MIN) {
                    return lhs.rating > rhs.rating;
                }
                else {
                    return lhs.relevance > rhs.relevance;
                }
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        vector<Document> matched_documents;
        return matched_documents = FindTopDocuments(raw_query, [status](int document_id, DocumentStatus s, int rating) {return s == status; });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
    }

    int GetDocumentCount() const {
        return static_cast<int>(documents_.size());
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
        int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return { matched_words, documents_.at(document_id).status };
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;


    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, IsStopWord(text) };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({
                document_id,
                relevance,
                documents_.at(document_id).rating
                });
        }
        return matched_documents;
    }
};

//Операторы вывода для тестирующих функций
template <typename First, typename Second>
ostream& operator<<(ostream& out, const pair<First, Second>& p) {
    return out << p.first << ": "s << p.second;
}

template <typename Container>
void Print(ostream& out, const Container& container) {
    bool is_first = true;
    for (const auto& element : container) {
        if (!is_first) {
            out << ", "s;
        }
        is_first = false;
        out << element;
    }
}
template <typename Vector>
ostream& operator<<(ostream& out, const vector<Vector> container) {
    out << '[';
    Print(out, container);
    out << ']';
    return out;
}

template <class Sets>
ostream& operator<<(ostream& out, const set<Sets>& container) {
    out << '{';
    Print(out, container);
    out << '}';
    return out;
}

template <typename Key, typename Value>
ostream& operator<<(ostream& out, const map<Key, Value>& container) {
    out << '{';
    Print(out, container);
    out << '}';
    return out;
}

//Фреймворк
template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))


template <typename P>
void RunTestImpl(P& func, const string& name_func) {
    func();
    cerr << name_func << " is OK!"s << endl;
}

#define RUN_TEST(func)  RunTestImpl((func), #func)

// -------- Начало модульных тестов поисковой системы ----------

// Исключение стоп-слов при добавлении документов (функция из авторского решения, в задании)
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}

//Mинус-слова
void TestExcludeMinusWords() {
    SearchServer server;
    {
        server.AddDocument(0, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
        server.AddDocument(1, "cat in the small house", DocumentStatus::ACTUAL, { 1, 2, 3 });
        const auto found_docs1 = server.FindTopDocuments("cat -city"s);
        const Document& doc1 = found_docs1[0];
        ASSERT_EQUAL_HINT(doc1.id, 1, "Minus-words must be excluded from search"s);
        const auto found_docs2 = server.FindTopDocuments("cat -small"s);
        const Document& doc2 = found_docs2[0];
        ASSERT_EQUAL_HINT(doc2.id, 0, "Minus-words must be excluded from search"s);
    }
}

//Добавление документа
void TestAddingDocuments() {
    SearchServer server;
    {
        ASSERT_EQUAL(server.GetDocumentCount(), 0);
        server.AddDocument(0, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
        server.AddDocument(1, "dog in the small house", DocumentStatus::ACTUAL, { 1, 2, 3 });
        ASSERT_EQUAL(server.GetDocumentCount(), 2);
        const auto found_docs = server.FindTopDocuments("cat"s);
        const Document& doc = found_docs[0];
        ASSERT_EQUAL(doc.id, 0);
    }
}

//Совпадение документов
void TestMatchingDocuments() {
    SearchServer server;
    server.SetStopWords("in at on with the"s);
    server.AddDocument(0, "fluffy cat with blue eyes in the New York city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(1, "dog with black hair in the Green street"s, DocumentStatus::ACTUAL, { 1, 2, 3 });

    {
        const auto [matched_words, status] = server.MatchDocument("big cat with red eyes in the Moscow city"s, 0);
        const vector<string> correct_answer = { "cat"s, "city"s, "eyes"s};
        ASSERT_EQUAL(correct_answer, matched_words);
    }

    {
        const auto [matched_words, status] = server.MatchDocument("dog with -black hair"s, 1);
        const vector<string> correct_answer = {};
        ASSERT_EQUAL(correct_answer, matched_words);
        ASSERT_HINT(matched_words.empty(), "Minus words must be excluded from search"s);
    }
}

//Сортировка по релевантности
void TestSortingByRelevance() {
    int doc_id1 = 0;
    int doc_id2 = 1;
    int doc_id3 = 2;
    int doc_id4 = 3;
    SearchServer server;
    {
        server.AddDocument(doc_id1, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
        server.AddDocument(doc_id2, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
        server.AddDocument(doc_id3, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
        server.AddDocument(doc_id4, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
        const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s);
        const Document& doc0 = found_docs[0];
        const Document& doc1 = found_docs[1];
        ASSERT(doc0.relevance > doc1.relevance);
    }
}

//Вычисление рейтинга
void TestСalculationAverageRating() {
    SearchServer server;
    vector <int> ratings = { 1, 2, 3 };
    {
        server.AddDocument(0, "cat in the city"s, DocumentStatus::ACTUAL, ratings);
        const int correct_answer = (1 + 2 + 3) / 3;
        const auto found_docs = server.FindTopDocuments("cat"s);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.rating, correct_answer);
    }
}

// Сортировка с предикатом
void TestSortingByPredicate() {
    SearchServer server;
    int doc_id0 = 0;
    int doc_id1 = 1;
    int doc_id2 = 2;
    int doc_id3 = 3;
    int doc_id4 = 4;
    {
        server.AddDocument(doc_id0, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
        server.AddDocument(doc_id1, "cat in the small house", DocumentStatus::ACTUAL, { 1, 2, 3 });
        server.AddDocument(doc_id2, "parrot in the city"s, DocumentStatus::BANNED, { 1, 2, 3 });
        server.AddDocument(doc_id3, "cat in the Green street", DocumentStatus::REMOVED, { 1, 2, 3 });
        server.AddDocument(doc_id4, "cat in the park"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
        const auto found_docs = server.FindTopDocuments("cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id > 3; });
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id4);
    }
}

//Поиск по статусу
void TestSearchingByStatus() {
    SearchServer server;
    int doc_id0 = 0;
    int doc_id1 = 1;
    int doc_id2 = 2;
    int doc_id3 = 3;
    int doc_id4 = 4;
    {
        server.AddDocument(doc_id0, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
        server.AddDocument(doc_id1, "cat in the small house", DocumentStatus::ACTUAL, { 1, 2, 3 });
        server.AddDocument(doc_id2, "parrot in the city"s, DocumentStatus::BANNED, { 1, 2, 3 });
        server.AddDocument(doc_id3, "cat in the Green street", DocumentStatus::REMOVED, { 1, 2, 3 });
        server.AddDocument(doc_id4, "cat in the park"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
        const auto found_docs = server.FindTopDocuments("cat"s, DocumentStatus::REMOVED);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id3);
    }
}

//Корректное вычисление релевантности
void TestCorrectСalculationRelevance() {
    SearchServer server;
    {
        server.SetStopWords("и в нa");
        server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
        server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
        server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
        server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
        const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s);
        double a = found_docs[0].relevance - MIN;
        ASSERT_HINT((a < 0.87), "Relevance calculated incorrectly"s);
    }
    {
        server.SetStopWords("и в нa");
        server.AddDocument(0, "кот кот кот и котенок"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
        server.AddDocument(1, "собака собака собака и щенок"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
        server.AddDocument(2, "заяц зяац и зайчиха"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
        const auto found_docs = server.FindTopDocuments("кот"s);
        double relevance1 = 0.82396;
        double a = found_docs[0].relevance - MIN;
        ASSERT_HINT((a < relevance1), "Relevance calculated incorrectly"s);
    }

}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeMinusWords);
    RUN_TEST(TestAddingDocuments);
    RUN_TEST(TestMatchingDocuments);
    RUN_TEST(TestSortingByRelevance);
    RUN_TEST(TestСalculationAverageRating);
    RUN_TEST(TestSortingByPredicate);
    RUN_TEST(TestSearchingByStatus);
    RUN_TEST(TestCorrectСalculationRelevance);
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}