#include "remove_duplicates.h"
using namespace std;

void RemoveDuplicates(SearchServer& search_server) {
    set<int> id_to_delete;
    set<set<string_view>> server_words;
    for (const int document_id : search_server) {
        map<string_view, double> words_id = search_server.GetWordFrequencies(document_id);
        set<string_view> words;
        for (const auto [word, freq] : words_id) {
            words.insert(word);
        }
        if (server_words.count(words)) {            
                id_to_delete.insert(document_id);
           
        }
        else { server_words.insert(words); }
    }
    for (auto id : id_to_delete) {
        search_server.RemoveDocument(id);
        cout << "Found duplicate document id "s << id << endl;
        
    }
}