#include <bounding_box.h>

BoundingBox::BoundingBox()
{
	this->x_min = std::numeric_limits<float>::infinity();
	this->y_min = std::numeric_limits<float>::infinity();
	this->z_min = std::numeric_limits<float>::infinity();

	this->x_max = -std::numeric_limits<float>::infinity();
	this->y_max = -std::numeric_limits<float>::infinity();
	this->z_max = -std::numeric_limits<float>::infinity();
}

BoundingBox::BoundingBox(const Point& point)
{
	this->x_min = point.x;
	this->y_min = point.y;
	this->z_min = point.z;

	this->x_max = point.x;
	this->y_max = point.y;
	this->z_max = point.z;
}

BoundingBox::BoundingBox(const Point& min, const Point& max)
{
	this->x_min = min.x;
	this->y_min = min.y;
	this->z_min = min.z;

	this->x_max = max.x;
	this->y_max = max.y;
	this->z_max = max.z;
}

void BoundingBox::unionBox(const BoundingBox& other)
{
	this->x_min = std::min(this->x_min, other.x_min);
	this->y_min = std::min(this->y_min, other.y_min);
	this->z_min = std::min(this->z_min, other.z_min);

	this->x_max = std::max(this->x_max, other.x_max);
	this->y_max = std::max(this->y_max, other.y_max);
	this->z_max = std::max(this->z_max, other.z_max);
}

void BoundingBox::unionPoint(const Point& other)
{
	this->x_min = std::min(this->x_min, other.x);
	this->y_min = std::min(this->y_min, other.y);
	this->z_min = std::min(this->z_min, other.z);

	this->x_max = std::max(this->x_max, other.x);
	this->y_max = std::max(this->y_max, other.y);
	this->z_max = std::max(this->z_max, other.z);
}

Point BoundingBox::getMin() const
{
	return Point{this->x_min, this->y_min, this->z_min};
}

Point BoundingBox::getMax() const
{
	return Point{this->x_max, this->y_max, this->z_max};
}

float BoundingBox::getLengthX() const
{
	return this->x_max - this->x_min;
}

float BoundingBox::getLengthY() const
{
	return this->y_max - this->y_min;
}

float BoundingBox::getLengthZ() const
{
	return this->z_max - this->z_min;
}

bool BoundingBox::overlaps(const BoundingBox& other) const
{
	if (this->x_min > other.x_max || this->x_max < other.x_min)
	{
		return false;
	}
	else if (this->y_min > other.y_max || this->y_max < other.y_min)
	{
		return false;
	}
	else if (this->z_min > other.z_max || this->z_max < other.z_min)
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool BoundingBox::overlaps(const Point& other) const
{
	if (this->x_min > other.x || this->x_max < other.x)
	{
		return false;
	}
	else if (this->y_min > other.y || this->y_max < other.y)
	{
		return false;
	}
	else if (this->z_min > other.z || this->z_max < other.z)
	{
		return false;
	}
	else
	{
		return true;
	}
}
