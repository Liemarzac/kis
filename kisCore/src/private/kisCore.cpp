#include "kisCore.h"
#include "kisFile.h"
#include "kisMem.h"

#include <cstdio>
#include <cstring>
#include <stdarg.h>

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
extern kisFilePathString g_kisFileDataPath;

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisCoreInit(const kisCoreInitParams& params)
{
    kisMemInit();
    kisMemThreadInit();

    KIS_MEM_SCOPE_BEGIN(kisMemTag::Core);

    kisFilePathString dataFilePath = kisFilePathString::format("%s" KIS_PATH_SEPARATOR "data", params.m_dataPath);
    kisFile_Init(dataFilePath.c_str());

    kisFileOutputFilesystem();
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisCoreShutdown()
{
    KIS_MEM_SCOPE_END(kisMemTag::Core);

    kisMemThreadShutdown();
    kisMemShutdown();
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisCoreStringCopy(char* dst, size_t dstSize, const char* src)
{
    KIS_ASSERT(dst != nullptr);
    KIS_ASSERT(dstSize > 0);
    KIS_ASSERT(src != nullptr);

    const size_t srcSize = strlen(src) + 1;
    const size_t copySize = kisMin(srcSize, dstSize);
    memcpy(dst, src, copySize);
    dst[dstSize - 1] = '\0';
}


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisLogLine(const char* str)
{
    printf("%s\n", str);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisLog(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    const size_t k_bufferSize = 1024;
    char buffer[k_bufferSize];
    vsnprintf(buffer, k_bufferSize, format, args);

    va_end(args);

    kisLogLine(buffer);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisLogError(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    const size_t k_bufferSize = 1024;
    char buffer[k_bufferSize];
    vsnprintf(buffer, k_bufferSize, format, args);

    va_end(args);

    kisLog("Error - %s\n", buffer);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisLogTableHeader(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    const size_t k_bufferSize = 1024;
    char buffer[k_bufferSize];
    vsnprintf(buffer, k_bufferSize, format, args);

    va_end(args);

    kisLogLine("|-------------------------------");
    kisLog("|   %s", buffer);
    kisLogLine("|-------------------------------");
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisLogTableEntry(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    const size_t k_bufferSize = 1024;
    char buffer[k_bufferSize];
    vsnprintf(buffer, k_bufferSize, format, args);

    va_end(args);

    kisLog("| %s", buffer);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisLogTableFooter()
{
    kisLogLine("|-------------------------------");
    kisLogLine("");
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
bool kisBitscanReverse(uint32_t& index, uint32_t mask)
{
    #if defined(KIS_BITSCAN_INTRISIC)
    return _BitScanReverse(&index, mask);
    #else
    uint32_t nShift = 0;
    while(mask != 0)
    {
        mask >>= 1;
        nShift++;
    }

    if(nShift > 0)
    {
        index = nShift - 1;
        return true;
    }

    return false;
    #endif
}
