#pragma once
#include <deque>
#include <string>
#include <vector>
#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        auto results = search_server_.FindTopDocuments(raw_query, document_predicate);

        if (results.empty()) {
            ++no_result_requests_;
            if (no_result_requests_ > min_in_day_) {
                --no_result_requests_;
            }
        }
        requests_.push_back({ static_cast<int>(results.size()) });
        if (requests_.size() > min_in_day_) {
            requests_.pop_front();
            --no_result_requests_;
        }
        return results;

    }
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;
private:
    struct QueryResult {
        int results_count_;
    };
    std::deque<QueryResult> requests_;
    int no_result_requests_ = 0;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_;
};