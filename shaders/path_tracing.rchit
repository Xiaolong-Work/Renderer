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


//// 采样 GGX 微表面法线 h
//vec3 sampleGGX(float alpha, vec3 N, vec3 T, vec3 B, inout float seed)
//{
//	// 计算 theta 和 phi
//	float phi = 2.0 * PI * xi.x;
//	float cos_theta = sqrt((1.0 - xi.y) / (1.0 + (alpha * alpha - 1.0) * xi.y));
//	float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
//
//	// 构造 h 在局部坐标系
//	vec3 h_local = vec3(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);
//
//	// 转换到世界坐标系
//	return normalize(T * h_local.x + B * h_local.y + N * h_local.z);
//}
//
//vec3 sampleSpecularGGX(float roughness, vec3 N, vec3 T, vec3 B, vec3 wi, inout float seed) 
//{
//	float alpha = roughness * roughness;
//    vec3 h = sampleGGX(alpha, N, T, B);
//    return reflect(-wi, h);  // 注意：V 应指向表面
//}

//float computeGGXPDF(vec3 N, vec3 H, vec3 V, float alpha) 
//{
//    float NdotH = max(dot(N, H), 0.0);
//    float HdotV = max(dot(H, V), 0.0);
//    float NdotV = max(dot(N, V), 0.0);
//    
//    float D = ggxDistribution(NdotH, alpha); // GGX NDF
//    return (D * NdotH) / (4.0 * HdotV);
//}

vec3 sampleGGX(float roughness, vec3 normal, inout uint seed) 
{
    float alpha = roughness * roughness;
	float rand1 = rnd(seed);
	float rand2 = rnd(seed);

    float phi = 2.0 * 3.14159265 * rand1;
    float cos_theta = sqrt((1.0 - rand2) / (1.0 + (alpha * alpha - 1.0) * rand2));
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);

    // 微表面法线在切线空间下的坐标
    vec3 h_tangent = vec3(
        sin_theta * cos(phi),
        sin_theta * sin(phi),
        cos_theta
    );

    // 创建与 n 对齐的切线空间基底
    vec3 up = abs(normal.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
    vec3 tangent = normalize(cross(up, normal));
    vec3 bitangent = cross(normal, tangent);

    // 将切线空间中的方向转换到世界空间
    vec3 h = normalize(
        tangent * h_tangent.x +
        bitangent * h_tangent.y +
        normal * h_tangent.z
    );

    return h;
}

vec3 diffuseSample(vec3 normal, inout uint random_seed)
{
    /* Generate random numbers */
    float u = rnd(random_seed);
    float v = rnd(random_seed);

    /* Cosine-weighted solid angle distribution */
    float phi = 2 * 3.14159 * u;
    float cos_theta = sqrt(v);
    float sin_theta = sqrt(1 - v);
    float x = cos(phi) * sin_theta;
    float y = sin(phi) * sin_theta;
    float z = cos_theta;

    /* Calculate the local coordinate system */
    vec3 tangent, bitangent;
	if (abs(normal.z) > 0.999f)
	{
		tangent = vec3(1, 0, 0);
	}
	else
	{
		tangent = normalize(cross(normal, vec3(0, 0, 1)));
	}
	bitangent = cross(normal, tangent);

	/* Transform to world coordinates */
    return tangent * x + bitangent * y + normal * z;
}

vec3 glossySample(vec3 wi, vec3 normal, Material material, inout uint random_seed)
{
    /* Compute the reflection direction and ensure normalization */
    vec3 reflect_direction = normalize(reflect(wi, normal));

	float u = rnd(random_seed);
	float v = rnd(random_seed);

	float exponent = max(material.ns, 0.0f); // Ensure the exponent is non-negative
	float cos_theta = pow(max(u, 1e-6f), 1.0f / (exponent + 1));
	float sin_theta = sqrt(1 - cos_theta * cos_theta);
	float phi = 2.0f * 3.14159 * v;

	float x = sin_theta * cos(phi);
	float y = sin_theta * sin(phi);
	float z = cos_theta;

	// Construct an orthonormal basis around the reflection direction
	vec3 tangent = (abs(reflect_direction.z) > 0.999f) ? vec3(1, 0, 0)
															: // If too close to the Z-axis, use (1,0,0) as tangent
					   normalize(cross(reflect_direction, vec3(0, 1, 0)));
	vec3 bitangent = normalize(cross(tangent, reflect_direction));

	// Transform the sampled direction from local to world space and return
	return normalize(tangent * x + bitangent * y + reflect_direction * z);
}

void main()
{  
	uint  ray_flags = gl_RayFlagsNoneEXT;
	float time_min     = 0.0001;
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

	

	/* Pointing from the shading point to the camera */
	vec3 wi = -normalize(gl_WorldRayDirectionEXT); 

	/* Pointing from the shading point to the light */
	vec3 wl = normalize(light_position - object_position);

	/* Rays hit the light source */ 
	if (property.is_light == 1)
	{
		if (payload.depth == 0)
		{
			payload.hit_value = property.radiance;
		}
		else
		{
			payload.hit_value = vec3(0);
		}
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
	//vec3 wo = sampleGGXVNDF(object_normal, wi, material.roughness, rand1, rand2);
	//vec3 wo = glossySample(-wi, object_normal, material, payload.seed);
	//vec3 h = sampleGGX(material.roughness, object_normal, payload.seed);
	//vec3 wo = reflect(-wi, h);

	vec3 wo = diffuseSample(object_normal, payload.seed);

	float pdf_O = 1.0 / (3.14159 * 2);

	if (material.type == 1)
	{
		wo = reflect(-wi, object_normal);
		pdf_O = 1.0;
	}
	else if (material.type == 2)
	{
		float ior;
		vec3 temp_normal;
		if (payload.in_object)
		{
			temp_normal = -object_normal;
			ior = material.ni;
		}
		else
		{
			temp_normal = object_normal;
			ior = 1.0 / material.ni;
		}
		
		float p = fresnelSchlickIor(wi, wo, ior);
		if (rnd(payload.seed) < p)
		{
			wo = reflect(-wi, temp_normal);
			pdf_O = 1.0;
		}
		else
		{
			wo = refract(-wi, temp_normal, ior);
			pdf_O = 1.0 - p; 
			if (length(wo) == 0)
			{
				wo = reflect(-wi, temp_normal);
				pdf_O = 1;
			}
			
			payload.in_object = !payload.in_object;
		}
	}
	
		
	

	payload.depth++;
	if (payload.depth >= 7) 
	{
		payload.hit_value = result_color;
		return;
	}

	traceRayEXT(tlas, ray_flags, 0xFF, 0, 0, 0, object_position, time_min, wo, time_max, 0);

	if (material.type == 0 || material.type == 3)
	{
		vec3 brdf = shaderPBR(wi, wo, material.roughness, material.metallic, color, object_normal);
		
		float cos_theta = dot(wo, object_normal);	
		payload.hit_value = result_color + payload.hit_value * brdf * cos_theta / pdf_O;
	}
	else
	{
		payload.hit_value = payload.hit_value / pdf_O;
	}
	//payload.hit_value = color.xyz;
	//payload.hit_value = vec3(color * interpolation.color);
}

