#version 450
#extension GL_KHR_vulkan_glsl : enable	

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 9, rgba32f) readonly uniform image2D textureSampler;

vec3 gammaCorrect(vec3 color) 
{
    return pow(color, vec3(1.0));
}

void main() 
{
	outColor = vec4(1,0,0,1);
    outColor = vec4(gammaCorrect(imageLoad(textureSampler, ivec2(gl_FragCoord.xy)).xyz), 1);
}