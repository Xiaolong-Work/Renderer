#version 450

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 9) uniform sampler2D textureSampler;

const float gamma = 1.0 / 0.7f;

void main() 
{
    // Sample the texture color (assumed to be in sRGB space)
    vec4 srgbColor = texture(textureSampler, fragTexCoord);
    
    // Convert from sRGB to Linear space
    vec3 linearColor = pow(srgbColor.rgb, vec3(gamma));

    // Output the linear color with the original alpha
    outColor = vec4(linearColor, srgbColor.a);
}