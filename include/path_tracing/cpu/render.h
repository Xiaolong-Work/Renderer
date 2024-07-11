#pragma once

#include <scene.h>
#include <utils.h>

/**
 * @class Renderer
 * @brief A renderer class responsible for rendering a scene using ray tracing.
 */
class Renderer
{
public:
	/**
	 * @brief Default constructor for Renderer.
	 */
	Renderer() = default;

	/**
	 * @brief Constructor that specifies the number of samples per pixel.
	 *
	 * @param[in] spp The number of rays cast per pixel for anti-aliasing.
	 */
	Renderer(const int spp);

	/**
	 * @brief Renders the given scene.
	 *
	 * @param[in,out] scene The scene to be rendered.
	 */
	void render(Scene& scene);

	/* Number of samples per pixel (spp), used for anti-aliasing. */
	int spp = 1;

protected:
	/**
	 * @brief Saves the rendered image result.
	 *
	 * @param[in] scene The scene whose rendering result is saved.
	 */
	void saveResult(const Scene& scene);

private:
	/* Frame buffer storing the computed pixel colors. */
	std::vector<Vector3f> frame_buffer;
};
