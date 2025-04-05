#pragma once
#include <data_loader.h>
#include <ray.h>
#include <render.h>
#include <path_tracing_scene.h>
#include <string>
#include <utils.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

Renderer::Renderer(const int spp)
{
	this->spp = spp;
}

void Renderer::render(PathTracingScene& scene)
{
	auto camera = scene.camera;

	this->frame_buffer.resize(camera.width * camera.height);

	float scale = std::tan(camera.fov * pi / 360.0f);
	float image_aspect_ratio = float(camera.width) / float(camera.height);
	Point eye_position = camera.position;
	Point image_center = camera.look;
	Direction n = image_center - eye_position;

	/* Calculate the local coordinate system on the imaging plane */
	Vector3f local_y = glm::normalize(camera.up);
	Vector3f local_x = glm::normalize(glm::cross(n, local_y));

	float t = scale * glm::length(n);
	float r = t * image_aspect_ratio;
	Point begin = image_center + local_y * t - local_x * r;

	for (int i = 0; i < camera.height; i++)
	{
#pragma omp parallel for
		for (int j = 0; j < camera.width; j++)
		{
			Point pixel_center = begin - local_y * float(i + 0.5) * 2.0f * t / float(camera.height) +
								 local_x * float(j + 0.5) * 2.0f * r / float(camera.width);
			Vector3f direction = glm::normalize(pixel_center - eye_position);
			for (int k = 0; k < this->spp; k++)
			{
				Ray ray{eye_position, direction};
				this->frame_buffer[i * camera.width + j] += scene.shader(ray) / float(this->spp);
			}
		}
		outputProgress(i / (float)camera.height);
	}
	outputProgress(1.f);

	this->saveResult(scene);
}

void Renderer::saveResult(const PathTracingScene& scene)
{
	std::string path = std::string(ROOT_DIR) + "/results/" + scene.name + "_spp_" + std::to_string(this->spp) +
					   "_depth_" + std::to_string(scene.max_depth) + "_cpu.bmp";

	int width = scene.camera.width;
	int height = scene.camera.height;

	std::vector<unsigned char> image_data(width * height * 3);
	for (auto i = 0; i < scene.camera.height * scene.camera.width; ++i)
	{
		static unsigned char color[3];
		image_data[i * 3 + 0] = (unsigned char)(255 * std::pow(clamp(0, 1, this->frame_buffer[i].x), 1.0f / 2.2f));
		image_data[i * 3 + 1] = (unsigned char)(255 * std::pow(clamp(0, 1, this->frame_buffer[i].y), 1.0f / 2.2f));
		image_data[i * 3 + 2] = (unsigned char)(255 * std::pow(clamp(0, 1, this->frame_buffer[i].z), 1.0f / 2.2f));
	}

	stbi_write_bmp(path.c_str(), width, height, 3, image_data.data());
}
