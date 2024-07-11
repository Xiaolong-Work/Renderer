#include <vertex.h>

float Vertex::u() const
{
	return texture.x;
}

float Vertex::v() const
{
	return texture.y;
}

float Vertex::x() const
{
	return position.x;
}

float Vertex::y() const
{
	return position.y;
}

float Vertex::z() const
{
	return position.z;
}

Vector4f Vertex::getHomogeneous() const
{
	return Vector4f{this->position, 1.0f};
}
