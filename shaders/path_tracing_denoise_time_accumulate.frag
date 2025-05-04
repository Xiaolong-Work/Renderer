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
};
layout(push_constant) uniform PushConstant { DenoisePushConstantData param; };

void main()
{
	out_color = imageLoad(denoise, ivec2(gl_FragCoord.xy));
	//return;

	/* Frame count is greater than 10, camera does not move, accumulate directly */
	if (param.frame_count >= 10)
	{
		float a = 1.0f / float(param.frame_count + 1);
		vec4 old_color = imageLoad(color[(param.current_index + 1) % 2], ivec2(gl_FragCoord.xy));
		vec4 new_color = imageLoad(color[param.current_index], ivec2(gl_FragCoord.xy));
		out_color = mix(old_color, new_color, a);
		imageStore(color[param.current_index], ivec2(gl_FragCoord.xy), out_color);
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
	
	// 将纹理坐标转换为像素坐标
	ivec2 last_pix;
	last_pix.x = int(last_uv.x * width + 0.1);
	last_pix.y = int(last_uv.y * height + 0.1);
	
	// 检查坐标是否在有效范围内
	if (last_pix.x >= 0 && last_pix.x < width && last_pix.y >= 0 && last_pix.y < height) 
	{
		// 获取上一帧对应位置的颜色
		vec4 last_result = imageLoad(color[(param.current_index + 1) % 2], last_pix);

		float now_id = imageLoad(id, ivec2(gl_FragCoord.xy)).x;
		float last_id  = imageLoad(id, last_pix).y;

		if (now_id == last_id)
		{
			result = mix(result, last_result, 0.9);
		}
	}
	
	// 存储结果
	imageStore(color[param.current_index], ivec2(gl_FragCoord.xy), result);
	out_color = result;
}