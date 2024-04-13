#pragma once

#include "kisCoreShared.h"
#include "kisFixedStack.h"

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
const uint32_t k_kisMarkersNumBytes = 16;
const uint32_t k_kisMaxNumMarkers = k_kisMarkersNumBytes * 8;
const uint32_t k_kisMaxMarkersStackDepth = 16;

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
#if defined(KIS_DEBUG)
#define KIS_ALLOC(size, name) kisMemAlloc(size, name, __FILE__, __LINE__)
#define KIS_ALIGNED_ALLOC(size, alignment, name) kisMemAlloc(size, name, __FILE__, __LINE__, alignment)
#else
#define KIS_ALLOC(size, name) kisMemAlloc(size)
#define KIS_ALIGNED_ALLOC(size, alignment, name) kisMemAlloc(size, alignment)
#endif
#define KIS_FREE(alloc) kisMemFree(alloc)

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
struct KisMemMarker
{
    uint8_t m_bitfield[k_kisMarkersNumBytes];
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
struct kisMemScope
{
    kisFixedStack<KisMemMarker, k_kisMaxMarkersStackDepth> m_markersStack;
};

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

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisSetMarker(const char* markerName);


