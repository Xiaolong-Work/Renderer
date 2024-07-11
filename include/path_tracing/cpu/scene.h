#pragma once

#include <bvh.h>
#include <path_tracing_material.h>
#include <path_tracing_object.h>
#include <utils.h>

/**
 * @class Scene
 * @brief Represents a 3D scene containing objects, a camera, and lighting.
 */
class Scene
{
public:
	/**
	 * @brief Default constructor for Scene.
	 */
	Scene() = default;

	void setData(std::vector<Object>& object, const Camera& camera);

	/**
	 * @brief Initializes the BVH structure for the scene.
	 */
	void initBVH();

	/**
	 * @brief Builds the BVH tree for the scene.
	 *
	 * @param[in] index The index of the BVH node.
	 * @param[in,out] object_index The list of object indices in the scene.
	 * @param[in] bounding_box The bounding box enclosing the objects.
	 */
	void buildBVH(const int index, std::vector<int>& object_index, const BoundingBox& bounding_box);

	/**
	 * @brief Performs a ray-scene intersection test.
	 *
	 * @param[in,out] ray The ray to test for intersection.
	 * @return The intersection result.
	 */
	IntersectResult intersect(Ray& ray) const;

	/**
	 * @brief Traverses the BVH tree to find ray intersections.
	 *
	 * @param[in] index The BVH node index.
	 * @param[in,out] ray The ray to test for intersection.
	 * @return The intersection result.
	 */
	IntersectResult traverse(const int index, Ray& ray) const;

	/**
	 * @brief Samples a point on a light-emitting object in the scene.
	 *
	 * @param[out] result The intersection result storing the sampled point.
	 * @param[out] pdf The probability density function (PDF) value of the sample.
	 */
	void sampleLight(IntersectResult& result, float& pdf);

	/**
	 * @brief Computes the shading for a given ray using path tracing.
	 *
	 * @param[in] ray The ray being traced.
	 * @return The computed radiance at the ray intersection.
	 */
	Vector3f shader(Ray ray);

	/**
	 * @brief Sets the ambient light intensity for the scene.
	 *
	 * @param[in] Ka The ambient light color and intensity.
	 */
	void setEnvironmentLight(const Vector3f& Ka);

	/* The name of the scene. */
	std::string name;

	/* The camera used to render the scene. */
	Camera camera;

	/* The BVH tree for the scene. */
	std::vector<SceneBVH> bvh;

	/* The list of objects present in the scene. */
	std::vector<PathTracingObject> objects;

	/* Indices of objects that function as light sources. */
	std::vector<int> light_object_index;

	/* The maximum recursion depth for path tracing. */
	int max_depth = 10;

	/* The ambient light color and intensity. */
	Vector3f Ka{0.0, 0.0, 0.0};
};
