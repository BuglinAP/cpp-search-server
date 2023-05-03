#pragma once
#include "string_processing.h"
#include "concurrent_map.h" 
#include "document.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <string>
#include <vector>
#include <execution>
#include <string_view>
#include <deque>
#include <thread>

constexpr int MAX_RESULT_DOCUMENT_COUNT = 5;
constexpr double COMPARISON_ACCURACY = 1e-6;
const unsigned int THREAD_COUNT = std::thread::hardware_concurrency();

class SearchServer
{
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);

    explicit SearchServer(const std::string_view stop_words_text);

    explicit SearchServer(const std::string& stop_words_text);

    void AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    // execution::sep, string, [](document_id, status, rating) { return; }
    template <typename DocumentFilter>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentFilter document_filter) const;

    // execution::sep, string, [](document_id, status, rating) { return; }
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus document_status) const;

    // execution::sep, string, status
    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;

    // execution::sep|par, string, [](document_id, status, rating) { return; }
    template <typename DocumentFilter, typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy,const std::string_view raw_query, DocumentFilter document_filter) const;

    // execution::sep|par, string, status
    template<typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query, DocumentStatus document_status) const;

    // execution::sep|par, int
    template<typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query) const;

    template<typename ExecutionPolicy>
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const ExecutionPolicy& policy,
        const std::string_view raw_query, int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view raw_query, int document_id) const;

    int GetDocumentCount() const;

    std::set<int>::iterator begin() const;

    std::set<int>::iterator end() const;

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    // execution::sep|par, int
    template<typename ExecutionPolicy>
    void RemoveDocument(ExecutionPolicy&& policy, int document_id);

    // int
    void RemoveDocument(int document_id);

private:
    struct DocumentData
    {
        int rating;
        DocumentStatus status;
        std::string document_view;
    };

    std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string_view, double>> document_to_word_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> all_documents_id_;

    bool IsStopWord(const std::string_view word) const;

    static bool IsValidWord(const std::string_view word);

    void IsValidDocument(int document_id, const std::string_view document) const;

    void IsValidId(int document_id) const;

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord
    {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string_view text) const;

    struct Query
    {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(const std::string_view text, bool flag = true) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    template <typename DocumentFilter>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentFilter document_filter) const;

    template <typename DocumentFilter, typename ExecutionPolicy>
    std::vector<Document> FindAllDocuments(const ExecutionPolicy& policy, const Query& query, DocumentFilter document_filter) const;
};


template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
    for (const std::string& stop_word : stop_words_)
    {
        if (!IsValidWord(stop_word))
        {
            using namespace std::string_literals;
            throw std::invalid_argument("Stop word contains invalid characters"s);
        }
    }
}

// execution::sep, string, [](document_id, status, rating) { return; }
template <typename DocumentFilter>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentFilter document_filter) const
{
    return FindTopDocuments(std::execution::seq, raw_query, document_filter);
}

// execution::sep|par, string, [](document_id, status, rating) { return; }
template <typename DocumentFilter, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query, DocumentFilter document_filter) const
{
    const Query query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(policy, query, document_filter);
    sort(policy, matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs)
    {
        if (std::abs(lhs.relevance - rhs.relevance) < COMPARISON_ACCURACY)
        {
            return lhs.rating > rhs.rating;
        }
        return lhs.relevance > rhs.relevance;
    });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
    {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

// execution::sep|par, string, status
template<typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query, DocumentStatus document_status) const
{
    return FindTopDocuments(policy, raw_query,
        [document_status](int document_id, DocumentStatus status, int rating)
            { return status == document_status; });
}

// execution::sep|par, string
template<typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query) const
{
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}


template<typename ExecutionPolicy>
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const ExecutionPolicy& policy,
    const std::string_view raw_query, int document_id) const
{
    if constexpr (std::is_same_v<std::decay_t<ExecutionPolicy>, std::execution::sequenced_policy>)
    {
        return MatchDocument(raw_query, document_id);
    }
    IsValidId(document_id);
    const Query query = ParseQuery(raw_query, false);
    std::vector<std::string_view> matched_words(query.plus_words.size());

    bool is_minus = std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(),
        [this, document_id](const std::string_view word)
    {
        return (word_to_document_freqs_.count(word) > 0) &&
            (word_to_document_freqs_.at(word).count(document_id) > 0);
    });
    if (is_minus)
    {
        return { std::vector<std::string_view> {}, documents_.at(document_id).status };
    }

    auto new_end = std::copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(),
        [this, document_id](const std::string_view word)
    {
        return (word_to_document_freqs_.count(word) > 0) &&
            (word_to_document_freqs_.at(word).count(document_id) > 0);;
    });

    std::sort(matched_words.begin(), new_end);
    new_end = std::unique(matched_words.begin(), new_end);
    matched_words.erase(new_end, matched_words.end());
    return { matched_words, documents_.at(document_id).status };
}

// execution::sep|par, int
template<typename ExecutionPolicy>
void SearchServer::RemoveDocument(ExecutionPolicy&& policy, int document_id)
{
    if (document_to_word_freqs_.count(document_id) == 0)
    {
        return;
    }

    size_t words_size = document_to_word_freqs_.at(document_id).size();
    std::vector<std::string_view> words_pointers(words_size);

    std::transform(
        policy,
        document_to_word_freqs_.at(document_id).begin(), document_to_word_freqs_.at(document_id).end(),
        words_pointers.begin(),
        [](auto& document_freqs)
        {
            return document_freqs.first;
        });

    std::for_each(
        policy,
        words_pointers.begin(), words_pointers.end(),
        [document_id, this](const std::string_view word)
        {
            word_to_document_freqs_.at(word).erase(document_id);
        });

    document_to_word_freqs_.erase(document_id);
    documents_.erase(document_id);
    all_documents_id_.erase(document_id);
}


template <typename DocumentFilter>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentFilter document_filter) const
{
    std::map<int, double> document_to_relevance;
    for (const std::string_view word : query.plus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word))
        {
            DocumentData document_at = documents_.at(document_id);
            if (document_filter(document_id, document_at.status, document_at.rating))
            {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const std::string_view word : query.minus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word))
        {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance)
    {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}


template <typename DocumentFilter, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindAllDocuments(const ExecutionPolicy& policy, const Query& query, DocumentFilter document_filter) const
{
    if constexpr (std::is_same_v<std::decay_t<ExecutionPolicy>, std::execution::sequenced_policy>)
    {
        return FindAllDocuments(query, document_filter);
    }
    ConcurrentMap<int, double> document_to_relevance(THREAD_COUNT);
    {
        for_each(
            std::execution::par,
            query.plus_words.begin(), query.plus_words.end(),
            [&, document_filter](const auto& word)
            {
                if (word_to_document_freqs_.count(word) > 0)
                {
                    const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                    for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word))
                    {
                        DocumentData document_at = documents_.at(document_id);
                        if (document_filter(document_id, document_at.status, document_at.rating))
                        {
                            document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                        }
                    }
                }
            });
    }
    {
        for_each(
            std::execution::par,
            query.minus_words.begin(), query.minus_words.end(),
            [&](auto word) 
            {
                if (word_to_document_freqs_.count(word) > 0) 
                {
                    for (const auto [document_id, _] : word_to_document_freqs_.at(word))
                    {
                        document_to_relevance.Erase(document_id);
                    }
                }
            });
    }
    {
        std::vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) 
        {
            matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
}