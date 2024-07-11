#pragma once
#include <algorithm>
#include <scene.h>

void Scene::setData(std::vector<Object>& objects, const Camera& camera)
{
	this->camera = camera;
	for (auto& object : objects)
	{
		PathTracingObject temp{object};
		this->objects.push_back(temp);
	}
}
void Scene::initBVH()
{
	float area = 0.0f;
	std::vector<int> object_index;
	BoundingBox bounding_box;
	for (int i = 0; i < this->objects.size(); i++)
	{
		this->objects[i].initBVH();

		area += objects[i].area;
		object_index.push_back(i);
		bounding_box.unionBox(objects[i].bounding_box);

		if (objects[i].is_light)
		{
			this->light_object_index.push_back(i);
		}
	}

	SceneBVH node;
	this->bvh.clear();
	this->bvh.push_back(node);
	this->bvh[0].area = area;

	this->buildBVH(0, object_index, bounding_box);
}

void Scene::buildBVH(const int index, std::vector<int>& object_index, const BoundingBox& bounding_box)
{
	this->bvh[index].bounding_box = bounding_box;

	if (object_index.size() == 1)
	{
		this->bvh[index].object_index = object_index[0];
		this->bvh[index].leaf_node_flag = true;
		this->bvh[index].area = this->objects[object_index[0]].area;
		return;
	}
	else
	{
		this->bvh[index].leaf_node_flag = false;
	}

	SceneBVH left_node;
	this->bvh.push_back(left_node);
	this->bvh[index].left = int(this->bvh.size()) - 1;

	SceneBVH right_node;
	this->bvh.push_back(right_node);
	this->bvh[index].right = int(this->bvh.size()) - 1;

	if (object_index.size() == 2)
	{
		std::vector<int> temp;
		temp.push_back(object_index[0]);
		buildBVH(this->bvh[index].left, temp, this->objects[object_index[0]].bounding_box);

		temp.clear();
		temp.push_back(object_index[1]);
		buildBVH(this->bvh[index].right, temp, this->objects[object_index[1]].bounding_box);
		return;
	}

	float x_length = bounding_box.getLengthX();
	float y_length = bounding_box.getLengthY();
	float z_length = bounding_box.getLengthZ();

	if (x_length > y_length && x_length > z_length)
	{
		std::sort(object_index.begin(), object_index.end(), [&](const int& a, const int& b) {
			return this->objects[a].bounding_box.getMin().x < this->objects[b].bounding_box.getMin().x;
		});
	}
	else if (y_length > x_length && y_length > z_length)
	{
		std::sort(object_index.begin(), object_index.end(), [&](const int& a, const int& b) {
			return this->objects[a].bounding_box.getMin().y < this->objects[b].bounding_box.getMin().y;
		});
	}
	else
	{
		std::sort(object_index.begin(), object_index.end(), [&](const int& a, const int& b) {
			return this->objects[a].bounding_box.getMin().z < this->objects[b].bounding_box.getMin().z;
		});
	}

	int middle = object_index.size() / 2;
	std::vector<int> left_objects_index(object_index.begin(), object_index.begin() + middle);
	std::vector<int> right_objects_index(object_index.begin() + middle, object_index.end());

	BoundingBox left_bounding_box{}, right_bounding_box{};
	float left_area = 0.0f, right_area = 0.0f;
	for (auto& index : left_objects_index)
	{
		left_bounding_box.unionBox(this->objects[index].bounding_box);
		left_area += this->objects[index].area;
	}
	for (auto& index : right_objects_index)
	{
		right_bounding_box.unionBox(this->objects[index].bounding_box);
		right_area += this->objects[index].area;
	}

	this->bvh[this->bvh[index].left].area = left_area;
	this->bvh[this->bvh[index].right].area = right_area;

	buildBVH(this->bvh[index].left, left_objects_index, left_bounding_box);
	buildBVH(this->bvh[index].right, right_objects_index, right_bounding_box);

	return;
}

IntersectResult Scene::intersect(Ray& ray) const
{
	return this->traverse(0, ray);
}

IntersectResult Scene::traverse(const int index, Ray& ray) const
{
	IntersectResult result;
	auto& root = this->bvh[index];
	auto bounding_box = root.bounding_box;

	if (ray.intersectBoundingBox(bounding_box))
	{
		if (!root.leaf_node_flag)
		{
			auto temp_result1 = this->traverse(root.left, ray);
			auto temp_result2 = this->traverse(root.right, ray);
			result = temp_result1.t < temp_result2.t ? temp_result1 : temp_result2;
		}
		else
		{
			if (this->objects[root.object_index].material.name == "Blinds")
			{
				return result;
			}
			IntersectResult temp_result = this->objects[root.object_index].intersect(ray);
			if (temp_result.is_intersect)
			{
				if (temp_result.t < result.t)
				{
					result = temp_result;
					result.object_index = root.object_index;
				}
			}
		}
	}

	return result;
}

void Scene::sampleLight(IntersectResult& result, float& pdf)
{
	float emit_area_sum = 0;
	for (int i = 0; i < this->light_object_index.size(); i++)
	{
		emit_area_sum += objects[light_object_index[i]].area;
	}

	float p = getRandomNumber(0.0f, 1.0f) * emit_area_sum;

	emit_area_sum = 0;
	for (int i = 0; i < light_object_index.size(); i++)
	{
		emit_area_sum += objects[light_object_index[i]].area;
		if (p <= emit_area_sum)
		{
			objects[light_object_index[i]].sample(result, pdf);
			result.object_index = light_object_index[i];
			break;
		}
	}
}

void Scene::setEnvironmentLight(const Vector3f& Ka)
{
	this->Ka = Ka;
}

Vector3f Scene::shader(Ray ray)
{
	std::vector<std::pair<Vector3f, Vector3f>> task{};
	int depth = 0;

	while (true)
	{
		Vector3f result_color{0.0f, 0.0f, 0.0f};

		/* Intersection of light and scene */
		auto result = this->intersect(ray);

		/* No intersection */
		if (!result.is_intersect)
		{
			task.push_back(std::make_pair(this->Ka, Vector3f(0.0f, 0.0f, 0.0f)));
			break;
		}
		auto& object = objects[result.object_index];

		/* The intersection is a light source */
		if (object.is_light)
		{
			if (depth == 0)
			{
				return object.radiance;
			}
			task.push_back(std::make_pair(this->Ka, Vector3f(0.0f, 0.0f, 0.0f)));
			break;
		}

		/* The object being intersected is a normal object */
		Point object_point = result.point;
		Direction object_normal = result.normal;
		Direction wo = result.ray.direction;
		Vector3f color = result.color;
		PathTracingMaterial material = object.material;

		/* Sample the light source */
		float pdf;
		IntersectResult light;
		this->sampleLight(light, pdf);
		Point light_point = light.point;
		Vector3f ws = glm::normalize(light_point - object_point);
		Direction light_point_normal = light.normal;
		Vector3f light_radiance = this->objects[light.object_index].radiance;

		/* Check if it is blocked */
		float distance = glm::length(light_point - object_point);
		Ray object_to_light{object_point, ws};
		auto test = this->intersect(object_to_light);

		/* No occlusion */
		if (test.t - distance > -0.001)
		{
			Vector3f evaluate = material.evaluate(wo, ws, object_normal, color);
			float cos_theta = glm::dot(object_normal, ws);
			float cos_theta_x = glm::dot(light_point_normal, -ws);
			result_color =
				light_radiance * evaluate * cos_theta * cos_theta_x / (float(std::pow(distance, 2.0f)) * pdf);
		}

		/* Sampling light */
		Vector3f wi = glm::normalize(material.sample(wo, object_normal));
		ray.origin = object_point;
		ray.direction = wi;
		ray.t = std::numeric_limits<float>::infinity();

		/* Specular reflection */
		if (material.type == MaterialType::Specular)
		{
			depth++;
			continue;
		}
		else
		{
			Vector3f evaluate = material.evaluate(wo, wi, object_normal, color);
			float pdf_O = material.pdf(wo, wi, object_normal);
			float cos_theta = glm::dot(wi, object_normal);
			task.push_back(std::make_pair(result_color, evaluate * cos_theta / pdf_O));
		}

		depth++;
		if (depth > this->max_depth)
		{
			task.push_back(std::make_pair(Vector3f(0.0f), Vector3f(0.0f)));
			break;
		}
	}

	Vector3f color{0.0f, 0.0f, 0.0f};

	auto& [temp, coefficient] = task.back();
	color = temp * coefficient;
	task.pop_back();

	while (!task.empty())
	{
		auto& [temp, coefficient] = task.back();
		color = temp + color * coefficient;
		task.pop_back();
	}

	return color;
}
