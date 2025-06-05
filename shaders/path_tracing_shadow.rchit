#version 460
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

#include "path_tracing.glsl"

layout(location = 1) rayPayloadInEXT ShadowPayload payload;

layout(std430, binding = 5) readonly buffer ObjectPropertyBuffer
{
	ObjectProperty object_properties[];
};

void main()
{
	ObjectProperty property = object_properties[gl_InstanceCustomIndexEXT];
	if (property.is_light == 0)
	{
		payload.is_hit = false;
	}
	else
	{
		payload.is_hit = true;
	}
}
