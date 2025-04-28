#version 460
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

#include "path_tracing_vertex.glsl"
#include "pbr.glsl"

layout(set = 0, binding = 0) uniform accelerationStructureEXT tlas;

layout(binding = 3) uniform sampler2D textures[];



layout(location = 0) rayPayloadInEXT HitPayload payload;
layout(location = 1) rayPayloadEXT ShadowPayload shadow_payload;


void main()
{
	uint  ray_flags = gl_RayFlagsNoneEXT;
	float time_min     = 0.001;
	float time_max     = 10000.0;

	int material_index = material_indices[gl_InstanceCustomIndexEXT];
	Material material = materials[material_index];
	ObjectProperty property = object_properties[gl_InstanceCustomIndexEXT];

	Vertex interpolation = getInterpolateVertex();

	vec3 object_position = vec3(gl_ObjectToWorldEXT * vec4(interpolation.position, 1.0));
	vec3 object_normal = normalize(vec3(interpolation.normal * gl_WorldToObjectEXT));

	vec3 light_position;
	vec3 light_normal;
	vec3 light_radiance;
	float light_pdf;

	sampleLight(light_radiance, light_position, light_normal, light_pdf, payload.seed);
	light_radiance;

	/* Pointing from the shading point to the camera */
	vec3 wi = -normalize(gl_WorldRayDirectionEXT);

	/* Pointing from the shading point to the light */
	vec3 wl = normalize(light_position - object_position);

	/* Rays hit the light source */ 
	if (property.is_light == 1)
	{
		payload.hit_value = property.radiance;
		return;
	}

	/* Shadow ray test */
	traceRayEXT(tlas, ray_flags, 0xFF, 1, 0, 0, object_position, time_min, wl, time_max, 1);

	vec4 color = vec4(0.0f);
	if (material.albedo_texture != -1)
	{
		color = texture(textures[material.albedo_texture], interpolation.texture);
	}
	else
	{
		color = material.albedo;
	}

	vec3 result_color = vec3(0);
	if (shadow_payload.is_hit) 
	{
		float distance = length(light_position - object_position);

		vec3 brdf = shaderPBR(wi, wl, material.roughness, material.metallic, color, object_normal);
		float cos_theta = dot(object_normal, wl);
		float cos_theta_x = dot(light_normal, -wl);
		result_color = light_radiance * brdf * cos_theta * cos_theta_x / (distance * distance * light_pdf);
	}
	
	float rand1 = rnd(payload.seed);
	float rand2 = rnd(payload.seed);
	vec3 wo = sampleGGXVNDF(object_normal, -wi, material.roughness, rand1, rand2);

	payload.depth++;
	if (payload.depth >= 5)
	{
		payload.hit_value = result_color;
		return;
	}

	traceRayEXT(tlas, ray_flags, 0xFF, 0, 0, 0, object_position, time_min, wo, time_max, 0);

	vec3 brdf = shaderPBR(wi, wo, material.roughness, material.metallic, color, object_normal);
	float pdf_O = 1.0 / 3.14159 * 2;
	float cos_theta = dot(wo, object_normal);	
	payload.hit_value = result_color + payload.hit_value * brdf * cos_theta / pdf_O;
	//payload.hit_value = vec3(color * interpolation.color);
}

//void main()
//{
//    // 物体数据
//    ObjDesc    objResource = objDesc.i[gl_InstanceCustomIndexEXT];
//    MatIndices matIndices  = MatIndices(objResource.materialIndexAddress);
//    Materials  materials   = Materials(objResource.materialAddress);
//    Indices    indices     = Indices(objResource.indexAddress);
//    Vertices   vertices    = Vertices(objResource.vertexAddress);
//  
//    // 三角形的索引
//    ivec3 ind = indices.i[gl_PrimitiveID];
//  
//    // 三角形的顶点
//    Vertex v0 = vertices.v[ind.x];
//    Vertex v1 = vertices.v[ind.y];
//    Vertex v2 = vertices.v[ind.z];
//
//	 const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
//	 // 计算命中位置的坐标
//	const vec3 pos      = v0.pos * barycentrics.x + v1.pos * barycentrics.y + v2.pos * barycentrics.z;
//	const vec3 worldPos = vec3(gl_ObjectToWorldEXT * vec4(pos, 1.0));  // 将位置转换到世界空间
//
//	// 计算命中位置的法线
//	const vec3 nrm      = v0.nrm * barycentrics.x + v1.nrm * barycentrics.y + v2.nrm * barycentrics.z;
//	const vec3 worldNrm = normalize(vec3(nrm * gl_WorldToObjectEXT));  // 将法线转换到世界空间
//
//	 // 指向光源的向量
//	vec3  L;
//	float lightIntensity = pcRay.lightIntensity;
//	float lightDistance  = 100000.0;
//	// 点光源
//	if(pcRay.lightType == 0)
//	{
//	  vec3 lDir      = pcRay.lightPosition - worldPos;
//	  lightDistance  = length(lDir);
//	  lightIntensity = pcRay.lightIntensity / (lightDistance * lightDistance);
//	  L              = normalize(lDir);
//	}
//	else  // 方向光
//	{
//	  L = normalize(pcRay.lightPosition);
//	}
//}

