#include <path_tracing_object.h>

bool sortByX(const Triangle& triangle1, const Triangle& triangle2)
{
	return triangle1.bounding_box.getMin().x < triangle2.bounding_box.getMin().x;
}

bool sortByY(const Triangle& triangle1, const Triangle& triangle2)
{
	return triangle1.bounding_box.getMin().y < triangle2.bounding_box.getMin().y;
}

bool sortByZ(const Triangle& triangle1, const Triangle& triangle2)
{
	return triangle1.bounding_box.getMin().z < triangle2.bounding_box.getMin().z;
}

PathTracingObject::PathTracingObject(Object& object)
{
	this->name = std::move(object.name);
	this->mesh = std::move(object.mesh);

	this->material = object.material;
	this->bounding_box = std::move(object.bounding_box);
	this->area = std::move(object.area);
	this->is_light = std::move(object.is_light);
	this->radiance = std::move(object.radiance);
}

void PathTracingObject::initBVH()
{
	BVH bvh_node;
	this->bvh.clear();
	this->bvh.push_back(bvh_node);
	this->bvh[0].area = this->area;
	this->buildBVH(0, this->mesh, this->bounding_box);
}

void PathTracingObject::buildBVH(const int index, std::vector<Triangle>& triangles, const BoundingBox& bounding_box)
{
	this->bvh[index].bounding_box = bounding_box;

	if (triangles.size() == 1)
	{
		this->bvh[index].triangle = triangles[0];
		this->bvh[index].leaf_node_flag = true;
		this->bvh[index].area = triangles[0].area;
		return;
	}
	else
	{
		this->bvh[index].leaf_node_flag = false;
	}

	BVH left_node;
	this->bvh.push_back(left_node);
	this->bvh[index].left = int(this->bvh.size()) - 1;

	BVH right_node;
	this->bvh.push_back(right_node);
	this->bvh[index].right = int(this->bvh.size()) - 1;

	if (triangles.size() == 2)
	{
		std::vector<Triangle> temp;
		temp.push_back(triangles[0]);
		buildBVH(this->bvh[index].left, temp, triangles[0].getBoundingBox());

		temp.clear();
		temp.push_back(triangles[1]);
		buildBVH(this->bvh[index].right, temp, triangles[1].getBoundingBox());
		return;
	}

	float x_length = bounding_box.getLengthX();
	float y_length = bounding_box.getLengthY();
	float z_length = bounding_box.getLengthZ();

	if (x_length > y_length && x_length > z_length)
	{
		std::sort(triangles.begin(), triangles.end(), sortByX);
	}
	else if (y_length > x_length && y_length > z_length)
	{
		std::sort(triangles.begin(), triangles.end(), sortByY);
	}
	else
	{
		std::sort(triangles.begin(), triangles.end(), sortByZ);
	}

	int middle = triangles.size() / 2;
	std::vector<Triangle> left_triangles(triangles.begin(), triangles.begin() + middle);
	std::vector<Triangle> right_triangles(triangles.begin() + middle, triangles.end());

	BoundingBox left_bounding_box{}, right_bounding_box{};
	float left_area = 0.0f, right_area = 0.0f;
	for (auto& triangle : left_triangles)
	{
		left_bounding_box.unionBox(triangle.getBoundingBox());
		left_area += triangle.area;
	}
	for (auto& triangle : right_triangles)
	{
		right_bounding_box.unionBox(triangle.getBoundingBox());
		right_area += triangle.area;
	}

	this->bvh[this->bvh[index].left].area = left_area;
	this->bvh[this->bvh[index].right].area = right_area;

	buildBVH(this->bvh[index].left, left_triangles, left_bounding_box);
	buildBVH(this->bvh[index].right, right_triangles, right_bounding_box);

	return;
}

IntersectResult PathTracingObject::intersect(Ray& ray) const
{
	return this->traverse(0, ray);
}

IntersectResult PathTracingObject::traverse(const int index, Ray& ray) const
{
	auto& root = this->bvh[index];
	auto bounding_box = root.bounding_box;
	IntersectResult intersect_result;
	if (ray.intersectBoundingBox(bounding_box))
	{
		if (!root.leaf_node_flag)
		{
			auto temp_result1 = this->traverse(root.left, ray);
			auto temp_result2 = this->traverse(root.right, ray);
			intersect_result = temp_result1.t < temp_result2.t ? temp_result1 : temp_result2;
		}
		else
		{
			auto triangle = root.triangle;
			float result[4];
			if (ray.intersectTriangle(triangle, result))
			{
				intersect_result.is_intersect = true;
				auto b1 = result[0];
				auto b2 = result[1];
				auto b3 = result[2];

				if (this->material.texture_load_flag)
				{
					Coordinate2D coordinate =
						triangle.vertex1.texture * b1 + triangle.vertex2.texture * b2 + triangle.vertex3.texture * b3;
					intersect_result.color = this->material.texture.getColor(coordinate.x, coordinate.y);
				}

				intersect_result.normal =
					triangle.vertex1.normal * b1 + triangle.vertex2.normal * b2 + triangle.vertex3.normal * b3;

				intersect_result.t = result[3];
				ray.t = std::min(intersect_result.t, ray.t);
				intersect_result.point = ray.spread(intersect_result.t);
				intersect_result.ray = ray;
			}
		}
	}

	return intersect_result;
}

void PathTracingObject::sample(IntersectResult& result, float& pdf)
{
	auto& root = this->bvh[0];
	float p = getRandomNumber(0.0f, 1.0f) * root.area;
	this->traverse(0, p, result, pdf);
	pdf /= root.area;
	return;
}

void PathTracingObject::traverse(const int index, float p, IntersectResult& result, float& pdf)
{
	auto& root = this->bvh[index];
	if (root.leaf_node_flag)
	{
		Point sample_point;

		root.triangle.sample(sample_point, pdf);

		result.point = sample_point;
		result.normal = root.triangle.normal;
		pdf *= root.area;

		return;
	}

	if (p < this->bvh[root.left].area)
	{
		traverse(root.left, p, result, pdf);
	}
	else
	{
		traverse(root.right, p - this->bvh[root.left].area, result, pdf);
	}
}
