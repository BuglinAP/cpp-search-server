#include "search_server.h"

SearchServer::SearchServer(const std::string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {
    }

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings)
{
    IsValidDocument(document_id, document);
    const std::vector<std::string> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words)
    {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    all_documents_id_.insert(document_id);
}

// string, status
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus document_status) const
{
    return FindTopDocuments(raw_query,
        [document_status](int document_id, DocumentStatus status, int rating)
            { return status == document_status; });
}

// string
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const
{
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const
{
    IsValidQuery(raw_query);
    const Query query = ParseQuery(raw_query);
    std::vector<std::string> matched_words;
    for (const std::string& word : query.plus_words)
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
    for (const std::string& word : query.minus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id))
        {
            matched_words.clear();
            break;
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

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const
{
    return document_to_word_freqs_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id)
{
    //std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    for (auto& [key_word, document_freqs_] : word_to_document_freqs_)
    {
        document_freqs_.erase(document_id);
    }
    document_to_word_freqs_.erase(document_id);
    documents_.erase(document_id);
    all_documents_id_.erase(document_id);
}

std::set<std::string> SearchServer::GetDocumentWords(int document_id) const
{
    std::set<std::string> document_words;
    std::map<std::string, double> words_frequencies = GetWordFrequencies(document_id);
    for (auto& [word, freqs] : words_frequencies) 
    {
        document_words.insert(word);
    }
    return document_words;
}

// private methods
bool SearchServer::IsStopWord(const std::string& word) const
{
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string& word)
{
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c)
    {
        return c >= '\0' && c < ' ';
    });
}

void SearchServer::IsValidDocument(int document_id, const std::string& document) const
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

void SearchServer::IsValidQuery(const std::string& query) const
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

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const
{
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text))
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

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const
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

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const
{
    Query query;
    for (const std::string& word : SplitIntoWords(text))
    {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop)
        {
            if (query_word.is_minus)
            {
                query.minus_words.insert(query_word.data);
            }
            else
            {
                query.plus_words.insert(query_word.data);
            }
        }
    }
    return query;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const
{
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}