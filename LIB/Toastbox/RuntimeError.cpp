#include "RuntimeError.h"
#include <stdarg.h>

std::runtime_error rt_error(const char * str, ...)
{
    static char msg[256];
    va_list args;
    va_start(args, str);
    vsnprintf(msg, sizeof(msg), str, args);
    return std::runtime_error(msg);
}