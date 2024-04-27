#pragma once

#include <kisCore/kisCore.h>

#include "kisGraphicsPrivate.h"
#include "kisGraphicsShared.h"

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkInit(const void* metalLayer);
void kisVkShutdown();

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkRender(const kisRenderParams& renderParams);

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
uint32_t kisVkCreateMesh(kisVertex_XYZ_UV_Color_Normal_Tangent_Bitangent* vertices, uint32_t nVertices, void* indices, uint32_t nIndices, kisIndexBufferType indexType);
void kisVkDestroyMesh(uint32_t iMesh);

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
uint32_t kisVkCreateTexture2D(const void* data, size_t size, uint32_t width, uint32_t height, uint32_t mipLevels, kisTextureFormat format);
void kisVkDestroyTexture2D(uint32_t iImage);

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkResize(uint32_t width, uint32_t height);
