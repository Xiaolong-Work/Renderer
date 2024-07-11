#include <ray.h>

Ray::Ray(const Point& point, const Direction& direction)
{
	this->origin = point;
	this->direction = direction;
}

Point Ray::spread(const float t) const
{
	return this->origin + this->direction * t;
}

bool Ray::intersectTriangle(const Triangle& triangle, float result[4]) const
{
	/* Moller Trumbore algorithm */
	if (glm::dot(this->direction, triangle.normal) > 0.0f)
	{
		return false;
	}

	Vector3f s = this->origin - triangle.vertex1.position;
	Vector3f s1 = glm::cross(this->direction, triangle.edge2);
	Vector3f s2 = glm::cross(s, triangle.edge1);
	float s1_dot_e1 = glm::dot(s1, triangle.edge1);

	float t = glm::dot(s2, triangle.edge2) / s1_dot_e1;
	float b1 = glm::dot(s1, s) / s1_dot_e1;
	float b2 = glm::dot(s2, this->direction) / s1_dot_e1;
	float b0 = 1 - b1 - b2;
	if (t < 0)
	{
		return false;
	}
	if (b1 < 0 || b2 < 0 || b0 < 0)
	{
		return false;
	}
	if (b1 > 1 || b2 > 1 || b0 > 1)
	{
		return false;
	}
	result[0] = b0;
	result[1] = b1;
	result[2] = b2;
	result[3] = t;

	return true;
}

bool Ray::intersectBoundingBox(const BoundingBox& box) const
{
	if (box.overlaps(this->origin))
	{
		return true;
	}

	float t_enter = -std::numeric_limits<float>::infinity();
	float t_exit = std::numeric_limits<float>::infinity();

	float t_min, t_max;

	t_min = (box.x_min - this->origin.x) / this->direction.x;
	t_max = (box.x_max - this->origin.x) / this->direction.x;
	if (this->direction.x < 0.0f)
	{
		std::swap(t_min, t_max);
	}
	t_enter = std::max(t_min, t_enter);
	t_exit = std::min(t_max, t_exit);

	t_min = (box.y_min - this->origin.y) / this->direction.y;
	t_max = (box.y_max - this->origin.y) / this->direction.y;
	if (this->direction.y < 0.0f)
	{
		std::swap(t_min, t_max);
	}
	t_enter = std::max(t_min, t_enter);
	t_exit = std::min(t_max, t_exit);

	t_min = (box.z_min - this->origin.z) / this->direction.z;
	t_max = (box.z_max - this->origin.z) / this->direction.z;
	if (this->direction.z < 0.0f)
	{
		std::swap(t_min, t_max);
	}
	t_enter = std::max(t_min, t_enter);
	t_exit = std::min(t_max, t_exit);

	if (t_enter <= t_exit && t_enter >= 0.0f && this->t > t_enter)
	{
		return true;
	}
	else
	{
		return false;
	}
}