#include <string.h>
#include <string>
#include <vector>

using std::string;
using std::vector;

static inline void
cStringToVecOfStrings(
    char* cstr,
    vector<string>& vec
) {
    auto delim = " ";
    char* token = strtok(cstr, delim);
    while (token != nullptr) {
        string s(token);
        vec.push_back(s);
        token = strtok(NULL, delim);
    }
}
