#pragma once

#include <cstdio>

#include <Windows.h>

#include "Types.h"

// ******************
// * Counter stuff. *
// ******************

LARGE_INTEGER counterEpoch;
LARGE_INTEGER counterFrequency;
static FILE* logFile;

float getElapsed() {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    auto result =
        (t.QuadPart - counterEpoch.QuadPart)
        / (float)counterFrequency.QuadPart;
    return result;
}

// ******************
// * Console stuff. *
// ******************

struct ConsoleLine {
    // NOTE(jan): Relative to Console::bytesRead
    umm start;
    umm size;
};

#define MAX_SCROLLBACK_LINES 1024
struct LineBuffer {
    ConsoleLine data[MAX_SCROLLBACK_LINES];
    umm first;
    umm next;
    umm count;
    umm max;
    umm viewOffset;
};

struct Console {
    void* data;
    umm bytesRead;
    umm top;
    umm bottom;
    umm size;
    LineBuffer lines;
    bool show;
};

Console console;

#ifndef isPowerOfTwo
#define isPowerOfTwo(x) (((x) & ((x) - 1)) == 0)
#endif

// Derived from https://github.com/cmuratori/refterm/blob/main/refterm_example_source_buffer.c
Console initConsole(size_t bufferSize) {
    Console result = {};

    SYSTEM_INFO info;
    GetSystemInfo(&info);
    if (!isPowerOfTwo(info.dwAllocationGranularity)) {
        fprintf(stderr, "system allocation size is not a power of two");
        exit(1);
    }

    bufferSize = ((bufferSize / info.dwAllocationGranularity) + 1) * info.dwAllocationGranularity;
    if (bufferSize % info.dwAllocationGranularity != 0) {
        fprintf(stderr, "invalid buffer size");
        exit(2);
    }

    HANDLE section = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        (DWORD)(bufferSize >> 32),
        (DWORD)(bufferSize & 0xffffffff),
        NULL
    );

    void* ringBuffer = nullptr;
    // NOTE(jan): Try to find two consecutive memory areas to map the section twice back-to-back.
    for(size_t offset =   0x40000000;
               offset <  0x400000000;
               offset +=   0x1000000) {
        void *view1 = MapViewOfFileEx(section, FILE_MAP_ALL_ACCESS, 0, 0, bufferSize, (void*)offset);
        void *view2 = MapViewOfFileEx(section, FILE_MAP_ALL_ACCESS, 0, 0, bufferSize, (void*)(((u8*)offset) + bufferSize));

        if(view1 && view2) {
            ringBuffer = view1;
            break;
        }

        if(view1) UnmapViewOfFile(view1);
        if(view2) UnmapViewOfFile(view2);
    }

    if (!ringBuffer) {
        fprintf(stderr, "could not allocate ringbuffer");
        exit(3);
    }

    result.data = ringBuffer;
    result.size = bufferSize;
    result.top = 0;
    result.bottom = 0;
    result.lines.max = MAX_SCROLLBACK_LINES;
    return result;
}

void
consolePushLine(Console& console, ConsoleLine& line) {
    LineBuffer& lines = console.lines;
    lines.data[lines.next] = line;
    lines.next = (lines.next + 1) % lines.max;
    lines.first = lines.count < lines.max ? 0 : lines.next + 1;
    lines.count = min(lines.max, lines.count + 1);
}

// ******************
// * Logging stuff. *
// ******************

void logRaw(const char* s) {
    char* consoleEnd = (char*)console.data + console.bottom;
    fprintf(logFile, "%s", s);
    int written = sprintf(consoleEnd, "%s", s);

    ConsoleLine line;
    line.start = console.bottom;
    line.size = written;
    consolePushLine(console, line);

    console.bytesRead += written;
    console.bottom = (console.bottom + written) % console.size;
}

void log(const char* level, const char* fileName, int lineNumber, const char* fmt, ...) {
    char* consoleEnd = (char*)console.data + console.bottom;

    ConsoleLine line;
    line.start = console.bottom;

    const char* prefixFmt = "[%s] [%f] [%s:%d] ";

    fprintf(logFile, prefixFmt, level, getElapsed(), fileName, lineNumber);
    int written = sprintf(consoleEnd, prefixFmt, level, getElapsed(), fileName, lineNumber);
    line.size = written;
    console.bytesRead += written;
    console.bottom = (console.bottom + written) % console.size;
    consoleEnd = (char*)console.data + console.bottom;

    va_list args;
    va_start(args, fmt);
    vfprintf(logFile, fmt, args);
    written = vsnprintf(consoleEnd, console.size, fmt, args);
    line.size += written;
    console.bytesRead += written;
    console.bottom = (console.bottom + written) % console.size;
    consoleEnd = (char*)console.data + console.bottom;
    va_end(args);

    fprintf(logFile, "\n");
    written = sprintf(consoleEnd, "\n");
    line.size += written;
    console.bytesRead += written;
    console.bottom = (console.bottom + written) % console.size;
    fflush(logFile);

    consolePushLine(console, line);
}

#define LOG(level, fmt, ...) log(level, __FILE__, __LINE__, fmt, __VA_ARGS__)
#define FATAL(fmt, ...) {\
    LOG("FATAL", fmt, __VA_ARGS__);\
    fclose(logFile);\
    exit(4);\
}
#define INFO(fmt, ...) LOG("INFO", fmt, __VA_ARGS__)
#define WARN(fmt, ...) LOG("WARN", fmt, __VA_ARGS__)
#define ERR(fmt, ...) LOG("ERR", fmt, __VA_ARGS__)

#define CHECK(x, fmt, ...) if (!x) { FATAL(fmt, __VA_ARGS__) }
#define LERROR(x) \
    if (x) {      \
        char buffer[1024]; \
        strerror_s(buffer, x); \
        FATAL("%s", buffer); \
    }
