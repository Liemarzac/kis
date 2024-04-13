#include "kisFixedArray.h"
#include "kisFixedHashMap.h"
#include "kisHash.h"
#include "kisMem.h"
#include "kisStringANSIStatic.h"

#include <rpmalloc/rpmalloc.h>
#include <cstdlib>

#if defined(KIS_DEBUG)
static const uint32_t k_kisMemMarkersMaxNumUInt64 = 2;
static const uint32_t k_kisMemMarkersMaxNum = 64 * k_kisMemMarkersMaxNumUInt64;
#endif

#if defined(KIS_DEBUG)
struct kisMemScopeBitfield
{
    uint64_t m_bitfield[k_kisMemMarkersMaxNumUInt64];
};

struct kisMemEntry
{
    const void* m_pointer;
    kisStringANSIStatic<32> m_name;
    kisMemScopeBitfield m_memScopeBitfield;
    const char* m_fileName;
    uint32_t m_line;
};
#endif

static bool s_bKisMemInitialized = false;
thread_local bool tl_bKisMemThreadInitialized = false;

#if defined(KIS_DEBUG)
static kisFixedHashMap<const void*, kisMemEntry, 1 << 20, kisHashPointerAsUInt32> s_kisMemEntries;
static kisFixedArray<kisStringANSIStatic<32>, k_kisMemMarkersMaxNum> s_kisScopeMarkerNames;
static kisMemScopeBitfield s_kisMemScopeBitfield;
#endif

#if defined(KIS_DEBUG)
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemRegisterEntry(const void* p, const char* name, const char* fileName, uint32_t line)
{
    auto itr = s_kisMemEntries.add(p);
    kisMemEntry& memEntry = itr.getValue();
    memEntry.m_pointer = p;
    memEntry.m_name = name;
    memEntry.m_memScopeBitfield = s_kisMemScopeBitfield;
    memEntry.m_fileName = fileName;
    memEntry.m_line = line;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemUnregisterEntry(const void* p)
{
    s_kisMemEntries.remove(p);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
uint32_t kisMemScopeFindOrCreate(const char* name)
{
    const uint32_t nNames = s_kisScopeMarkerNames.num();
    for(uint32_t i = 0; i < nNames; i++)
    {
        if(s_kisScopeMarkerNames[i].len() == 0)
        {
            // Free slot found
            break;
        }

        if(s_kisScopeMarkerNames[i] == name)
        {
            return i;
        }
    }

    s_kisScopeMarkerNames.add() = name;
    return s_kisScopeMarkerNames.num() - 1;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemScopeStart(uint32_t index)
{
    uint32_t iUInt64 = index / 64;
    uint32_t iBit = index % 64;
    KIS_LOG_ASSERT((s_kisMemScopeBitfield.m_bitfield[iUInt64] & (1ull << iBit)) == 0, "Mem scope %s is already started", s_kisScopeMarkerNames[index].c_str());

    s_kisMemScopeBitfield.m_bitfield[iUInt64] |= (1ull << iBit);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemScopeEnd(uint32_t index)
{
    uint32_t iUInt64 = index / 64;
    uint32_t iBit = index % 64;
    KIS_LOG_ASSERT((s_kisMemScopeBitfield.m_bitfield[iUInt64] & (1ull << iBit)) != 0, "Mem scope %s is ended before being started", s_kisScopeMarkerNames[index].c_str());

    s_kisMemScopeBitfield.m_bitfield[iUInt64] &= ~(1ull << iBit);

    for(auto itr = s_kisMemEntries.begin(); itr.isValid(); ++itr)
    {
        kisMemEntry& memEntry = itr.getValue();
        KIS_LOG_ASSERT((memEntry.m_memScopeBitfield.m_bitfield[iUInt64] & (1ull << iBit)) == 0, "Mem entry [%s] allocated within mem scope [%s] was not freed before the end of the scope.\n%s:%u", memEntry.m_name.c_str(), s_kisScopeMarkerNames[(iUInt64 * 64) + iBit], memEntry.m_fileName, memEntry.m_line);
    }
}
#endif

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemInit()
{
    rpmalloc_initialize();

    #if defined(KIS_DEBUG)
    memset(&s_kisMemScopeBitfield.m_bitfield, 0, sizeof(s_kisMemScopeBitfield.m_bitfield));
    #endif

    s_bKisMemInitialized = true;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemShutdown()
{
    s_bKisMemInitialized = false;

    #if defined(KIS_DEBUG)
    for(uint32_t iUInt64 = 0; iUInt64 < KIS_ARRAY_COUNT(s_kisMemScopeBitfield.m_bitfield); iUInt64++)
    {
        for(uint32_t iBit = 0; iBit < 64; iBit++)
        {
            if((s_kisMemScopeBitfield.m_bitfield[iUInt64] & (1ull << iBit)) != 0)
            {
                KIS_LOG_ASSERT(false, "Mem scope was never ended: %s", s_kisScopeMarkerNames[(iUInt64 * 64) + iBit].c_str());
            }
        }
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
