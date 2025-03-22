#include <renderer.h>

#include <stb_image_write.h>

void VulkanPathTracingRender::saveResult()
{
	std::vector<Vector3f> frame_buffer;
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	this->storageImageManager.getData(stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(this->contentManager.device, stagingBufferMemory, 0, 1024 * 1024 * 4, 0, &data);

	VkImageSubresource subresource{};
	subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource.mipLevel = 0;
	subresource.arrayLayer = 0;

	VkSubresourceLayout layout;
	vkGetImageSubresourceLayout(this->contentManager.device, this->storageImageManager.image, &subresource, &layout);

	std::string path = std::string(ROOT_DIR) + "/results/" + this->bufferManager.scene_name + "_spp_" +
					   std::to_string(this->bufferManager.scene.spp) + "_depth_" +
					   std::to_string(this->bufferManager.scene.max_depth) + "_gpu.bmp";

	int width = this->bufferManager.scene.width;
	int height = this->bufferManager.scene.height;

	std::vector<unsigned char> image_data(width * height * 3);
	float* pixelData = static_cast<float*>(data);
	for (uint32_t y = 0; y < 1024; y++)
	{
		for (uint32_t x = 0; x < 1024; x++)
		{
			static unsigned char color[3];
			uint32_t pixelIndex = y * 1024 + x;
			image_data[pixelIndex * 3 + 0] =
				(unsigned char)(255 * std::pow(clamp(0, 1, pixelData[pixelIndex * 4 + 0]), 0.6f));
			image_data[pixelIndex * 3 + 1] =
				(unsigned char)(255 * std::pow(clamp(0, 1, pixelData[pixelIndex * 4 + 1]), 0.6f));
			image_data[pixelIndex * 3 + 2] =
				(unsigned char)(255 * std::pow(clamp(0, 1, pixelData[pixelIndex * 4 + 2]), 0.6f));
		}
	}

	stbi_write_bmp(path.c_str(), width, height, 3, image_data.data());

	std::cout << "The rendering result has been written to " << path << std::endl;

	vkDestroyBuffer(contentManager.device, stagingBuffer, nullptr);
	vkFreeMemory(contentManager.device, stagingBufferMemory, nullptr);
}
