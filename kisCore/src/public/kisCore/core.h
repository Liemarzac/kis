#pragma once

#include <cstdint>
#include <stddef.h>

#ifndef WIN32
#include <signal.h>
#endif

void log(const char* format, ...);
void logError(const char* format, ...);
void logTableHeader(const char* format, ...);
void logTableEntry(const char* format, ...);
void logTableFooter();

#define ASSERT(CONDITION) if((CONDITION) == false){logError("Assertion Failed"); raise(SIGTRAP);}
