#include "FileSystem.h"

#include <io.h>

bool
fexists(char* path) {
    auto accessResult = _access_s(path, 4);
    return accessResult != ENOENT;
}

std::vector<char>
readFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        ERR("could not open %s", path.c_str());
    }
    size_t size = (size_t)file.tellg();
    std::vector<char> buffer(size);
    file.seekg(0);
    file.read(buffer.data(), size);
    file.close();
    return buffer;
}

void
seek(FILE* file, int32_t offset) {
    auto code = fseek(file, offset, SEEK_SET);
    if (code != 0) {
        throw std::runtime_error("could not seek to position");
    }
}
