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
#define KIS_DEBUG
#if defined(KIS_DEBUG)
#define KIS_ASSERT(CONDITION) if((CONDITION) == false){kisLogError("Assertion Failed"); raise(SIGTRAP);}
#define KIS_CHECK(CONDITION) KIS_ASSERT(CONDITION)
#else
#define KIS_ASSERT(CONDITION)
#define KIS_CHECK(CONDITION) CONDITION
#endif

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
#define KIS_ARRAY_COUNT(X) sizeof(X)/sizeof(X[0])

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisLog(const char* format, ...);
void kisLogError(const char* format, ...);
void kisLogTableHeader(const char* format, ...);
void kisLogTableEntry(const char* format, ...);
void kisLogTableFooter();

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
struct kisCoreInitParams
{
    const char* m_dataPath;
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisCoreInit(const kisCoreInitParams& params);

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisCoreStringCopy(char* dst, size_t dstSize, const char* src);

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
    return val + ((align - 1) & (~(align - 1)));
}
