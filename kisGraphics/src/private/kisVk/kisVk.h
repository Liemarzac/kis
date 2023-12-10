#pragma once

#include <kisCore/kisCore.h>

#include "kisGraphicsShared.h"

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkInit(const void* metalLayer);
void kisVkShutdown();

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkRender();

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkCreateMesh(kisVertex_XYZ_UV_Color_Normal_Tangent_Bitangent* vertices, uint32_t nVertices, void* indices, uint32_t nIndices, kisIndexBufferType indexType);

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisVkResize(uint32_t width, uint32_t height);
