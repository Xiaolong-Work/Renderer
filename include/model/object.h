#pragma once

#include <string>
#include <vector>

#include <material.h>
#include <triangle.h>

/**
 * @class Object
 * @brief Represents a geometric object in the scene.
 */
class Object
{
public:
	/* The name of the object. */
	std::string name;

	/* Object's vertex data */
	std::vector<Vertex> vertices;

	/* Object's vertex index data */
	std::vector<Index> indices;

	/* The material index of the object */
	Index material_index{-1};

	Matrix4f model{1};

	/* ========== Path Tracing Data Extension ========== */
	/* The triangle mesh representing the object's geometry. */
	std::vector<Triangle> mesh;

	/* The material of the object. */
	Material material;

	/* The bounding box of the object. */
	BoundingBox bounding_box{};

	/* The total surface area of the object. */
	float area = 0.0f;

	/* Flag indicating whether the object is a light source. */
	bool is_light{false};

	/* The emitted radiance of the object if it is a light source. */
	Vector3f radiance{0.0f, 0.0f, 0.0f};

	void getTriangleData()
	{
		for (size_t i = 0; i < this->indices.size(); i += 3)
		{
			auto& vertex1 = this->vertices[this->indices[i + 0]];
			auto& vertex2 = this->vertices[this->indices[i + 1]];
			auto& vertex3 = this->vertices[this->indices[i + 2]];

			Triangle triangle(vertex1, vertex2, vertex3);
			this->mesh.push_back(triangle);
			this->bounding_box.unionBox(triangle.bounding_box);
			this->area += triangle.area;
		}
	}

	int triangle_count{0};
	void getArea()
	{
		for (size_t i = 0; i < this->indices.size(); i += 3)
		{
			auto& vertex1 = this->vertices[this->indices[i + 0]];
			auto& vertex2 = this->vertices[this->indices[i + 1]];
			auto& vertex3 = this->vertices[this->indices[i + 2]];

			Triangle triangle(vertex1, vertex2, vertex3);
			this->area += triangle.area;
			triangle_count++;
		}
	}
};