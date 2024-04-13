#pragma once

#include "kisCoreShared.h"
#include "kisFixedStack.h"

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
#if defined(KIS_DEBUG)
#define KIS_ALLOC(size, name) kisMemAlloc(size, name, __FILE__, __LINE__)
#define KIS_ALIGNED_ALLOC(size, alignment, name) kisMemAlloc(size, name, __FILE__, __LINE__, alignment)
#define KIS_MEM_SCOPE_START_(name, VAR_UNIQUE_ID)     static uint32_t KIS_CONCAT(s_memScope_, VAR_UNIQUE_ID) = kisMemScopeFindOrCreate(#name);\
kisMemScopeStart(KIS_CONCAT(s_memScope_, VAR_UNIQUE_ID));
#define KIS_MEM_SCOPE_START(name) KIS_MEM_SCOPE_START_(name, __LINE__)
#define KIS_MEM_SCOPE_END_(name, VAR_UNIQUE_ID)       static uint32_t KIS_CONCAT(s_memScope_, VAR_UNIQUE_ID) = kisMemScopeFindOrCreate(#name);\
kisMemScopeEnd(KIS_CONCAT(s_memScope_, VAR_UNIQUE_ID));
#define KIS_MEM_SCOPE_END(name) KIS_MEM_SCOPE_END_(name, __LINE__)
#else
#define KIS_ALLOC(size, name) kisMemAlloc(size)
#define KIS_ALIGNED_ALLOC(size, alignment, name) kisMemAlloc(size, alignment)
#define KIS_MEM_SCOPE_START(name)
#define KIS_MEM_SCOPE_END(name)
#endif
#define KIS_FREE(alloc) kisMemFree(alloc)


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemInit();

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemShutdown();

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemThreadInit();

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemThreadShutdown();

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
#if defined(KIS_DEBUG)
void* kisMemAlloc(size_t size, const char* name, const char* fileName, uint32_t line, uint32_t alignment = 16);
#else
void* kisMemAlloc(size_t size, uint32_t alignment = 16);
#endif

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemFree(void* p);

#if defined(KIS_DEBUG)
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemScopeStart(uint32_t index);

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemScopeEnd(uint32_t index);

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
uint32_t kisMemScopeFindOrCreate(const char* name);
#endif

