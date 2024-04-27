#pragma once

#include "kisGraphicsShared.h"

#include <kisCore/kisCore.h>

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
struct kisDDSFileInfo
{
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_pitch;
    uint32_t m_depth;
    uint32_t m_mipMapCount;
    uint32_t m_dataOffset;
    kisTextureFormat m_format;
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisGraphicsInit(const void* metalLayer);
void kisGraphicsShutdown();

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisGraphicsRender();
void kisGraphicsResize(uint32_t width, uint32_t height);

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
kisMeshHandle kisGraphicsCreateMesh(kisVertex_XYZ_UV_Color_Normal_Tangent_Bitangent* vertices, uint32_t nVertices, void* indices, uint32_t nIndices, kisIndexBufferType indexType);
void kisGraphicsDestroyMesh(kisMeshHandle meshHandle);

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
kisTextureHandle kisGraphicsCreateTexture2D(const void* data, size_t size, uint32_t width, uint32_t height, uint32_t mipLevels, kisTextureFormat format);
void kisGraphicsDestroyTexture(kisTextureHandle textureHandle);

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
kisMeshInstanceHandle kisGraphicsAddMeshInstance(kisMeshHandle meshHandle);

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
kisMeshInstance& kisGraphicsGetMeshInstance(kisMeshInstanceHandle meshInstanceHandle);

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
bool kisGraphicsGetDDSFileInfo(void* ddsData, size_t size, kisDDSFileInfo& info_out);


