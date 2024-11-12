#version 450

layout(set = 0, binding = 1) uniform sampler2D samplerColor;

// Pixel input
layout(location = 0) in vec2 inUV;

//Pixel output
layout(location = 0) out vec4 outColor;

void main()
{
    vec4 color = texture(samplerColor, inUV);
    outColor = color + vec4(0.05f);
}
