#include "kisMem.h"

#include <cstdlib>

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void* kisMemAlloc(size_t size, uint32_t alignment)
{
    size_t alignedSize = kisAlignPowerOf2(size, alignment);
    return std::aligned_alloc(alignment, alignedSize);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisMemFree(void* p)
{
    std::free(p);
}
