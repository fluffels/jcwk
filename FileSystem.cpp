#include "FileSystem.h"

#include <io.h>

bool
fexists(char* path) {
    auto accessResult = _access_s(path, 4);
    return accessResult != ENOENT;
}

FILE*
openFile(const char* path, const char* mode) {
    FILE* result;
    errno_t errorCode;
    #if WIN32
    errorCode = fopen_s(&result, path, mode);
    LERROR(errorCode != 0);
    #else
    result = fopen(path, mode);
    LERROR(result != NULL);
    #endif
    return result;
}

std::vector<char>
readFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        #if WIN32
        ERR("could not open %ls", path.c_str());
        #else
        ERR("could not open %s", path.c_str());
        #endif
    }
    size_t size = (size_t)file.tellg();
    std::vector<char> buffer(size);
    file.seekg(0);
    file.read(buffer.data(), size);
    file.close();
    return buffer;
}

size_t
readFromFile(FILE* file, size_t bufferSize, void* buffer) {
    size_t readCount;
    #if WIN32
    readCount = fread_s(buffer, bufferSize, 1, bufferSize, file);
    #else
    readCount = fread(buffer, 1, bufferSize, file);
    #endif
    if (readCount < bufferSize) {
        LERROR(ferror(file));
    }
    return readCount;
}

void
seek(FILE* file, int32_t offset) {
    auto code = fseek(file, offset, SEEK_SET);
    if (code != 0) {
        throw std::runtime_error("could not seek to position");
    }
}
