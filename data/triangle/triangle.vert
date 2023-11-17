#version 450

// Uniforms
layout(set = 0, binding = 0) uniform ObjectUBO
{
    mat4 m_modelViewProj;
} objectUBO;

// Vertex input
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

// Vertex output
layout(location = 0) out vec3 fragColor;

//mat4 g_identity = mat4(1.0f, 0.0f, 0.0f, 0.0f,
//                       0.0f, 1.0f, 0.0f, 0.0f,
//                       0.0f, 0.0f, 1.0f, 0.0f,
//                       0.0f, 0.0f, 0.0f, 1.0f);

void main()
{
    gl_Position = objectUBO.m_modelViewProj * vec4(inPosition, 0.0, 1.0);
    //gl_Position = vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}
