#include "kisFixedArray.h"
#include "kisFixedHashMap.h"
#include "kisFlagSet.h"
#include "kisHash.h"
#include "kisMem.h"
#include "kisStringANSIStatic.h"

#include <rpmalloc/rpmalloc.h>
#include <cstdlib>

#if defined(KIS_DEBUG)
// Follow enum kisMemTag in kisMem.h
static const char* s_kisMemTagNames[] = {
    "App",
    "Core",
    "Editor",
    "Vulkan"
};
#endif

#if defined(KIS_DEBUG)
struct kisMemEntry
{
    const void* m_pointer;
    kisStringANSIStatic<kisMemEntryMaxNameLength> m_name;
    kisMemTag m_tag;
    const char* m_fileName;
    uint32_t m_line;
    uint32_t m_id;
};
#endif

static bool s_bKisMemInitialized = false;
thread_local bool tl_bKisMemThreadInitialized = false;

#if defined(KIS_DEBUG)
static kisFixedHashMap<const void*, kisMemEntry, 1 << 20, kisHashPointerAsUInt32> s_kisMemEntries;
static kisFlagSet<kisMemTag, uint64_t> s_kisMemScopeFlagSet;
#endif

#if defined(KIS_DEBUG)
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemRegisterEntry(const void* p, kisMemTag tag, const char* name, const char* fileName, uint32_t line)
{
    KIS_LOG_ASSERT(s_kisMemScopeFlagSet.isSet(tag) == true, "Mem scope for tag %s has not yet begun", s_kisMemTagNames[(uint32_t)tag]);

    static uint32_t s_iAlloc = 0;

    auto itr = s_kisMemEntries.add(p);
    kisMemEntry& memEntry = itr.getValue();
    memEntry.m_pointer = p;
    memEntry.m_tag = tag;
    memEntry.m_name = name;
    memEntry.m_fileName = fileName;
    memEntry.m_line = line;
    memEntry.m_id = s_iAlloc;

    s_iAlloc++;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemUnregisterEntry(const void* p)
{
    s_kisMemEntries.remove(p);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemScopeBegin(kisMemTag tag)
{
    KIS_LOG_ASSERT(s_kisMemScopeFlagSet.isSet(tag) == false, "Mem scope for tag %s is already begun", s_kisMemTagNames[(uint32_t)tag]);

    s_kisMemScopeFlagSet.set(tag);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemScopeEnd(kisMemTag tag)
{
    KIS_LOG_ASSERT(s_kisMemScopeFlagSet.isSet(tag) == true, "Mem scope for tag %s has not begun yet", s_kisMemTagNames[(uint32_t)tag]);

    s_kisMemScopeFlagSet.remove(tag);

    bool bFoundScopeLeaks = false;
    for(auto itr = s_kisMemEntries.begin(); itr.isValid(); ++itr)
    {
        kisMemEntry& memEntry = itr.getValue();
        if(memEntry.m_tag == tag)
        {
            kisLogError("Mem entry [%s] with id [%u] allocated with mem tag [%s] was not freed before the end of the scope.\n%s:%u", memEntry.m_name.c_str(), memEntry.m_id, s_kisMemTagNames[(uint32_t)tag], memEntry.m_fileName, memEntry.m_line);

            bFoundScopeLeaks = true;
        }
    }

    KIS_ASSERT(bFoundScopeLeaks == false);
}
#endif

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemInit()
{
    rpmalloc_initialize();

    s_bKisMemInitialized = true;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemShutdown()
{
    s_bKisMemInitialized = false;

    #if defined(KIS_DEBUG)
    for(uint32_t iTag = 0; iTag < (uint32_t)kisMemTag::Count; iTag++)
    {
        KIS_LOG_ASSERT(s_kisMemScopeFlagSet.isSet((kisMemTag)iTag) == false, "Mem scope %s was never ended", s_kisMemTagNames[iTag]);
    }
    #endif

    rpmalloc_finalize();
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemThreadInit()
{
    rpmalloc_thread_initialize();
    tl_bKisMemThreadInitialized = true;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemThreadShutdown()
{
    tl_bKisMemThreadInitialized = false;
    rpmalloc_thread_finalize(1); // 1 ?
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
#if defined(KIS_DEBUG)
void* kisMemAlloc(size_t size, kisMemTag tag, const char* name, const char* fileName, uint32_t line, uint32_t alignment)
#else
void* kisMemAlloc(size_t size, uint32_t alignment)
#endif
{
    KIS_ASSERT(s_bKisMemInitialized == true);
    KIS_ASSERT(tl_bKisMemThreadInitialized == true);
    void* p = rpmemalign(alignment, size);

    #if defined(KIS_DEBUG)
    kisMemRegisterEntry(p, tag, name, fileName, line);
    #endif

    return p;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
#if defined(KIS_DEBUG)
void* kisMemRealloc(void* oldPointer, size_t size, const char* fileName, uint32_t line)
#else
void* kisMemRealloc(void* oldPointer, size_t size)
#endif
{
    KIS_ASSERT(s_bKisMemInitialized == true);
    KIS_ASSERT(tl_bKisMemThreadInitialized == true);

    #if defined(KIS_DEBUG)
    auto itr = s_kisMemEntries.find(oldPointer);
    kisMemEntry oldMemEntry = itr.getValue();
    kisMemUnregisterEntry(oldPointer);
    #endif

    void* newPointer = rprealloc(oldPointer, size);

    #if defined(KIS_DEBUG)
    kisMemRegisterEntry(newPointer, oldMemEntry.m_tag, oldMemEntry.m_name.c_str(), fileName, line);
    #endif

    return newPointer;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemFree(void* p)
{
    KIS_ASSERT(s_bKisMemInitialized == true);
    KIS_ASSERT(tl_bKisMemThreadInitialized == true);

    #if defined(KIS_DEBUG)
    kisMemUnregisterEntry(p);
    #endif

    rpfree(p);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
#if defined(KIS_DEBUG)
void* operator new(size_t size, kisMemTag tag, const char* name, const char* fileName, uint32_t line, uint32_t alignment)
#else
void* operator new(size_t size, bool, uint32_t alignment)
#endif
{
    #if defined(KIS_DEBUG)
    void* p = kisMemAlloc(size, tag, name, fileName, line, alignment);
    #else
    void* p = kisMemAlloc(size, alignment)
    #endif

    return p;
}
