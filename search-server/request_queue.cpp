#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server)
        : search_server_(search_server)
        , no_results_requests_(0)
        , current_time_(0) 
    {
    }

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) 
{
    return AddFindRequest(raw_query,
        [status](int document_id, DocumentStatus document_status, int rating)
            {return document_status == status; });
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) 
{
    return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
}

int RequestQueue::GetNoResultRequests() const 
{
    return no_results_requests_;
}

void RequestQueue::AddRequest(int results_num) 
    {
        // New request - new minute
        ++current_time_;
        // Deleting all outtimed search results
        while (!requests_.empty() && (min_in_day_ <= current_time_ - requests_.front().timestamp))
        {
            if (requests_.front().results == 0) 
            {
                --no_results_requests_;
            }
            requests_.pop_front();
        }
        // Saving new search result
        requests_.push_back({ current_time_, results_num });
        if (results_num == 0) 
        {
            ++no_results_requests_;
        }
    }