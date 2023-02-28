#pragma once
#include "search_server.h"
#include "document.h"
#include <vector>
#include <deque>

class RequestQueue 
{
public:
    explicit RequestQueue(const SearchServer& search_server);
    
    // Search methods to save the results for statistics
    template <typename DocumentFilter>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentFilter document_filter)
    {
        const auto result = search_server_.FindTopDocuments(raw_query, document_filter);
        AddRequest(result.size());
        return result;
    }
    
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
  
    std::vector<Document> AddFindRequest(const std::string& raw_query);
   
    int GetNoResultRequests() const;
private:
    struct QueryResult 
    {
        uint64_t timestamp;
        int results;
    };
    
    std::deque<QueryResult> requests_;
    const SearchServer& search_server_;
    int no_results_requests_;
    uint64_t current_time_;
    const static int min_in_day_ = 1440;

    void AddRequest(int results_num);
};