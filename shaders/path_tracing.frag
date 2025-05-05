#version 450

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 9, rgba32f) readonly uniform image2D textureSampler;

void main() 
{
	outColor = imageLoad(textureSampler, ivec2(gl_FragCoord.xy));
}