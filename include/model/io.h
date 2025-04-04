#pragma once

#include <iostream>
#include <vector>

#include <tiny_obj_loader.h>

#include <vertex.h>

class Object
{
public:
	std::string name;
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};

class InputOutput
{
public:
	void loadObj(const std::string& obj_path)
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, error;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &error, obj_path.c_str()))
		{
			throw std::runtime_error(warn + error);
		}

		this->objects.resize(shapes.size());

		for (size_t i = 0; i < shapes.size(); i++)
		{
			auto& shape = shapes[i];

			this->objects[i].name = shape.name;

			std::map<std::tuple<int, int, int>, uint32_t> deduplication_index;
			deduplication_index.clear();

			Vertex vertex{};
			for (const auto& index : shape.mesh.indices)
			{
				if (index.vertex_index != -1)
				{
					vertex.position = {attrib.vertices[3 * index.vertex_index + 0],
									   attrib.vertices[3 * index.vertex_index + 1],
									   attrib.vertices[3 * index.vertex_index + 2]};
				}

				if (index.normal_index != -1)
				{
					vertex.normal = {attrib.normals[3 * index.normal_index + 0],
									 attrib.normals[3 * index.normal_index + 1],
									 attrib.normals[3 * index.normal_index + 2]};
				}

				if (index.texcoord_index != -1)
				{
					vertex.texture = {attrib.texcoords[2 * index.texcoord_index + 0],
									  1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};
				}

				auto key = std::make_tuple(index.vertex_index, index.normal_index, index.texcoord_index);
				if (deduplication_index.find(key) != deduplication_index.end())
				{
					this->objects[i].indices.push_back(deduplication_index[key]);
				}
				else
				{
					this->objects[i].vertices.push_back(vertex);
					this->objects[i].indices.push_back(this->objects[i].vertices.size() - 1);
					deduplication_index[key] = static_cast<uint32_t>(this->objects[i].vertices.size() - 1);
				}
			}
		}
	}

private:
	std::vector<Object> objects;
};
