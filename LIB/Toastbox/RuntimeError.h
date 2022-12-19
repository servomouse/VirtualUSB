#pragma once
#include <sstream>

std::runtime_error rt_error(const char * str, ...);

#define RUNTIME_ERROR(...)    rt_error(__VA_ARGS__)

