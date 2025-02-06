#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

extern "C"
{
    EMSCRIPTEN_KEEPALIVE int add(const int a, const int b)
    {
        return a + b;
    }
    EMSCRIPTEN_KEEPALIVE int subtract(const int a, const int b)
    {
        return a - b;
    }
}
