#define VMA_IMPLEMENTATION
#pragma clang diagnostic push
//#pragma clang diagnostic ignored "-Wdocumentation"
//#pragma clang diagnostic ignored "-Wnullability-completeness"
#pragma clang diagnostic ignored "-Wall"
//#include <vk_mem_alloc.h>
#pragma clang diagnostic pop


#include "kisVkPrivate.h"


kisVkInfo g_kisVkInfo;
VkAllocationCallbacks g_kisVkAllocCallbacks;


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
VkIndexType kisVkIndexType(kisIndexBufferType type)
{
    switch (type)
    {
        case kisIndexBufferType::U16:
            return VK_INDEX_TYPE_UINT16;

        case kisIndexBufferType::U32:
            return VK_INDEX_TYPE_UINT32;

        default:
            KIS_ASSERT(false);
            break;
    }

    return VK_INDEX_TYPE_UINT32;
}
