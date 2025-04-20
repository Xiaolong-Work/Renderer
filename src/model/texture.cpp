#include <texture.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Texture::Texture(const std::string& texture_filepath)
{
	this->setTexture(texture_filepath);
}

void Texture::setTexture(const std::string& texture_filepath)
{
	/* Read in texture data */
	stbi_uc* temp = stbi_load(texture_filepath.c_str(), &this->width, &this->height, &this->channel, STBI_rgb_alpha);
	if (!temp)
	{
		throw std::runtime_error("Failed to load texture image!");
	}
	this->texture_path = texture_filepath;

	this->data.resize(4 * this->width * this->height);
	memcpy(this->data.data(), temp, 4 * this->width * this->height * sizeof(unsigned char));
	stbi_image_free(temp);
}

Vector3f Texture::getColor(float u, float v) const
{
	/* Boundary judgment, repeat tiling mode */
	if (u > 1)
	{
		u = u - static_cast<int>(u);
	}
	if (v > 1)
	{
		v = v - static_cast<int>(v);
	}
	if (u < 0)
	{
		u = 1 + (u - static_cast<int>(u));
	}
	if (v < 0)
	{
		v = 1 + (v - static_cast<int>(v));
	}

	int x = static_cast<int>(u * (width - 1));
	int y = static_cast<int>(v * (height - 1));

	int index = (y * width + x) * 4;
	unsigned char r = this->data[index];
	unsigned char g = this->data[index + 1];
	unsigned char b = this->data[index + 2];
	unsigned char a = this->data[index + 3];

	return Vector3f{float(r) / 255.0, float(g) / 255.0, float(b) / 255.0};
}

int Texture::getWidth() const
{
	return this->width;
}

int Texture::getHeight() const
{
	return this->height;
}

void Texture::clear()
{
	this->data.clear();
}

std::string Texture::getPath() const
{
	return this->texture_path;
}

const std::vector<unsigned char>& Texture::getData() const
{
	return this->data;
}
