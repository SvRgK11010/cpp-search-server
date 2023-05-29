#include "search_server.h"
using namespace std;

SearchServer::SearchServer(const string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}

SearchServer::SearchServer(std::string_view stop_words_text) {
    vector<string_view> stops = SplitIntoWords(stop_words_text);
    for (string_view stop_w : stops) {
        stop_words_.insert(string{ stop_w });
    }
}

void SearchServer::AddDocument(int document_id, string_view document, DocumentStatus status, const vector<int>& ratings) {
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
        const vector<string_view> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (string_view word : words) {
            server_dictionary_.push_back(string{word});
            word_to_document_freqs_[server_dictionary_[server_dictionary_.size()-1]][document_id] += inv_word_count;
            word_frequency_[document_id][server_dictionary_[server_dictionary_.size() - 1]] += inv_word_count;

        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
        documents_index_.push_back(document_id);
        documents_id_.insert(document_id);
    }
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(execution::seq, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status; });
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query) const {
    return FindTopDocuments(execution::seq, raw_query, DocumentStatus::ACTUAL);
}


int SearchServer::GetDocumentCount() const {
    int x = static_cast<int>(documents_.size());
    return x;
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(string_view raw_query, int document_id) const {
    if (!documents_id_.count(document_id)) {
        throw out_of_range("Недействительный id документа"s);
    }
    const Query query = ParseQuery(raw_query, false);
    vector<string_view> matched_words;
    for (string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word)) {
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                return { matched_words, documents_.at(document_id).status };
            }
        }
    }
    for (string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word)) {
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy&, std::string_view raw_query, int document_id) const {
    return MatchDocument(raw_query, document_id);
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy&, std::string_view raw_query, int document_id) const {

    if (!documents_id_.count(document_id)) {
        throw out_of_range("Недействительный id документа"s);
    }
    const Query query = ParseQuery(raw_query, true);

    auto minus = any_of(execution::par, query.minus_words.begin(), query.minus_words.end(), [this, &document_id](string_view word)
        {if (word_to_document_freqs_.count(word)) {
        return (word_to_document_freqs_.at(word).count(document_id) != 0);
    } return false; });

    if (minus) {
        vector<string_view> matched_words = {};
        return { matched_words, documents_.at(document_id).status };
    }

    vector<string_view> matched_words(query.plus_words.size());

    auto words_end = copy_if(execution::par, query.plus_words.begin(), query.plus_words.end(),
        matched_words.begin(),
        [this, &document_id](auto word) {
            if (word_to_document_freqs_.count(word)) {
                return word_to_document_freqs_.at(word).count(document_id) != 0;
            }
    return false; });

    sort(matched_words.begin(), words_end);
    words_end = unique(matched_words.begin(), words_end);
    matched_words.erase(words_end, matched_words.end());

    return { matched_words, documents_.at(document_id).status };
}

std::set<int>::const_iterator SearchServer::begin() const {
    return documents_id_.begin();
}

std::set<int>::const_iterator SearchServer::end() const {

    return documents_id_.end();
}

const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    return word_frequency_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
    documents_.erase(document_id);
    documents_id_.erase(document_id);
    map<string_view, double> words_to_delete = GetWordFrequencies(document_id);
    for (const auto& [word, id] : words_to_delete) {
        word_to_document_freqs_.at(word).erase(document_id);
        if (word_to_document_freqs_.at(word).empty()) { word_to_document_freqs_.erase(word); }
    }
    if (word_frequency_.count(document_id)) { word_frequency_.erase(document_id); }
}

void SearchServer::RemoveDocument(std::execution::sequenced_policy exec, int document_id) {
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(execution::parallel_policy policy, int document_id) {
    if (!documents_.count(document_id)) {
        return;
    }
    if (word_frequency_.count(document_id)) {
        auto& words_to_delete = word_frequency_.at(document_id);
        vector<string_view*> words;
        words.reserve(words_to_delete.size());
        for (const auto& word : words_to_delete) {
            words.push_back(move(const_cast<string_view*>(&word.first)));
        }
        //for_each(execution::par, words_to_delete.begin(), words_to_delete.end(), [&words] (const auto& word) {words.push_back(move(const_cast<string*>(&word.first)));});
        for_each(execution::par, words.begin(), words.end(), [this, &document_id](string_view* word) {word_to_document_freqs_.at(*word).erase(document_id); });
        word_frequency_.erase(document_id);
    }
    documents_.erase(document_id);
    documents_id_.erase(find(execution::par, documents_id_.begin(), documents_id_.end(), document_id));
}



bool SearchServer::IsStopWord(string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(string_view word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(string_view text) const {
    vector<string_view> words;
    for (string_view word : SplitIntoWords(text)) {
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

SearchServer::QueryWord SearchServer::ParseQueryWord(string_view text) const {
    bool is_minus = false;
    // Word shouldn't be empty
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(string_view text, bool skip_sort) const {
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
    for (string_view word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.push_back(query_word.data);
            }
            else {
                query.plus_words.push_back(query_word.data);
            }
        }
    }
    if (!skip_sort) {
        for (auto* words : { &query.minus_words, &query.plus_words }) {
            sort(words->begin(), words->end());
            words->erase(unique(words->begin(), words->end()), words->end());
        }
    }
    return query;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}