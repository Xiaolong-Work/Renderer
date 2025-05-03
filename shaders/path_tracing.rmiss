#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable

#include "path_tracing.glsl"

layout(location = 0) rayPayloadInEXT HitPayload payload;

layout(binding = 9, rgba32f) uniform image2D position;
layout(binding = 10, rgba32f) uniform image2D normal_depth;
layout(binding = 11, rgba32f) uniform image2D albedo;
layout(binding = 12, rgba32f) uniform image2D material_ssao;

void main()
{
	payload.hit_value = vec3(0.0f);
	/* Write geometry buffer */
	if (payload.depth == 0)
	{
		float id = imageLoad(albedo, ivec2(gl_LaunchIDEXT.xy)).x;
		imageStore(albedo, ivec2(gl_LaunchIDEXT.xy), vec4(-1, id, 0.0, 0.0));
		imageStore(position, ivec2(gl_LaunchIDEXT.xy), vec4(0));
		imageStore(normal_depth, ivec2(gl_LaunchIDEXT.xy), vec4(0));
	}
}