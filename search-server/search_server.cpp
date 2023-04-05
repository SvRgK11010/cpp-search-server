#include "search_server.h"
using namespace std;

SearchServer::SearchServer(const string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}

void SearchServer::AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
    if (!IsValidWord(document)) {
        throw invalid_argument("Документ содержит недопустимые символы."s);
    }
    else if (document_id < 0) {
        throw invalid_argument("Невозможно добавить документ с отрицательным id."s);
    }
    else if (documents_.count(document_id) > 0) {
        throw invalid_argument("Документ с таким id уже был добавлен."s);
    }
    else {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
            word_frequency_[document_id][word] += inv_word_count;

        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
        documents_index_.push_back(document_id); 
        documents_id_.insert(document_id);
    }
}

vector<Document> SearchServer::FindTopDocuments(const string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status; });
}

vector<Document> SearchServer::FindTopDocuments(const string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    int x = static_cast<int>(documents_.size());
    return x;
}

tuple<vector<string>, DocumentStatus> SearchServer::MatchDocument(const string& raw_query, int document_id) const {
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

std::set<int>::const_iterator SearchServer::begin() const {
    return documents_id_.begin();
}

std::set<int>::const_iterator SearchServer::end() const {
    
    return documents_id_.end();
}

const map<string, double>& SearchServer::GetWordFrequencies(int document_id) const {
    return word_frequency_.at(document_id);
    /*
    if (word_frequency_.count(document_id) == 1) {
        return word_frequency_.at(document_id);
    }
    else {
        map<string, double> empty;
        return empty;
    }*/
}

void SearchServer::RemoveDocument (int document_id) {
    documents_.erase(document_id); 
    documents_id_.erase(document_id);
    map<string, double> words_to_delete = GetWordFrequencies(document_id);
    for (const auto& [word, id] : words_to_delete) {
            word_to_document_freqs_.at(word).erase(document_id);
        if (word_to_document_freqs_.at(word).empty()) { word_to_document_freqs_.erase(word); }
    }
    if (word_frequency_.count(document_id)) { word_frequency_.erase(document_id); }
}



bool SearchServer::IsStopWord(const string& word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const string& word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

vector<string> SearchServer::SplitIntoWordsNoStop(const string& text) const {
    vector<string> words;
    for (const string& word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(string text) const {
    bool is_minus = false;
    // Word shouldn't be empty
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(const string& text) const {
    for (size_t i = 0; i < text.size(); i++) {
        if (text[i] == '-' && text[i + 1] == '-') {
            throw invalid_argument("Добавлено два символа «минус» подряд"s);
        }
        else if ((text[i] == '-' && text[i + 1] == ' ') || (text[text.size() - 1] == '-')) {
            throw invalid_argument("Отсутствие текста после символа «минус»"s);
        }
    }
    if (!IsValidWord(text)) { throw invalid_argument("Текст запроса содержит недопустимые символы"s); }
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
double SearchServer::ComputeWordInverseDocumentFreq(const string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}