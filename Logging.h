#pragma once

static FILE* LOG_FILE;

#ifndef TIME
#define TIME()
#endif

#define LOC()\
    fprintf(LOG_FILE, "[%s:%d]", __FILE__, __LINE__);

#define LOG(level, ...)\
    LOC()\
    TIME()\
    fprintf(LOG_FILE, "[%s] ", level);\
    fprintf(LOG_FILE, __VA_ARGS__); \
    fprintf(LOG_FILE, "\n"); \
    fflush(LOG_FILE);

#define FATAL(...)\
    LOG("FATAL", __VA_ARGS__); \
    fclose(LOG_FILE);\
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
        FATAL(buffer); \
    }
