#include <string.h>
#include <string>
#include <vector>

#include "Types.h"
#include "Memory.cpp"

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

struct String {
    umm size;
    umm length;
    char* data;
};

struct String
allocateString(struct MemoryArena* arena, umm size) {
    struct String result = {
        .size = size,
        .length = 0,
        .data = (char*)memoryArenaAllocate(arena, size)
    };

    return result;
}
