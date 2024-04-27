#include "kisGraphics.h"
#include "kisGraphicsShared.h"
#include "kisGraphicsPrivate.h"

#include "kisVk/kisVk.h"

#include "kisMem.h"

#include <string.h>

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
kisMeshInstanceArray g_kisMeshInstances;

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisGraphicsInit(const void* metalLayer)
{
    kisVkInit(metalLayer);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisGraphicsShutdown()
{
    kisVkShutdown();
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisGraphicsRender()
{
    kisRenderParams renderParams;
    renderParams.m_instances = g_kisMeshInstances.dataPointer();
    renderParams.m_nInstances = g_kisMeshInstances.num();
    kisVkRender(renderParams);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisGraphicsResize(uint32_t width, uint32_t height)
{
    kisVkResize(width, height);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
kisMeshHandle kisGraphicsCreateMesh(kisVertex_XYZ_UV_Color_Normal_Tangent_Bitangent* vertices, uint32_t nVertices, void* indices, uint32_t nIndices, kisIndexBufferType indexType)
{
    kisMeshHandle handle = {
        .m_index = kisVkCreateMesh(vertices, nVertices, indices, nIndices, indexType)
    };

    return handle;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisGraphicsDestroyMesh(kisMeshHandle handle)
{
    kisVkDestroyMesh(handle.m_index);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
kisTextureHandle kisGraphicsCreateTexture2D(const void* data, size_t size, uint32_t width, uint32_t height, uint32_t mipLevels, kisTextureFormat format)
{
    kisTextureHandle handle = {
        .m_index = kisVkCreateTexture2D(data, size, width, height, mipLevels, format)
    };

    return handle;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisGraphicsDestroyTexture(kisTextureHandle textureHandle)
{
    kisVkDestroyTexture2D(textureHandle.m_index);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
kisMeshInstanceHandle kisGraphicsAddMeshInstance(kisMeshHandle meshHandle)
{
    kisMeshInstance& instance = g_kisMeshInstances.add();
    instance.m_meshHandle = meshHandle;

    kisMeshInstanceHandle meshInstanceHandle = {
        .m_index = g_kisMeshInstances.num() - 1
    };

    return meshInstanceHandle;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
kisMeshInstance& kisGraphicsGetMeshInstance(kisMeshInstanceHandle handle)
{
    return g_kisMeshInstances[handle.m_index];
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
size_t kisGraphicsImageSize2D(uint32_t width, uint32_t height, kisTextureFormat format)
{
    switch (format)
    {
        case kisTextureFormat::R8G8B8A8:
            return width * height * 4;

        case kisTextureFormat::BC1_RGB:
        case kisTextureFormat::BC1_RGBA:
        case kisTextureFormat::BC4:
            return width * height / 2;

        case kisTextureFormat::BC2:
        case kisTextureFormat::BC3:
        case kisTextureFormat::BC5:
        case kisTextureFormat::BC6:
        case kisTextureFormat::BC7:
            return width * height;

        default:
            KIS_LOG_ASSERT(false, "Unsupported image format");
            return 0;
    }
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
bool kisGraphicsGetDDSFileInfo(void* ddsData, size_t size, kisDDSFileInfo& info_out)
{
    enum DDPF
    {
        DDPF_ALPHAPIXELS = 0x1,
        DDPF_ALPHA = 0x2,
        DDPF_FOURCC = 0x4,
        DDPF_RGB = 0x40,
        DDPF_YUV = 0x200,
        DDPF_LUMINANCE = 0x20000
    };

    enum DXGI_FORMAT
    {
        DXGI_FORMAT_UNKNOWN = 0,
        DXGI_FORMAT_R32G32B32A32_TYPELESS = 1,
        DXGI_FORMAT_R32G32B32A32_FLOAT = 2,
        DXGI_FORMAT_R32G32B32A32_UINT = 3,
        DXGI_FORMAT_R32G32B32A32_SINT = 4,
        DXGI_FORMAT_R32G32B32_TYPELESS = 5,
        DXGI_FORMAT_R32G32B32_FLOAT = 6,
        DXGI_FORMAT_R32G32B32_UINT = 7,
        DXGI_FORMAT_R32G32B32_SINT = 8,
        DXGI_FORMAT_R16G16B16A16_TYPELESS = 9,
        DXGI_FORMAT_R16G16B16A16_FLOAT = 10,
        DXGI_FORMAT_R16G16B16A16_UNORM = 11,
        DXGI_FORMAT_R16G16B16A16_UINT = 12,
        DXGI_FORMAT_R16G16B16A16_SNORM = 13,
        DXGI_FORMAT_R16G16B16A16_SINT = 14,
        DXGI_FORMAT_R32G32_TYPELESS = 15,
        DXGI_FORMAT_R32G32_FLOAT = 16,
        DXGI_FORMAT_R32G32_UINT = 17,
        DXGI_FORMAT_R32G32_SINT = 18,
        DXGI_FORMAT_R32G8X24_TYPELESS = 19,
        DXGI_FORMAT_D32_FLOAT_S8X24_UINT = 20,
        DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
        DXGI_FORMAT_X32_TYPELESS_G8X24_UINT = 22,
        DXGI_FORMAT_R10G10B10A2_TYPELESS = 23,
        DXGI_FORMAT_R10G10B10A2_UNORM = 24,
        DXGI_FORMAT_R10G10B10A2_UINT = 25,
        DXGI_FORMAT_R11G11B10_FLOAT = 26,
        DXGI_FORMAT_R8G8B8A8_TYPELESS = 27,
        DXGI_FORMAT_R8G8B8A8_UNORM = 28,
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
        DXGI_FORMAT_R8G8B8A8_UINT = 30,
        DXGI_FORMAT_R8G8B8A8_SNORM = 31,
        DXGI_FORMAT_R8G8B8A8_SINT = 32,
        DXGI_FORMAT_R16G16_TYPELESS = 33,
        DXGI_FORMAT_R16G16_FLOAT = 34,
        DXGI_FORMAT_R16G16_UNORM = 35,
        DXGI_FORMAT_R16G16_UINT = 36,
        DXGI_FORMAT_R16G16_SNORM = 37,
        DXGI_FORMAT_R16G16_SINT = 38,
        DXGI_FORMAT_R32_TYPELESS = 39,
        DXGI_FORMAT_D32_FLOAT = 40,
        DXGI_FORMAT_R32_FLOAT = 41,
        DXGI_FORMAT_R32_UINT = 42,
        DXGI_FORMAT_R32_SINT = 43,
        DXGI_FORMAT_R24G8_TYPELESS = 44,
        DXGI_FORMAT_D24_UNORM_S8_UINT = 45,
        DXGI_FORMAT_R24_UNORM_X8_TYPELESS = 46,
        DXGI_FORMAT_X24_TYPELESS_G8_UINT = 47,
        DXGI_FORMAT_R8G8_TYPELESS = 48,
        DXGI_FORMAT_R8G8_UNORM = 49,
        DXGI_FORMAT_R8G8_UINT = 50,
        DXGI_FORMAT_R8G8_SNORM = 51,
        DXGI_FORMAT_R8G8_SINT = 52,
        DXGI_FORMAT_R16_TYPELESS = 53,
        DXGI_FORMAT_R16_FLOAT = 54,
        DXGI_FORMAT_D16_UNORM = 55,
        DXGI_FORMAT_R16_UNORM = 56,
        DXGI_FORMAT_R16_UINT = 57,
        DXGI_FORMAT_R16_SNORM = 58,
        DXGI_FORMAT_R16_SINT = 59,
        DXGI_FORMAT_R8_TYPELESS = 60,
        DXGI_FORMAT_R8_UNORM = 61,
        DXGI_FORMAT_R8_UINT = 62,
        DXGI_FORMAT_R8_SNORM = 63,
        DXGI_FORMAT_R8_SINT = 64,
        DXGI_FORMAT_A8_UNORM = 65,
        DXGI_FORMAT_R1_UNORM = 66,
        DXGI_FORMAT_R9G9B9E5_SHAREDEXP = 67,
        DXGI_FORMAT_R8G8_B8G8_UNORM = 68,
        DXGI_FORMAT_G8R8_G8B8_UNORM = 69,
        DXGI_FORMAT_BC1_TYPELESS = 70,
        DXGI_FORMAT_BC1_UNORM = 71,
        DXGI_FORMAT_BC1_UNORM_SRGB = 72,
        DXGI_FORMAT_BC2_TYPELESS = 73,
        DXGI_FORMAT_BC2_UNORM = 74,
        DXGI_FORMAT_BC2_UNORM_SRGB = 75,
        DXGI_FORMAT_BC3_TYPELESS = 76,
        DXGI_FORMAT_BC3_UNORM = 77,
        DXGI_FORMAT_BC3_UNORM_SRGB = 78,
        DXGI_FORMAT_BC4_TYPELESS = 79,
        DXGI_FORMAT_BC4_UNORM = 80,
        DXGI_FORMAT_BC4_SNORM = 81,
        DXGI_FORMAT_BC5_TYPELESS = 82,
        DXGI_FORMAT_BC5_UNORM = 83,
        DXGI_FORMAT_BC5_SNORM = 84,
        DXGI_FORMAT_B5G6R5_UNORM = 85,
        DXGI_FORMAT_B5G5R5A1_UNORM = 86,
        DXGI_FORMAT_B8G8R8A8_UNORM = 87,
        DXGI_FORMAT_B8G8R8X8_UNORM = 88,
        DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
        DXGI_FORMAT_B8G8R8A8_TYPELESS = 90,
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB = 91,
        DXGI_FORMAT_B8G8R8X8_TYPELESS = 92,
        DXGI_FORMAT_B8G8R8X8_UNORM_SRGB = 93,
        DXGI_FORMAT_BC6H_TYPELESS = 94,
        DXGI_FORMAT_BC6H_UF16 = 95,
        DXGI_FORMAT_BC6H_SF16 = 96,
        DXGI_FORMAT_BC7_TYPELESS = 97,
        DXGI_FORMAT_BC7_UNORM = 98,
        DXGI_FORMAT_BC7_UNORM_SRGB = 99,
        DXGI_FORMAT_AYUV = 100,
        DXGI_FORMAT_Y410 = 101,
        DXGI_FORMAT_Y416 = 102,
        DXGI_FORMAT_NV12 = 103,
        DXGI_FORMAT_P010 = 104,
        DXGI_FORMAT_P016 = 105,
        DXGI_FORMAT_420_OPAQUE = 106,
        DXGI_FORMAT_YUY2 = 107,
        DXGI_FORMAT_Y210 = 108,
        DXGI_FORMAT_Y216 = 109,
        DXGI_FORMAT_NV11 = 110,
        DXGI_FORMAT_AI44 = 111,
        DXGI_FORMAT_IA44 = 112,
        DXGI_FORMAT_P8 = 113,
        DXGI_FORMAT_A8P8 = 114,
        DXGI_FORMAT_B4G4R4A4_UNORM = 115,
        DXGI_FORMAT_P208 = 130,
        DXGI_FORMAT_V208 = 131,
        DXGI_FORMAT_V408 = 132,
        DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE,
        DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE,
        DXGI_FORMAT_FORCE_UINT = 0xffffffff
    };

    enum D3D10_RESOURCE_DIMENSION
    {
        D3D10_RESOURCE_DIMENSION_UNKNOWN = 0,
        D3D10_RESOURCE_DIMENSION_BUFFER = 1,
        D3D10_RESOURCE_DIMENSION_TEXTURE1D = 2,
        D3D10_RESOURCE_DIMENSION_TEXTURE2D = 3,
        D3D10_RESOURCE_DIMENSION_TEXTURE3D = 4
    };

    struct DDS_PIXELFORMAT
    {
        uint32_t dwSize;
        uint32_t dwFlags;
        uint32_t dwFourCC;
        uint32_t dwRGBBitCount;
        uint32_t dwRBitMask;
        uint32_t dwGBitMask;
        uint32_t dwBBitMask;
        uint32_t dwABitMask;
    };

    struct DDS_HEADER
    {
        uint32_t           dwSize;
        uint32_t           dwFlags;
        uint32_t           dwHeight;
        uint32_t           dwWidth;
        uint32_t           dwPitchOrLinearSize;
        uint32_t           dwDepth;
        uint32_t           dwMipMapCount;
        uint32_t           dwReserved1[11];
        DDS_PIXELFORMAT    ddspf;
        uint32_t           dwCaps;
        uint32_t           dwCaps2;
        uint32_t           dwCaps3;
        uint32_t           dwCaps4;
        uint32_t           dwReserved2;
    } ddsHeader;

//    struct DDS_HEADER_DXT10
//    {
//        DXGI_FORMAT                     dxgiFormat;
//        D3D10_RESOURCE_DIMENSION        resourceDimension;
//        uint32_t                        miscFlag;
//        uint32_t                        arraySize;
//        uint32_t                        miscFlags2;
//    } ddsHeaderDXT10;

    const uint8_t* end = (uint8_t*)ddsData + size;

    uint8_t* p = (uint8_t*)ddsData;

    uint32_t magicNumber;
    if(p + sizeof(magicNumber) > end)
    {
        return false;
    }

    memcpy(&magicNumber, p, sizeof(magicNumber));

    if(magicNumber != 0x20534444)
    {
        // Incorrect tag.
        return false;
    }

    p += sizeof(magicNumber);

    if(p + sizeof(DDS_HEADER) > end)
    {
        return false;
    }

    memcpy(&ddsHeader, p, sizeof(ddsHeader));
    p += sizeof(ddsHeader);

    if((ddsHeader.ddspf.dwFlags & DDPF_FOURCC) != 0)
    {
        char fourCC[5];
        memcpy(fourCC, &ddsHeader.ddspf.dwFourCC, 4);
        fourCC[4] = '\0';

        #define MAKE_FOURCC(A, B, C, D) ((uint32_t)A << 24) | ((uint32_t)B << 16) | ((uint32_t)C << 8) | ((uint32_t)D);
        const uint32_t fourCCAsUint32 = MAKE_FOURCC(fourCC[0], fourCC[1], fourCC[2], fourCC[3]);
        //const uint32_t DX10 = MAKE_FOURCC('D', 'X', '1', '0');
        const uint32_t DXT1 = MAKE_FOURCC('D', 'X', 'T', '1');
        //const uint32_t DXT2 = MAKE_FOURCC('D', 'X', 'T', '2');
        const uint32_t DXT3 = MAKE_FOURCC('D', 'X', 'T', '3');
        //const uint32_t DXT4 = MAKE_FOURCC('D', 'X', 'T', '4');
        const uint32_t DXT5 = MAKE_FOURCC('D', 'X', 'T', '5');

        switch (fourCCAsUint32)
        {
            case DXT1:
                if((ddsHeader.ddspf.dwFlags & DDPF_ALPHA) != 0)
                {
                    info_out.m_format = kisTextureFormat::BC1_RGBA;
                }
                else
                {
                    info_out.m_format = kisTextureFormat::BC1_RGB;
                }
                break;

            case DXT3:
                info_out.m_format = kisTextureFormat::BC2;
                break;

            case DXT5:
                info_out.m_format = kisTextureFormat::BC3;
                break;
                
            default:
                return false;
        }

        info_out.m_dataOffset = (uint32_t)(p - (uint8_t*)ddsData);
        info_out.m_width = ddsHeader.dwWidth;
        info_out.m_height = ddsHeader.dwHeight;
        info_out.m_depth = ddsHeader.dwDepth;
        info_out.m_mipMapCount = ddsHeader.dwMipMapCount;
        info_out.m_pitch = ddsHeader.dwPitchOrLinearSize;
    }
    return true;
}

