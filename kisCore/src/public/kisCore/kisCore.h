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
T kisCoreMin(T a, T b)
{
    return a < b ? a : b;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename T>
T kisCoreMax(T a, T b)
{
    return a > b ? a : b;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisLog(const char* format, ...);
void kisLogError(const char* format, ...);
void kisLogTableHeader(const char* format, ...);
void kisLogTableEntry(const char* format, ...);
void kisLogTableFooter();

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
