#pragma once

#include "kisGraphicsShared.h"

#include <kisCore/kisCore.h>

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
uint32_t kisGraphicsCreateMesh(kisVertex_XYZ_UV_Color_Normal_Tangent_Bitangent* vertices, uint32_t nVertices, void* indices, uint32_t nIndices, kisIndexBufferType indexType);

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
uint32_t kisGraphicsCreateInstance(uint32_t iMesh);

