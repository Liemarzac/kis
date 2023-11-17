#include "kisCore.h"
#include "kisFile.h"

#include <cstdio>
#include <cstring>
#include <stdarg.h>

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
extern char g_kisFileDataPath[k_kisCoreMaxPathSize];

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisCoreInit(const kisCoreInitParams& params)
{
    kisCoreStringCopy(g_kisFileDataPath, sizeof(g_kisFileDataPath), params.m_dataPath);

    kisFileOutputFilesystem();
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
