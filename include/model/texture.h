#pragma once

#include <stb_image.h>

#include <utils.h>

enum class SamplerFilterMode
{
	Undefined,
	Nearest,
	Linear,
};

enum class SamplerMipMapMode
{
	Undefined,
	Nearest,
	Linear,
};

enum SamplerAddressMode
{
	Undefined,
	Clamp_To_Edge,
	Mirrored_Repeat,
	Repeat
};

/**
 * @class Texture
 * @brief A class representing a texture in 3D rendering.
 */
class Texture
{
public:
	/**
	 * @brief Default constructor for an empty texture..
	 */
	Texture() = default;

	/**
	 * @brief Constructs a texture from an image file.
	 *
	 * @param[in] texture_filepath The file path of the texture image.
	 */
	Texture(const std::string& texture_filepath);

	/**
	 * @brief Loads texture data from an image file.
	 *
	 * @param[in] texture_filepath The file path of the texture image.
	 */
	void setTexture(const std::string& texture_filepath);

	/**
	 * @brief Retrieves the color at the specified texture coordinates.
	 *
	 * @param[in] u The horizontal texture coordinate (0.0 to 1.0).
	 * @param[in] v The vertical texture coordinate (0.0 to 1.0).
	 * @return The color at the specified texture coordinates as a Vector3f.
	 */
	Vector3f getColor(float u, float v) const;

	/**
	 * @brief Gets the width of the texture.
	 *
	 * @return The width of the texture in pixels.
	 */
	int getWidth() const;

	/**
	 * @brief Gets the height of the texture.
	 *
	 * @return The height of the texture in pixels.
	 */
	int getHeight() const;

	/**
	 * @brief Clear texture data.
	 */
	void clear();

	/**
	 * @brief Get the path of the texture.
	 *
	 * @return The texture path.
	 */
	std::string getPath() const;

	/**
	 * @brief Retrieves the raw texture data.
	 *
	 * @return A pointer to the texture data array.
	 */
	const std::vector<unsigned char>& getData() const;

	std::string name;

	/* The texture data stored as an array of unsigned bytes. */
	std::vector<unsigned char> data;

	/* The width, height, and channel count of the texture. */
	int width, height, channel;

	/* The texture file path */
	std::string texture_path;

	SamplerFilterMode minify{SamplerFilterMode::Undefined};
	SamplerFilterMode magnify{SamplerFilterMode::Undefined};

	SamplerMipMapMode mipmap{SamplerMipMapMode::Undefined};

	SamplerAddressMode address_u{SamplerAddressMode::Undefined};
	SamplerAddressMode address_v{SamplerAddressMode::Undefined};
	SamplerAddressMode address_w{SamplerAddressMode::Undefined};
};