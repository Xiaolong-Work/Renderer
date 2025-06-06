#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) out vec2 fragTexCoord;

vec2 positions[6] = vec2[6]
(
    vec2(-1.0, -1.0), vec2(-1.0,  1.0), vec2( 1.0, -1.0),
    vec2(-1.0,  1.0), vec2( 1.0,  1.0), vec2( 1.0, -1.0)
);

vec2 texCoords[6] = vec2[6]
(
    vec2(0.0, 0.0), vec2(0.0, 1.0), vec2(1.0, 0.0),
    vec2(0.0, 1.0), vec2(1.0, 1.0), vec2(1.0, 0.0)
);

void main() 
{
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragTexCoord = texCoords[gl_VertexIndex];
}