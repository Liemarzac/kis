#version 450

// Uniforms
layout(set = 0, binding = 0) uniform ObjectUBO
{
    mat4 m_modelViewProj;
} objectUBO;

// Vertex input
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBitangent;

// Vertex output
layout(location = 0) out vec3 fragColor;

void main()
{
    gl_Position = objectUBO.m_modelViewProj * vec4(inPosition, 1.0);
    fragColor = inColor;
}
