#include <windows.h>

#ifdef WIN32
    #define START_TIMER(X) \
        LARGE_INTEGER start##X = {}; \
        QueryPerformanceCounter(&start##X);

    #define END_TIMER(X) \
        LARGE_INTEGER end##X = {}; \
        QueryPerformanceCounter(&end##X); \
        float delta##X = float(end##X.QuadPart - start##X.QuadPart) / \
                         counterFrequency.QuadPart;

    #define DELTA(X) delta##X
#else
    #error Timer.h not implemented for this platform.
#endif
