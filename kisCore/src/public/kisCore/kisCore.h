#pragma once

#include <cstdint>
#include <stddef.h>

#ifndef WIN32
#include <signal.h>
#endif

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
const int k_kisCoreInvalidIndex = -1;

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisLog(const char* format, ...);
void kisLogError(const char* format, ...);
void kisLogTableHeader(const char* format, ...);
void kisLogTableEntry(const char* format, ...);
void kisLogTableFooter();

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
#define KIS_ASSERT(CONDITION) if((CONDITION) == false){kisLogError("Assertion Failed"); raise(SIGTRAP);}
