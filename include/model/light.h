#pragma once

#include <utils.h>

struct PointLight
{
	alignas(16) Coordinate3D position;
	alignas(16) Vector3f color;
	float intensity;
};

struct DirectionLight
{
	alignas(16) Coordinate3D position;
	alignas(16) Direction direction;
	alignas(16) Vector3f color;
	float intensity;
};
