#pragma once

#include <kisCore/kisArray.h>
#include <kisCore/kisCore.h>
#include <kisCore/kisMath.h>

struct kisMeshInstance
{
    uint32_t m_iMesh;
    kisMatrix4 m_transform;
};

struct kisRenderParams
{
    kisMeshInstance* m_instances;
    uint32_t m_nInstances;
};

extern kisArray<kisMeshInstance> g_kisMeshInstances;
