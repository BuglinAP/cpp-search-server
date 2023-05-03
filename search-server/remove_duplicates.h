#pragma once
#include "search_server.h"
#include <set>
#include <iostream>
#include <map>
#include <string_view>

std::set<std::string_view> GetDocumentWords(SearchServer& search_server, int document_id);

void RemoveDuplicates(SearchServer& search_server);