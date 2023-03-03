#include "remove_duplicates.h"
#include <set>
#include <iostream>

void RemoveDuplicates(SearchServer& search_server)
{
    std::set<int> documents_to_delete;
    for (const int current_document : search_server) 
    {
        //slow implementation, but I didn't come up with better one :(
        std::set<std::string> current_words = search_server.GetDocumentWords(current_document);
        for (const int next_document : search_server)
        {
            std::set<std::string> next_words = search_server.GetDocumentWords(next_document);
            bool to_delete = ((current_document < next_document) 
                && (!documents_to_delete.count(next_document))
                    && (current_words == next_words));
            if (to_delete)
            {
                using namespace std::literals;
                std::cout << "Found duplicate document id "s << next_document << std::endl;
                documents_to_delete.insert(next_document);
            }
        }
    }

    
    for (int document_to_delete : documents_to_delete)
    {
        search_server.RemoveDocument(document_to_delete);
    }
}