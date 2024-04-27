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

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
VkFormat kisVkImageFormat(kisTextureFormat format)
{
    switch (format)
    {
        case kisTextureFormat::R8G8B8A8:
            return VK_FORMAT_R8G8B8A8_UNORM;

        case kisTextureFormat::BC1_RGB:
            return VK_FORMAT_BC1_RGB_UNORM_BLOCK;

        case kisTextureFormat::BC1_RGBA:
            return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;

        case kisTextureFormat::BC2:
            return VK_FORMAT_BC2_UNORM_BLOCK;

        case kisTextureFormat::BC3:
            return VK_FORMAT_BC3_UNORM_BLOCK;

        case kisTextureFormat::BC4:
            return VK_FORMAT_BC4_UNORM_BLOCK;

        case kisTextureFormat::BC5:
            return VK_FORMAT_BC5_UNORM_BLOCK;

        case kisTextureFormat::BC6:
            return VK_FORMAT_BC6H_UFLOAT_BLOCK;

        case kisTextureFormat::BC7:
            return VK_FORMAT_BC7_UNORM_BLOCK;

        default:
            KIS_LOG_ASSERT(false, "Unsupported image format");
            return VK_FORMAT_UNDEFINED;
    }
}
