#version 450

layout(location = 0) in vec2 texture_coordinate;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D texture_sampler;

vec3 gammaCorrect(vec3 color) 
{
    return pow(color, vec3(2.2));
}

void main() 
{
	vec3 linear_color = texture(texture_sampler, texture_coordinate).rgb;
    out_color = vec4(gammaCorrect(linear_color), 1.0);
}