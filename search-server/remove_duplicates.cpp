#include "remove_duplicates.h"
#include <set>
#include <iostream>

void RemoveDuplicates(SearchServer& search_server)
{
    std::set<int> documents_to_delete;
    std::set<std::set<std::string_view>>documents_words;
    for (const int current_document : search_server) 
    {
        std::set<std::string_view> current_words = search_server.GetDocumentWords(current_document);
        if (documents_words.count(current_words))
        {
            using namespace std::literals;
            std::cout << "Found duplicate document id "s << current_document << std::endl;
            documents_to_delete.insert(current_document);
        }
        else
        {
            documents_words.insert(current_words);
        }
    }
    
    for (int document_to_delete : documents_to_delete)
    {
        search_server.RemoveDocument(document_to_delete);
    }
}