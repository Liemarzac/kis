#include "kisHash.h"

#include <cstring>

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
uint32_t kisHashStringAsUint32(const char* str)
{
    const uint64_t hash64 = kisHashStringAsUInt64(str);
    const uint32_t hash32 = ((uint32_t)(hash64 & 0xFFFFFFFF)) ^ ((uint32_t)((hash64 >> 32) & 0xFFFFFFFF));
    return hash32;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
uint64_t kisHashStringAsUInt64(const char* str)
{
    const int p = 31;
    const int m = 1e9 + 9;
    uint64_t hash = 0;
    uint64_t p_pow = 1;
    size_t len = strlen(str);
    for (int i = 0; i < len; i++)
    {
        hash = (hash + (str[i] - 'a' + 1) * p_pow) % m;
        p_pow = (p_pow * p) % m;
    }
    return hash;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
uint32_t kisHashPointerAsUInt32(const void* p)
{
    const uint64_t hash64 = ((uint64_t)p) >> 4; // Assume it is 16 bytes aligned.
    const uint32_t hash32 = ((uint32_t)(hash64 & 0xFFFFFFFF)) ^ ((uint32_t)((hash64 >> 32) & 0xFFFFFFFF));
    return hash32;
}

