#include "remove_duplicates.h"
using namespace std;

void RemoveDuplicates(SearchServer& search_server) {
    set<int> id_to_delete;
    map<set<string>, int> server_words;
    for (const int document_id : search_server) {
        map<string, double> words_id = search_server.GetWordFrequencies(document_id);
        set<string> words;
        for (const auto [word, freq] : words_id) {
            words.insert(word);
        }
        if (server_words.count(words)) {
            if (server_words.at(words) < document_id) {
                id_to_delete.insert(document_id);
            } 
            else {
                id_to_delete.insert(server_words.at(words));
                server_words.erase(words);
                server_words[words] = document_id; }
        }
        else { server_words[words] = document_id; }
    }
    for (auto id : id_to_delete) {
        search_server.RemoveDocument(id);
        cout << "Found duplicate document id "s << id << endl;
        
    }
}