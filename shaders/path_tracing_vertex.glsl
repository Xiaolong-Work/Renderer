#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

#include "path_tracing.glsl"
#include "temp.h"

layout(std430, binding = 4) readonly buffer ObjectAddressBuffer
{
	ObjectAddress object_address[];
};

layout(std430, binding = 5) readonly buffer ObjectPropertyBuffer
{
	ObjectProperty object_properties[];
};

layout(buffer_reference, std430) buffer Indices
{
	int data[];
};

layout(buffer_reference, std430) buffer Vertices
{
	float data[];
};

layout(std430, binding = 8) readonly buffer LightObjectIndex
{
	int light_object_index[];
};

hitAttributeEXT vec2 attributes;

Vertex unpackVertex(Vertices vertices, int start_index)
{
	Vertex vertex;
	vertex.position =
		vec3(vertices.data[start_index + 0], vertices.data[start_index + 1], vertices.data[start_index + 2]);
	vertex.normal =
		vec3(vertices.data[start_index + 3], vertices.data[start_index + 4], vertices.data[start_index + 5]);
	vertex.texture = vec2(vertices.data[start_index + 6], vertices.data[start_index + 7]);
	vertex.color = vec4(vertices.data[start_index + 8],
						vertices.data[start_index + 9],
						vertices.data[start_index + 10],
						vertices.data[start_index + 11]);
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

Vertex getInterpolateVertex()
{
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
	return interpolate(vertex1, vertex2, vertex3, barycentrics);
}

void sampleTriangle(
	Vertex vertex1, Vertex vertex2, Vertex vertex3, inout vec3 position, inout vec3 normal, inout uint random_seed)
{
	float x = sqrt(rnd(random_seed));
	float y = rnd(random_seed);
	position = vertex1.position * (1.0f - x) + vertex2.position * (x * (1.0f - y)) + vertex3.position * (x * y);
	normal = vertex1.normal * (1.0f - x) + vertex2.normal * (x * (1.0f - y)) + vertex3.normal * (x * y);
}

void sampleObject(int index, inout vec3 position, inout vec3 normal, inout uint random_seed)
{
	float p = rnd(random_seed) * object_properties[index].triangle_count;

	int triangle_index = int(p);

	ObjectAddress address = object_address[index];
	Indices indices = Indices(address.index_address);
	Vertices vertices = Vertices(address.vertex_address);

	int index1 = indices.data[triangle_index * 3 + 0];
	int index2 = indices.data[triangle_index * 3 + 1];
	int index3 = indices.data[triangle_index * 3 + 2];

	Vertex vertex1 = unpackVertex(vertices, index1 * 12);
	Vertex vertex2 = unpackVertex(vertices, index2 * 12);
	Vertex vertex3 = unpackVertex(vertices, index3 * 12);

	sampleTriangle(vertex1, vertex2, vertex3, position, normal, random_seed);
}

void sampleLight(inout vec3 radiance, inout vec3 position, inout vec3 normal, inout float pdf, inout uint random_seed)
{
	float emit_area_sum = 0;
	for (int i = 0; i < light_object_index.length(); i++)
	{
		emit_area_sum += object_properties[light_object_index[i]].area;
	}

	float p = rnd(random_seed) * emit_area_sum;

	emit_area_sum = 0;
	for (int i = 0; i < light_object_index.length(); i++)
	{
		emit_area_sum += object_properties[light_object_index[i]].area;
		if (p <= emit_area_sum)
		{
			sampleObject(light_object_index[i], position, normal, random_seed);
			pdf = 1.0 / object_properties[light_object_index[i]].area;
			radiance = object_properties[light_object_index[i]].radiance;
			break;
		}
	}
}
