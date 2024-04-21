#pragma once

#include "kisCoreShared.h"
#include "kisFixedStack.h"

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
#if defined(KIS_DEBUG)
#define KIS_ALLOC(size, tag, name) kisMemAlloc(size, tag, name, __FILE__, __LINE__)
#define KIS_ALIGNED_ALLOC(size, alignment, tag, name) kisMemAlloc(size, tag, name, __FILE__, __LINE__, alignment)
#define KIS_REALLOC(p, size) kisMemRealloc(p, size, __FILE__, __LINE__)
#define KIS_NEW(tag, name) new(tag, name, __FILE__, __LINE__)
#define KIS_ALIGNED_NEW(tag, name, alignment) new(tag, name, __FILE__, __LINE__, alignment)
#define KIS_MEM_SCOPE_BEGIN(tag) kisMemScopeBegin(tag);
#define KIS_MEM_SCOPE_END(tag) kisMemScopeEnd(tag);
#define KIS_MEM_REGISTER_EXTERNAL(pointer, tag, name) kisMemRegisterEntry((void*)pointer, tag, name, __FILE__, __LINE__);
#define KIS_MEM_UNREGISTER_EXTERNAL(pointer) kisMemUnregisterEntry((void*)pointer);
#else
#define KIS_ALLOC(size, name) kisMemAlloc(size)
#define KIS_ALIGNED_ALLOC(size, alignment, name) kisMemAlloc(size, alignment)
#define KIS_REALLOC(p, size) kisMemRealloc(p, size)
#define KIS_NEW(tag, name) new(true) // bool to differenciate from the default new operator
#define KIS_MEM_SCOPE_BEGIN(name)
#define KIS_MEM_SCOPE_END(name)
#define KIS_MEM_REGISTER_EXTERNAL(pointer, tag, name)
#define KIS_MEM_UNREGISTER_EXTERNAL(pointer)
#endif

#define KIS_FREE(p) kisMemFree(p)
#define KIS_DELETE(p) kisMemDelete(p); p = nullptr;


#if defined(KIS_DEBUG)
static const uint32_t kisMemEntryMaxNameLength = 32;
#endif

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemInit();
void kisMemShutdown();

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemThreadInit();
void kisMemThreadShutdown();

#if defined(KIS_DEBUG)
// Follow tag names in kisMem.cpp
enum class kisMemTag : uint32_t
{
    App,
    Core,
    Editor,
    Vulkan,
    Count
};
#endif

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
#if defined(KIS_DEBUG)
void* kisMemAlloc(size_t size, kisMemTag tag, const char* name, const char* fileName, uint32_t line, uint32_t alignment = 16);
void* kisMemRealloc(void* p, size_t size, const char* fileName, uint32_t line);
void* operator new(size_t size, kisMemTag tag, const char* name, const char* fileName, uint32_t line, uint32_t alignment = 16);
void operator delete(void* p, size_t size, kisMemTag tag, const char* name, const char* fileName, uint32_t line, uint32_t alignment);
void operator delete(void* p, size_t size);
#else
void* kisMemAlloc(size_t size, uint32_t alignment = 16);
void* kisMemRealloc(void* p, size_t size);
void* operator new(size_t size, bool, uint32_t alignment = 16);
void operator delete(void* p, size_t size, bool, uint32_t alignment);
#endif

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemFree(void*);

#if defined(KIS_DEBUG)
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemScopeBegin(kisMemTag tag);
void kisMemScopeEnd(kisMemTag tag);

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemRegisterEntry(const void* p, kisMemTag tag, const char* name, const char* fileName, uint32_t line);
void kisMemUnregisterEntry(const void* p);
#endif

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename T>
void kisMemDelete(T* p)
{
    p->~T();
    kisMemFree(p);
}

