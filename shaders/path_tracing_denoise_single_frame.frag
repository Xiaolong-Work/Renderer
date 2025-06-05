#version 460
#extension GL_KHR_vulkan_glsl : enable	
#extension GL_EXT_nonuniform_qualifier : enable

layout(binding = 0, rgba32f) writeonly uniform image2D result;
layout(binding = 1, rgba32f) readonly uniform image2D position;
layout(binding = 2, rgba32f) readonly uniform image2D normal;
layout(binding = 3, rgba32f) uniform image2D color[2];

struct DenoisePushConstantData
{
	mat4 last_camera_matrix;
	int current_index;
	int frame_count;
	int kernel_size;
	float sigma_point;
	float sigma_color ;
	float sigma_normal;
	float sigma_plane;
	bool enable_single_frame_denoise;
};
layout(push_constant) uniform PushConstant { DenoisePushConstantData param; };

float computeWeight(ivec2 i, ivec2 j)
{
	float sum = 0;

	float point_distance = length(i - j);
	sum -= point_distance * point_distance / (2.0 * param.sigma_point);

	vec3 color_i = imageLoad(color[param.current_index], i).xyz;
	vec3 color_j = imageLoad(color[param.current_index], j).xyz;
	float color_distance = length(color_j - color_i);
	sum -= color_distance * color_distance / (2.0 * param.sigma_color);
	
	vec3 normal_i = imageLoad(normal, i).xyz;
	vec3 normal_j = imageLoad(normal, j).xyz;
	float normal_distance = acos(clamp(dot(normal_j, normal_i), 0.0, 1.0));
	sum -= normal_distance * normal_distance / (2.0 * param.sigma_normal);
	
	vec3 position_i = imageLoad(position, i).xyz;
	vec3 position_j = imageLoad(position, j).xyz;
	float plane_distance = max(dot(normal_i, normalize(position_j - position_i)), 0);
	sum -= plane_distance * plane_distance / (2.0 * param.sigma_plane);

	return pow(2.718281828, sum);
}

vec4 denoise()
{
	ivec2 i = ivec2(gl_FragCoord.xy);

	int min_x = max(0, i.x - param.kernel_size);
    int max_x = min(imageSize(color[param.current_index]).x - 1, i.x + param.kernel_size);
    int min_y = max(0, i.y - param.kernel_size);
    int max_y = min(imageSize(color[param.current_index]).y - 1, i.y + param.kernel_size);

	float sum_weight = 0;
	vec4 sum_color = vec4(0);

	for (int x = min_x; x <= max_x; x++)
	{
		for (int y = min_y; y <= max_y; y++)
		{
			ivec2 j = ivec2(x, y);
			float weight = computeWeight(i, j);
			vec4 color = imageLoad(color[param.current_index], j);
			
			sum_weight += weight;
			sum_color += weight * color;
		}
	}

	return sum_color / sum_weight;
}

void main()
{
	imageStore(result, ivec2(gl_FragCoord.xy), denoise());
}