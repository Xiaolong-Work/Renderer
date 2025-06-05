#version 460
#extension GL_KHR_vulkan_glsl : enable	
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) out vec4 out_color;

layout(binding = 0, rgba32f) readonly uniform image2D denoise;
layout(binding = 1, rgba32f) readonly uniform image2D position;
layout(binding = 2, rgba32f) readonly uniform image2D id;
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
};
layout(push_constant) uniform PushConstant { DenoisePushConstantData param; };

void compute(inout vec4 mu, inout vec4 sigma)
{
	ivec2 i = ivec2(gl_FragCoord.xy);

	int min_x = max(0, i.x - 3);
    int max_x = min(imageSize(color[param.current_index]).x - 1, i.x + 3);
    int min_y = max(0, i.y - 3);
    int max_y = min(imageSize(color[param.current_index]).y - 1, i.y + 3);

	vec4 sum_color = vec4(0);
	for (int x = min_x; x <= max_x; x++)
	{
		for (int y = min_y; y <= max_y; y++)
		{
			ivec2 j = ivec2(x, y);
			vec4 color = imageLoad(color[param.current_index], j);
			sum_color += color;
		}
	}
	mu = sum_color /= 49;

	sum_color = vec4(0);
	for (int x = min_x; x <= max_x; x++)
	{
		for (int y = min_y; y <= max_y; y++)
		{
			ivec2 j = ivec2(x, y);
			vec4 color = imageLoad(color[param.current_index], j);
			vec4 temp = color - mu;
			sum_color += temp * temp;
		}
	}
	sigma = sqrt(sum_color) / 7.0;
}

vec3 gammaCorrect(vec3 color) 
{
    return pow(color, vec3(0.5));
}

vec3 acesToneMapping(vec3 color) 
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

void main()
{
	/* Frame count is greater than 10, camera does not move, accumulate directly */
	if (param.frame_count >= 10)
	{
		float a = 1.0f / float(param.frame_count - 9);
		vec4 old_color = imageLoad(color[(param.current_index + 1) % 2], ivec2(gl_FragCoord.xy));
		vec4 new_color = imageLoad(color[param.current_index], ivec2(gl_FragCoord.xy));
		vec4 result_color = mix(old_color, new_color, a);
		out_color = vec4(gammaCorrect(acesToneMapping(result_color.xyz)), 1.0);
		imageStore(color[param.current_index], ivec2(gl_FragCoord.xy), result_color);
		return;
	}

	vec4 result = imageLoad(denoise, ivec2(gl_FragCoord.xy));

	float width = imageSize(color[param.current_index]).x;
	float height = imageSize(color[param.current_index]).y;
	
	/* Get the world coordinates corresponding to the current pixel */
	vec3 world_position = imageLoad(position, ivec2(gl_FragCoord.xy)).xyz;
	
	/* Calculate the clipping coordinates of the current coordinates in the previous frame */
	vec4 temp = param.last_camera_matrix * vec4(world_position, 1.0);
	
	/* Convert clip space coordinates to NDC [-1,1] and then to texture coordinates [0,1] */
	vec3 last_clip_position = (temp / temp.w).xyz;
	vec2 last_uv = last_clip_position.xy * 0.5 + 0.5;
	
	ivec2 last_pix;
	last_pix.x = int(last_uv.x * width + 0.1);
	last_pix.y = int(last_uv.y * height + 0.1);
	
	if (last_pix.x >= 0 && last_pix.x < width && last_pix.y >= 0 && last_pix.y < height) 
	{
		vec4 last_result = imageLoad(color[(param.current_index + 1) % 2], last_pix);

		vec4 mu; 
		vec4 sigma;
		compute(mu, sigma);
		last_result = clamp(last_result, mu - sigma, mu + sigma);

		float now_id = imageLoad(id, ivec2(gl_FragCoord.xy)).x;
		float last_id  = imageLoad(id, last_pix).y;

		if (now_id == last_id)
		{
			result = mix(result, last_result, 0.0);
		}
	}
	
	// ´æ´¢½á¹û
	imageStore(color[param.current_index], ivec2(gl_FragCoord.xy), result);
	out_color = vec4(gammaCorrect(acesToneMapping(result.xyz)), 1.0);
}