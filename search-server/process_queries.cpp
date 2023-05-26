#include "process_queries.h"
using namespace std;

vector<vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const vector<string>& queries) {
    vector<vector<Document>> result(queries.size());
    transform(
        execution::par,
        queries.begin(), queries.end(),
        result.begin(),
        [&search_server](const string& query) {return search_server.FindTopDocuments(query); });
    return result;
}

vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    vector<vector<Document>> temp_docs = ProcessQueries(search_server, queries);
    vector<Document> result;
    for (const vector<Document>& documents : temp_docs) {
        for (const Document& document : documents) {
            result.push_back(document);
        }
    }
    return result;
}