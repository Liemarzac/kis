#pragma once

#include "kisGraphicsShared.h"

#include <kisCore/kisFixedArray.h>
#include <kisCore/kisCore.h>

struct kisRenderParams
{
    kisMeshInstance* m_instances;
    uint32_t m_nInstances;
};

const uint32_t kisMaxNumMeshInstances = 1024;
typedef kisFixedArray<kisMeshInstance, kisMaxNumMeshInstances> kisMeshInstanceArray;
extern kisMeshInstanceArray g_kisMeshInstances;


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
size_t kisGraphicsImageSize2D(uint32_t width, uint32_t height, kisTextureFormat format);
