#pragma once

#include <cstdint>
#include <stddef.h>

#ifndef WIN32
#include <signal.h>
#endif

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
const int       k_kisCoreInvalidIndex = -1;
const size_t    k_kisCoreMaxPathSize = 256;

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
typedef uint8_t kisByte;

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisLog(const char* format, ...);
void kisLogError(const char* format, ...);
void kisLogTableHeader(const char* format, ...);
void kisLogTableEntry(const char* format, ...);
void kisLogTableFooter();

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
bool kisBitscanReverse(uint32_t& index, uint32_t mask);

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
#define KIS_DEBUG
#if defined(KIS_DEBUG)
    #define KIS_ASSERT(CONDITION) if((CONDITION) == false){kisLogError("Assertion Failed"); raise(SIGTRAP);}
    #define KIS_LOG_ASSERT(CONDITION, ...) if((CONDITION) == false){kisLogError(__VA_ARGS__); raise(SIGTRAP);}
    #define KIS_CHECK(CONDITION) KIS_ASSERT(CONDITION)
    #define KIS_PERF_ASSERT(CONDITION) KIS_ASSERT(CONDITION)
#else
    #define KIS_ASSERT(CONDITION)
    #define KIS_CHECK(CONDITION) CONDITION
    #define KIS_PERF_ASSERT(CONDITION)
#endif

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
#define KIS_ARRAY_COUNT(X) sizeof(X)/sizeof(X[0])
#define KIS_INVALID_INDEX -1
#define KIS_CONCAT(a, b) a ## b

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
#ifdef __GNUC__
#define KIS_INLINE __attribute__((always_inline)) inline
#else
#define KIS_INLINE inline
#endif

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
#if defined(__APPLE__)
#define KIS_INTERFACE_POSIX
#endif

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename T>
T kisMin(T a, T b)
{
    return a < b ? a : b;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename T>
T kisMax(T a, T b)
{
    return a > b ? a : b;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename T>
bool kisIsPowerOf2(T val)
{
    return (val & (val - 1)) == 0;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename T>
T kisAlignPowerOf2(T val, uint32_t align)
{
    KIS_CHECK(align > 0 && kisIsPowerOf2(align));
    return (val + (align - 1)) & (~(align - 1));
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename T>
bool kisLog2(T val_in, T& val_out)
{
    if(kisIsPowerOf2(val_in) == false)
    {
        return false;
    }

    return kisBitscanReverse(val_out, val_in);
}

