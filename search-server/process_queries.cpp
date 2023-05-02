#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries)
{
    std::vector<std::vector<Document>> process_queries(queries.size());
    std::transform
    (
        std::execution::par, 
        queries.begin(), queries.end(), 
        process_queries.begin(), 
        [&search_server](const std::string& querie) 
        { return search_server.FindTopDocuments(querie); }
    );
    return process_queries;
}

std::vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries) 
{
    std::vector<Document> queries_joined;
    for (const auto& process_querie : ProcessQueries(search_server, queries)) {
        std::copy(process_querie.begin(), process_querie.end(), std::back_inserter(queries_joined));
    }
    return queries_joined;
}