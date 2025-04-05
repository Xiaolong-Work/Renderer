#pragma once

#include <camera.h>

class Scene
{
public:
	/* The name of the scene. */
	std::string name;

	/* The default camera used to render the scene. */
	Camera camera;
	
	/* The object data in the scene */
	std::vector<Object> objects;

	/* The material data in the scene */
	std::vector<Material> materials;

	/* The texture data in the scene */
	std::vector<Texture> textures;
};
