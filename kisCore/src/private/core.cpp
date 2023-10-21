#include "core.h"

#include <cstdio>
#include <cstring>
#include <stdarg.h>

void logLine(const char* str)
{
    printf("%s\n", str);
}

void log(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    const size_t k_bufferSize = 1024;
    char buffer[k_bufferSize];
    vsnprintf(buffer, k_bufferSize, format, args);

    va_end(args);

    logLine(buffer);
}

void logError(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    const size_t k_bufferSize = 1024;
    char buffer[k_bufferSize];
    vsnprintf(buffer, k_bufferSize, format, args);

    va_end(args);

    log("Error - %s\n", buffer);
}

void logTableHeader(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    const size_t k_bufferSize = 1024;
    char buffer[k_bufferSize];
    vsnprintf(buffer, k_bufferSize, format, args);

    va_end(args);

    logLine("|-------------------------------");
    log("|   %s", buffer);
    logLine("|-------------------------------");
}

void logTableEntry(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    const size_t k_bufferSize = 1024;
    char buffer[k_bufferSize];
    vsnprintf(buffer, k_bufferSize, format, args);

    va_end(args);

    log("| %s", buffer);
}

void logTableFooter()
{
    logLine("|-------------------------------");
    logLine("");
}
