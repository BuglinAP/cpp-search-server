#include "string_processing.h"

std::vector<std::string_view> SplitIntoWords(std::string_view text)
{
    std::vector<std::string_view> words;
    text.remove_prefix(std::min(text.find_first_not_of(" "), text.size()));
    const int64_t pos_end = text.npos;

    while (!text.empty())
    {
        int64_t space = text.find(' ');
        words.push_back(text.substr(0, space));
        if (space == pos_end)
        {
            break;
        }
        text.remove_prefix(std::min(text.find_first_not_of(" ", space), text.size()));
    }

    return words;
}