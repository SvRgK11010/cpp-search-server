#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
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
        } else {
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
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        double tf_word = 1.0/words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] +=tf_word;
        }
        ++document_count_;
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:    
    map<string, map<int, double>> word_to_document_freqs_;

    set<string> stop_words_;
    
    int document_count_ = 0;
    
    struct Query {
        set<string> minus_w;
        set<string> plus_w;
    };
    
    Query ParseQuery(const string& text) const {
        Query query_words;
        for (string& word : SplitIntoWords(text)) {
                if (word[0] =='-') {
                word=word.substr(1);
                query_words.minus_w.insert(word);
            } else {query_words.plus_w.insert(word);}
        }
        return query_words;
    }

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

    vector<Document> FindAllDocuments(const Query& query_words) const {
        vector<Document> matched_documents;
        map<int, double> document_to_relevance;
        for (const auto& plus_word : query_words.plus_w) {
            if (word_to_document_freqs_.count(plus_word)>=1) {
                double doc_with_plus = word_to_document_freqs_.at(plus_word).size();
                double idf_plus_word = log(document_count_/doc_with_plus); //подсчет idf
                for (const auto& [id, freqs] : word_to_document_freqs_.at(plus_word)) {
                    document_to_relevance[id] += freqs*idf_plus_word;
                    }
            }
            }
        
        for (const auto& minus_word : query_words.minus_w) {
            if (word_to_document_freqs_.count(minus_word)) {
                for (const auto& [id, freqs]: word_to_document_freqs_.at(minus_word)) {
                    document_to_relevance.erase(id);
                }
            }
        }
        
        for (const auto& [id, relevance] : document_to_relevance) {
            matched_documents.push_back({id, relevance});
        }
        return matched_documents;
    }

    /*static int MatchDocument(const DocumentContent& content, const Query& query_words) {
        if (query_words.plus_w.empty()) {
            return 0;
        }
        set<string> matched_words;
        for (const string& word : content.words) {
            if (query_words.minus_w.count(word) == 1) {
                return 0;}
            if (matched_words.count(word) != 0) {
                continue;
            }
            if (query_words.plus_w.count(word) != 0) {
                matched_words.insert(word);
            }
        }
        return static_cast<int>(matched_words.size());
    }*/
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }
}