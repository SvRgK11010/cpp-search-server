#include "string_processing.h"

using namespace std;

vector<string_view> SplitIntoWords(string_view str) {
    vector<string_view> result;
    str.remove_prefix(min(str.size(), str.find_first_not_of(" ")));
    const int64_t pos_end = str.npos;

    while (!str.empty()) {
        int64_t space = str.find(" ");
        result.push_back(space == pos_end ? str.substr(0, pos_end) : str.substr(0, space));
        str.remove_prefix(min(str.size(), str.find(" ")));
        str.remove_prefix(min(str.size(), str.find_first_not_of(" ")));
    }

    return result;
}