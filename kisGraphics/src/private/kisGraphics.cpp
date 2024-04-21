#include "kisGraphics.h"
#include "kisGraphicsShared.h"
#include "kisGraphicsPrivate.h"

#include "kisVk/kisVk.h"

#include "kisMem.h"

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

