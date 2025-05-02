#version 450
#extension GL_KHR_vulkan_glsl : enable	

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 out_color;

layout(binding = 9, rgba32f) readonly uniform image2D textureSampler[];
layout(binding = 0, rgba32f) readonly uniform image2D position_id;
layout(binding = 1, rgba32f) readonly uniform image2D normal_depth;
layout(binding = 2, rgba32f) readonly uniform image2D albedo;
layout(binding = 3, rgba32f) readonly uniform image2D material_ssao;

vec3 gammaCorrect(vec3 color) 
{
    return pow(color, vec3(1.0));
}


float sigma_point = 32.0;
float sigma_color = 0.6;
float sigma_normal = 0.1;
float sigma_plane = 0.1;

float computeWeight(ivec2 i, ivec2 j)
{
	float sum = 0;

	float point_distance = length(i - j);
	sum -= point_distance * point_distance / (2.0 * sigma_point);

	vec3 color_i = imageLoad(textureSampler[0], i).xyz;
	vec3 color_j = imageLoad(textureSampler[0], j).xyz;
	float color_distance = length(color_j - color_i);
	sum -= color_distance * color_distance / (2.0 * sigma_color);
	
	vec3 normal_i = imageLoad(normal_depth, i).xyz;
	vec3 normal_j = imageLoad(normal_depth, j).xyz;
	float normal_distance = acos(dot(normal_j, normal_i));
	sum -= normal_distance * normal_distance / (2.0 * sigma_normal);
	
	vec3 position_i = imageLoad(position_id, i).xyz;
	vec3 position_j = imageLoad(position_id, j).xyz;
	float plane_distance = max(dot(normal_i, normalize(position_j - position_i)), 0);
	sum -= plane_distance * plane_distance / (2.0 * sigma_plane);

	return pow(2.718281828, sum);
}

int kernel_size = 3;

vec3 deNoise()
{
	ivec2 i = ivec2(gl_FragCoord.xy);

	int min_x = max(0, i.x - kernel_size);
    int max_x = min(imageSize(textureSampler[0]).x - 1, i.x + kernel_size);
    int min_y = max(0, i.y - kernel_size);
    int max_y = min(imageSize(textureSampler[0]).y - 1, i.y + kernel_size);

	float sum_weight = 0;
	vec3 sum_color = vec3(0);

	for (int x = min_x; x <= max_x; x++)
	{
		for (int y = min_y; y <= max_y; y++)
		{
			ivec2 j = ivec2(x, y);
			float weight = computeWeight(i, j);
			sum_weight += weight;
			vec3 color = imageLoad(textureSampler[0], j).xyz;
			sum_color += weight * color;
		}
	}

	return sum_color / sum_weight;
}


void main() 
{
    out_color = vec4(deNoise(), 1.0);
}