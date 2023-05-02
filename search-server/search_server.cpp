#include "search_server.h"

SearchServer::SearchServer(const std::string_view stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWords(std::string_view(stop_words_text)))  // Invoke delegating constructor from string container
{
}

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings)
{
    IsValidDocument(document_id, document);
    documents_view_add_.push_back(std::string(document));
    const std::vector<std::string_view> words = SplitIntoWordsNoStop(std::string_view(documents_view_add_.back()));
    const double inv_word_count = 1.0 / words.size();
    for (const std::string_view word : words)
    {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    all_documents_id_.insert(document_id);
}

// execution::sep, string, status
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus document_status) const
{
    return FindTopDocuments(raw_query,
        [document_status](int document_id, DocumentStatus status, int rating)
            { return status == document_status; });
}

// execution::sep, string
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const
{
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view raw_query, int document_id) const
{
    IsValidQuery(raw_query);
    IsValidId(document_id);
    const Query query = ParseQuery(raw_query);
    std::vector<std::string_view> matched_words;

    for (const std::string_view word : query.minus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id))
        {
            return { matched_words, documents_.at(document_id).status };
        }
    }
    for (const std::string_view word : query.plus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id))
        {
            matched_words.push_back(word);
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

int SearchServer::GetDocumentCount() const
{
    return documents_.size();
}

std::set<int>::iterator SearchServer::begin() const
{
    return all_documents_id_.begin();
}

std::set<int>::iterator SearchServer::end() const
{
    return all_documents_id_.end();
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const
{
    return document_to_word_freqs_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id)
{
    RemoveDocument(std::execution::seq, document_id);
}

std::set<std::string_view> SearchServer::GetDocumentWords(int document_id) const
{
    std::set<std::string_view> document_words;
    std::map<std::string_view, double> words_frequencies = GetWordFrequencies(document_id);
    for (auto& [word, freqs] : words_frequencies)
    {
        document_words.insert(word);
    }
    return document_words;
}

// private methods
bool SearchServer::IsStopWord(const std::string_view word) const
{
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string_view word)
{
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c)
    {
        return c >= '\0' && c < ' ';
    });
}

void SearchServer::IsValidDocument(int document_id, const std::string_view document) const
{
    using namespace std;
    if (!IsValidWord(document))
    {
        throw invalid_argument("Document contains invalid characters"s);
    }
    if (document_id < 0)
    {
        throw invalid_argument("Attempt to add a document with a negative id"s);
    }
    if (documents_.count(document_id))
    {
        throw invalid_argument("Attempt to add a document with the id of a previously added document"s);
    }
}

void SearchServer::IsValidQuery(const std::string_view query) const
{
    using namespace std;
    if (!IsValidWord(query))
    {
        throw invalid_argument("Query contains invalid characters"s);
    }
    if (query[query.size() - 1] == '-')
    {
        throw invalid_argument("No text after the minus sign"s);
    }
    for (int i = 0; i < query.size(); ++i)
    {
        if (query[i] == '-')
        {
            if (query[i + 1] == '-')
            {
                throw invalid_argument("More than one minus sign before the words"s);
            }
            if (query[i + 1] == ' ')
            {
                throw invalid_argument("No text after the minus sign"s);
            }
        }
    }
}

void SearchServer::IsValidId(int document_id) const
{
    using namespace std;
    if (!documents_.count(document_id))
    {
        throw out_of_range("Non-existent document id"s);
    }
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const
{
    std::vector<std::string_view> words;
    for (const std::string_view word : SplitIntoWords(text))
    {
        if (!IsStopWord(word))
        {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings)
{
    if (ratings.empty())
    {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const
{
    bool is_minus = false;
    // Word shouldn't be empty
    if (text[0] == '-')
    {
        is_minus = true;
        text = text.substr(1);
    }
    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text) const
{
    return ParseQuery(text, true);
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text, bool sequenced_flag) const
{
    Query query;

    for (std::string_view word : SplitIntoWords(text))
    {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop)
        {
            if (query_word.is_minus)
            {
                query.minus_words.push_back(query_word.data);
            }
            else
            {
                query.plus_words.push_back(query_word.data);
            }
        }
    }

    if (sequenced_flag)
    {
        std::sort(query.plus_words.begin(), query.plus_words.end());
        auto new_end = unique(query.plus_words.begin(), query.plus_words.end());
        query.plus_words.erase(new_end, query.plus_words.end());
    }
    return query;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const
{
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}