
#include "HashInts.h"

size_t hash_ints(const uint8_t * bytes, int len)
{
    size_t prime = 0x100000001b3;
    size_t res = (size_t)0xcbf29ce484222325;
    for (int i=0; i < len; ++i)
    {
        res ^= bytes[i];
        res *= prime;
    }
    return res;
}
