#pragma once

#include <utils.h>

struct PointLight
{
	alignas(16) Vector3f position;
	alignas(16) Vector3f intensity;
};