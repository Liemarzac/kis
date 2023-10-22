#include "kisCore.h"

#include <cstdio>
#include <cstring>
#include <stdarg.h>

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
