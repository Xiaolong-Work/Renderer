#include <triangle.h>

Triangle::Triangle(const Vertex& vertex1, const Vertex& vertex2, const Vertex& vertex3)
{
	this->vertex1 = vertex1;
	this->vertex2 = vertex2;
	this->vertex3 = vertex3;

	this->edge1 = vertex2.position - vertex1.position;
	this->edge2 = vertex3.position - vertex1.position;
	this->edge3 = vertex3.position - vertex2.position;

	auto average_normal = (vertex1.normal + vertex2.normal + vertex3.normal) / 3.0f;
	this->normal = glm::normalize(glm::cross(this->edge1, this->edge2));
	if (glm::dot(this->normal, average_normal) < 0.0f)
	{
		this->normal *= -1;
	}

	this->area = glm::length(glm::cross(this->edge1, this->edge2)) * 0.5f;

	this->bounding_box = BoundingBox{vertex1.position};
	this->bounding_box.unionPoint(vertex2.position);
	this->bounding_box.unionPoint(vertex3.position);
}

BoundingBox Triangle::getBoundingBox() const
{
	return this->bounding_box;
}

void Triangle::sample(Point& point, float& pdf) const
{
	float x = std::sqrt(getRandomNumber(0.0f, 1.0f));
	float y = getRandomNumber(0.0f, 1.0f);
	point = this->vertex1.position * (1.0f - x) + this->vertex2.position * (x * (1.0f - y)) +
			this->vertex3.position * (x * y);
	pdf = 1.0f / this->area;
}
