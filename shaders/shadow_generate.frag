#version 450

layout(location = 0) in vec3 world_position;
layout(location = 1) flat in vec3 light_position;

layout(location = 0) out float liner_depth;

void main() 
{
	liner_depth = length(world_position - light_position);
}