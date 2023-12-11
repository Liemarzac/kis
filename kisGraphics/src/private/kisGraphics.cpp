#include "kisGraphics.h"
#include "kisGraphicsShared.h"
#include "kisGraphicsPrivate.h"
#include "kisVk/kisVk.h"

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
kisArray<kisMeshInstance> g_kisMeshInstances;

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
uint32_t kisGraphicsCreateMesh(kisVertex_XYZ_UV_Color_Normal_Tangent_Bitangent* vertices, uint32_t nVertices, void* indices, uint32_t nIndices, kisIndexBufferType indexType)
{
    return kisVkCreateMesh(vertices, nVertices, indices, nIndices, indexType);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
uint32_t kisGraphicsCreateInstance(uint32_t iMesh)
{
    kisMeshInstance& instance = g_kisMeshInstances.add();
    instance.m_iMesh = iMesh;
    return g_kisMeshInstances.num() - 1;
}

