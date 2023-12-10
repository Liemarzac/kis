#include "kisGraphics.h"
#include "kisGraphicsShared.h"
#include "kisVk/kisVk.h"

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
    kisVkRender();
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisGraphicsResize(uint32_t width, uint32_t height)
{
    kisVkResize(width, height);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void kisGraphicsCreateMesh(kisVertex_XYZ_UV_Color_Normal_Tangent_Bitangent* vertices, uint32_t nVertices, void* indices, uint32_t nIndices, kisIndexBufferType indexType)
{
    kisVkCreateMesh(vertices, nVertices, indices, nIndices, indexType);
}

