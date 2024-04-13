#include "kisFixedHashMap.h"
#include "kisHash.h"
#include "kisMem.h"
#include "kisStringANSIStatic.h"

#include <rpmalloc/rpmalloc.h>
#include <cstdlib>

#if defined(KIS_DEBUG)
struct kisMemEntry
{
    const void* m_pointer;
    kisStringANSIStatic<32> m_name;
    const char* m_fileName;
    int line;
};
#endif

static bool s_bKisMemInitialized = false;
thread_local bool tl_bKisMemThreadInitialized = false;

#if defined(KIS_DEBUG)
static kisFixedHashMap<const void*, kisMemEntry, 1 << 20, kisHashPointerAsUInt32> s_kisMemEntries;
#endif

#if defined(KIS_DEBUG)
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemRegisterEntry(const void* p, const char* name, const char* fileName, int line)
{
    auto itr = s_kisMemEntries.add(p);
    kisMemEntry& memEntry = itr.getValue();
    memEntry.m_pointer = p;
    memEntry.m_name = name;
    memEntry.m_fileName = fileName;
    memEntry.line = line;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemUnregisterEntry(const void* p)
{
    s_kisMemEntries.remove(p);
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
void* kisMemAlloc(size_t size, const char* name, const char* fileName, uint32_t line, uint32_t alignment)
#else
void* kisMemAlloc(size_t size, uint32_t alignment)
#endif
{
    KIS_ASSERT(s_bKisMemInitialized == true);
    KIS_ASSERT(tl_bKisMemThreadInitialized == true);
    void* p = rpmemalign(alignment, size);

    #if defined(KIS_DEBUG)
    kisMemRegisterEntry(p, name, fileName, line);
    #endif

    return p;
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
