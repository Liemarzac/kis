#include "kisFixedHashMap.h"
#include "kisHash.h"
#include "kisMem.h"

#include <rpmalloc/rpmalloc.h>
#include <cstdlib>

static thread_local kisMemScope s_kisThreadLocalMemScope;

bool s_bKisMemInitialized = false;

thread_local bool tl_bKisMemThreadInitialized = false;

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
void* kisMemAlloc(size_t size, uint32_t alignment)
{
    KIS_ASSERT(s_bKisMemInitialized == true);
    KIS_ASSERT(tl_bKisMemThreadInitialized == true);
    return rpmemalign(alignment, size);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemFree(void* p)
{
    KIS_ASSERT(s_bKisMemInitialized == true);
    KIS_ASSERT(tl_bKisMemThreadInitialized == true);
    rpfree(p);
}
