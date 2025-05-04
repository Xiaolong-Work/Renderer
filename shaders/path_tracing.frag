#version 450
#extension GL_KHR_vulkan_glsl : enable	
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) out vec4 out_color;

layout(binding = 9, rgba32f) uniform image2D color[2];
layout(binding = 0, rgba32f) readonly uniform image2D position;
layout(binding = 1, rgba32f) readonly uniform image2D normal;
layout(binding = 2, rgba32f) readonly uniform image2D id;
layout(binding = 3, rgba32f) readonly uniform image2D material_ssao;

struct PushConstantData
{
	mat4 last_camera_matrix;
	int frame_index;
	int frame_count;
};

layout(push_constant) uniform PushConstant { PushConstantData data; };

vec3 gammaCorrect(vec3 color) 
{
    return pow(color, vec3(1.0));
}

int kernel_size = 5;
float sigma_point = 32.0;
float sigma_color = 0.6;
float sigma_normal = 0.1;
float sigma_plane = 0.1;

float computeWeight(ivec2 i, ivec2 j)
{
	float sum = 0;

	float point_distance = length(i - j);
	sum -= point_distance * point_distance / (2.0 * sigma_point);

	vec3 color_i = imageLoad(color[data.frame_index], i).xyz;
	vec3 color_j = imageLoad(color[data.frame_index], j).xyz;
	float color_distance = length(color_j - color_i);
	sum -= color_distance * color_distance / (2.0 * sigma_color);
	
	vec3 normal_i = imageLoad(normal, i).xyz;
	vec3 normal_j = imageLoad(normal, j).xyz;
	float normal_distance = acos(clamp(dot(normal_j, normal_i), 0.0, 1.0));
	sum -= normal_distance * normal_distance / (2.0 * sigma_normal);
	
	vec3 position_i = imageLoad(position, i).xyz;
	vec3 position_j = imageLoad(position, j).xyz;
	float plane_distance = max(dot(normal_i, normalize(position_j - position_i)), 0);
	sum -= plane_distance * plane_distance / (2.0 * sigma_plane);

	return pow(2.718281828, sum);
}

vec3 deNoise()
{
	ivec2 i = ivec2(gl_FragCoord.xy);

	int min_x = max(0, i.x - kernel_size);
    int max_x = min(imageSize(color[data.frame_index]).x - 1, i.x + kernel_size);
    int min_y = max(0, i.y - kernel_size);
    int max_y = min(imageSize(color[data.frame_index]).y - 1, i.y + kernel_size);

	float sum_weight = 0;
	vec3 sum_color = vec3(0);

	for (int x = min_x; x <= max_x; x++)
	{
		for (int y = min_y; y <= max_y; y++)
		{
			ivec2 j = ivec2(x, y);
			float weight = computeWeight(i, j);
			sum_weight += weight;
			vec3 color = imageLoad(color[data.frame_index], j).xyz;
			sum_color += weight * color;
		}
	}

	return sum_color / sum_weight;
}


void main() 
{
	if (data.frame_count >= 10)
	{
		float a = 1.0f / float(data.frame_count + 1);
		vec4 old_color = imageLoad(color[(data.frame_index + 1) % 2], ivec2(gl_FragCoord.xy));
		vec4 new_color = imageLoad(color[data.frame_index], ivec2(gl_FragCoord.xy));
		vec4 temp = mix(old_color, new_color, a);
		imageStore(color[data.frame_index], ivec2(gl_FragCoord.xy), temp);
		out_color = temp;
		return;
	}

    vec4 result = vec4(deNoise(), 1.0);
    
    vec3 world_position = imageLoad(position, ivec2(gl_FragCoord.xy)).xyz;
    
    vec4 temp = data.last_camera_matrix * vec4(world_position, 1.0);
    vec3 last_clip_position = (temp / temp.w).xyz;
    
    // 将裁剪空间坐标转换为NDC [-1,1]，然后转换为纹理坐标 [0,1]
    vec2 last_uv = last_clip_position.xy * 0.5 + 0.5;
    
	float width = imageSize(color[data.frame_index]).x;
	float height = imageSize(color[data.frame_index]).y;

    // 将纹理坐标转换为像素坐标
    ivec2 last_pix;
    last_pix.x = int(last_uv.x * width + 0.1);
    last_pix.y = int(last_uv.y * height + 0.1);
    
    // 检查坐标是否在有效范围内
    if (last_pix.x >= 0 && last_pix.x < width && 
        last_pix.y >= 0 && last_pix.y < height) 
    {
        // 获取上一帧对应位置的颜色
        vec4 last_result = imageLoad(color[(data.frame_index + 1) % 2], last_pix);

		float now_id = imageLoad(id, ivec2(gl_FragCoord.xy)).x;
		float last_id  = imageLoad(id, last_pix).y;

		if (now_id == last_id)
        result = mix(result, last_result, 0.9);
    }
    
    // 存储结果
    imageStore(color[data.frame_index], ivec2(gl_FragCoord.xy), result);
    out_color = result;
}