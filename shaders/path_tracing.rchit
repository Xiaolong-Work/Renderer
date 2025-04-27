#version 460
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

#include "path_tracing.glsl"

layout(location = 0) rayPayloadInEXT HitPayload payload;

layout(binding = 3) uniform sampler2D textures[]; 
layout(std430, binding = 4) readonly buffer ObjectAddressBuffer { ObjectAddress object_address[]; };
layout(std430, binding = 5) readonly buffer MaterialIndexBuffer { int material_indices[]; };
layout(std430, binding = 6) readonly buffer MaterialBuffer { Material materials[]; };

layout(buffer_reference, std430) buffer Indices { int data[]; }; 
layout(buffer_reference, std430) buffer Vertices { float data[]; }; 
hitAttributeEXT vec2 attributes;

Vertex unpackVertex(Vertices vertices, int start_index)
{
    Vertex vertex;
    vertex.position = vec3(vertices.data[start_index + 0], vertices.data[start_index + 1], vertices.data[start_index + 2]);
    vertex.normal = vec3(vertices.data[start_index + 3], vertices.data[start_index + 4], vertices.data[start_index + 5]);
    vertex.texture = vec2(vertices.data[start_index + 6], vertices.data[start_index + 7]);
    vertex.color = vec4(vertices.data[start_index + 8], vertices.data[start_index + 9], vertices.data[start_index + 10], vertices.data[start_index + 11]);
    return vertex;
}

Vertex interpolate(Vertex vertex1, Vertex vertex2, Vertex vertex3, vec3 barycentrics)
{
    float a = barycentrics.x;
    float b = barycentrics.y;
    float c = barycentrics.z;

    Vertex result;
    result.position = vertex1.position * a + vertex2.position * b + vertex3.position * c;
    result.normal = normalize(vertex1.normal * a + vertex2.normal * b + vertex3.normal * c);
    result.texture = vertex1.texture * a + vertex2.texture * b + vertex3.texture * c;
    result.color = vertex1.color * a + vertex2.color * b + vertex3.color * c;
    
    return result;
}

void main()
{
	int material_index = material_indices[gl_InstanceCustomIndexEXT];
	Material material = materials[material_index];

	ObjectAddress address = object_address[gl_InstanceCustomIndexEXT];
	Indices indices = Indices(address.index_address);
	Vertices vertices = Vertices(address.vertex_address);

	int index1 = indices.data[gl_PrimitiveID * 3 + 0];
	int index2 = indices.data[gl_PrimitiveID * 3 + 1];
	int index3 = indices.data[gl_PrimitiveID * 3 + 2];

	Vertex vertex1 = unpackVertex(vertices, index1 * 12);
	Vertex vertex2 = unpackVertex(vertices, index2 * 12);
	Vertex vertex3 = unpackVertex(vertices, index3 * 12);

	vec3 barycentrics = vec3(1.0 - attributes.x - attributes.y, attributes.x, attributes.y);
	Vertex interpolation = interpolate(vertex1, vertex2, vertex3, barycentrics);

	vec3 world_position = vec3(gl_ObjectToWorldEXT * vec4(interpolation.position, 1.0));
	vec3 world_normal = normalize(vec3(interpolation.normal * gl_WorldToObjectEXT));

	vec4 color = vec4(0.0f);
	if (material.albedo_texture != -1)
	{
		color = texture(textures[material.albedo_texture], interpolation.texture);
	}
	else if (material.diffuse_texture != -1)
	{
		color = texture(textures[material.diffuse_texture], interpolation.texture);
	}
	else
	{
		color = material.albedo;
	}
	payload.hit_value = vec3(color);
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

