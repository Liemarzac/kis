#pragma once

#include <kisCore/kisCore.h>
#include <kisCore/kisMath.h>

struct kisMeshHandle
{
    uint32_t m_index;
};

struct kisMeshInstanceHandle
{
    uint32_t m_index;
};

struct kisMeshInstance
{
    kisMeshHandle m_meshHandle;
    kisMatrix4 m_orientation;
};

struct kisVertex_XYZ_UV_Color_Normal_Tangent_Bitangent
{
    float m_position[3];
    float m_uv[2];
    float m_color[3];
    float m_normal[3];
    float m_tangent[3];
    float m_bitangent[3];
};

enum class kisIndexBufferType
{
    U16,
    U32
};
