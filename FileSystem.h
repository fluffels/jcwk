#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#define readStruct(f, s) fread_s(&s, sizeof(s), sizeof(s), 1, f)

bool fexists(char* path);
std::vector<char> readFile(const std::filesystem::path&);
void seek(FILE*, int32_t);
