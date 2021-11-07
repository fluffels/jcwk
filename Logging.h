#pragma once

LARGE_INTEGER counterEpoch;
LARGE_INTEGER counterFrequency;
static FILE* logFile;

float GetElapsed() {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    auto result =
        (t.QuadPart - counterEpoch.QuadPart)
        / (float)counterFrequency.QuadPart;
    return result;
}

#define TIME()\
    fprintf(logFile, "[%f]", GetElapsed());

#define LOC()\
    fprintf(logFile, "[%s:%d]", __FILE__, __LINE__);

#define LOG(level, ...)\
    LOC()\
    TIME()\
    fprintf(logFile, "[%s] ", level);\
    fprintf(logFile, __VA_ARGS__); \
    fprintf(logFile, "\n"); \
    fflush(logFile);

#define FATAL(...)\
    LOG("FATAL", __VA_ARGS__); \
    fclose(logFile);\
    exit(1);

#define WARN(...) LOG("WARN", __VA_ARGS__);

#define ERR(...) LOG("ERROR", __VA_ARGS__);

#define INFO(...)\
    LOG("INFO", __VA_ARGS__)

#define CHECK(x, ...)\
    if (!x) { FATAL(__VA_ARGS__) }

#define LERROR(x) \
    if (x) {      \
        char buffer[1024]; \
        strerror_s(buffer, x); \
        FATAL("%s", buffer); \
    }

void initLogging() {
    auto error = fopen_s(&logFile, "LOG", "w");
    if (error) exit(-1);

    QueryPerformanceCounter(&counterEpoch);
    QueryPerformanceFrequency(&counterFrequency);
}
