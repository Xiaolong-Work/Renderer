#include <vertex.h>

float Vertex::u() const
{
	return texture.x;
}

float& Vertex::u()
{
	return texture.x;
}

float Vertex::v() const
{
	return texture.y;
}

float& Vertex::v()
{
	return texture.y;
}

float Vertex::x() const
{
	return position.x;
}

float& Vertex::x()
{
	return position.x;
}

float Vertex::y() const
{
	return position.y;
}

float& Vertex::y()
{
	return position.y;
}

float Vertex::z() const
{
	return position.z;
}

float& Vertex::z()
{
	return position.z;
}

float Vertex::w() const
{
	return weight;
}

float& Vertex::w()
{
	return weight;
}

Vector4f Vertex::getHomogeneous() const
{
	return Vector4f{this->position, this->weight};
}
