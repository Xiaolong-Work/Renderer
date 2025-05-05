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

layout(binding = 9, rgba32f) uniform image2D position;
layout(binding = 10, rgba32f) uniform image2D normal;
layout(binding = 11, rgba32f) uniform image2D id;

 float pi = 3.14159265358979;

vec3 sampleGGX(float roughness, vec3 normal, inout uint seed) 
{
    float alpha = roughness * roughness;
    float rand1 = rnd(seed);
    float rand2 = clamp(rnd(seed), 1e-6, 1.0 - 1e-6);

    float phi = 2.0 * 3.14159265 * rand1;
    float cos_phi = cos(phi);
    float sin_phi = sin(phi);
    float cos_theta = sqrt((1.0 - rand2) / (1.0 + (alpha * alpha - 1.0) * rand2));
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);

    vec3 h_tangent = vec3(
        sin_theta * cos_phi,
        sin_theta * sin_phi,
        cos_theta
    );

    normal = normalize(normal);
    vec3 up = abs(normal.y) > 0.999 ? vec3(0, 0, 1) : vec3(0, 1, 0);
    vec3 tangent = normalize(cross(up, normal));
    vec3 bitangent = cross(normal, tangent);

    return normalize(
        tangent * h_tangent.x +
        bitangent * h_tangent.y +
        normal * h_tangent.z
    );
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

void main()
{  
	uint  ray_flags = gl_RayFlagsNoneEXT;
	float time_min     = 0.001;
	float time_max     = 10000.0;

	int material_index = material_indices[gl_InstanceCustomIndexEXT];
	Material material = materials[material_index];

	Vertex interpolation = getInterpolateVertex();

	vec3 object_position = vec3(gl_ObjectToWorldEXT * vec4(interpolation.position, 1.0));
	vec3 object_normal = normalize(vec3(interpolation.normal * gl_WorldToObjectEXT));
	/* Write geometry buffer */
	if (payload.depth == 0)
	{
		imageStore(position, ivec2(gl_LaunchIDEXT.xy), vec4(object_position, 1.0));
		imageStore(normal, ivec2(gl_LaunchIDEXT.xy), vec4(object_normal, 1.0));

		float last_id = imageLoad(id, ivec2(gl_LaunchIDEXT.xy)).x;
		imageStore(id, ivec2(gl_LaunchIDEXT.xy), vec4(gl_InstanceCustomIndexEXT, last_id, 0.0, 1.0));
	}

	ObjectProperty property = object_properties[gl_InstanceCustomIndexEXT];
	/* Rays hit the light source */ 
	if (property.is_light == 1)
	{
		payload.hit_value = property.radiance;
		payload.hit_light = true;
		return;
	}

	vec3 light_position;
	vec3 light_normal;
	vec3 light_radiance;
	float light_pdf;

	sampleLight(light_radiance, light_position, light_normal, light_pdf, payload.seed);

	/* Pointing from the shading point to the camera */
	vec3 wi = -normalize(gl_WorldRayDirectionEXT); 

	/* Pointing from the shading point to the light */
	vec3 wl = normalize(light_position - object_position);

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
		float cos_theta = max(dot(object_normal, wl), 0);
		float cos_theta_x = max(dot(light_normal, -wl), 0);
		result_color = light_radiance * brdf * cos_theta * cos_theta_x / (distance * distance * light_pdf);
	}
	
	vec3 wo;
	float pdf_O;
	if(material.type == Diffuse)
	{
		wo = diffuseSample(object_normal, payload.seed);
		pdf_O = 1.0 / (2 * pi);
	}
	else if (material.type == Specular)
	{
		wo = reflect(-wi, object_normal);
		pdf_O = 1.0;
	}
	else if (material.type == Refraction)
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
		
		float p = fresnelSchlickIor(wi, temp_normal, ior);
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
	else if (material.type == Glossy)
	{
		vec3 h = sampleGGX(material.roughness, object_normal, payload.seed);
		wo = reflect(-wi, h);
		pdf_O = distributionGGX(object_normal, h, material.roughness) * dot(object_normal, h) / (4.0 * max(dot(wo, object_normal), 1e-6));
	}
	
	payload.depth++;
	if (payload.depth >= 5) 
	{
		payload.hit_value = result_color;
		return;
	}

	traceRayEXT(tlas, ray_flags, 0xFF, 0, 0, 0, object_position, time_min, wo, time_max, 0);

	if (material.type == Diffuse || material.type == Glossy)
	{
		vec3 brdf = shaderPBR(wi, wo, material.roughness, material.metallic, color, object_normal);
		float cos_theta = max(dot(wo, object_normal), 0);	
		if (pdf_O > 1e-2)
		{
			vec3 last_result = payload.hit_value * brdf * cos_theta / pdf_O;
			float weight_direct = light_pdf * light_pdf;
			float weight_indirect = pdf_O * pdf_O;
			if (length(last_result) == 0.0)
			{
				weight_indirect = 0;
			}
			
			float weigth_sum = weight_direct + weight_indirect;
			weight_direct /= weigth_sum;
			weight_indirect /= weigth_sum;

			if (payload.hit_light && material.type == Diffuse)
			{
				weight_indirect = 0;
				weight_direct = 1;
				payload.hit_light = false;
			}

			payload.hit_value = weight_direct * result_color + weight_indirect * last_result;
		}
	}
	else if (material.type == Specular || material.type == Refraction)
	{ 
		payload.hit_value = payload.hit_value * 0.9;
	}
}

