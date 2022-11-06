#include "RuntimeError.h"

std::string Toastbox::_RuntimeErrorFmtMsg(const char* str)
{
    char msg[256];
    int sr = snprintf(msg, sizeof(msg), "%s", str);
    if (sr<0 || (size_t)sr>=(sizeof(msg)-1)) throw std::runtime_error("failed to create RuntimeError");
    return msg;
}

