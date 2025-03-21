#pragma once

#include <buffer_manager.h>
#include <command_manager.h>
#include <fstream>
#include <image_manager.h>
#include <iostream>
#include <scene.h>
#include <texture_manager.h>
#include <ssbo_buffer_manager.h>
#include <shader_manager.h>


class VulkanPathTracingRender
{
public:
	int spp;
	void setSpp(int spp)
	{
		this->spp = spp;
	}

	VkPipeline computePipeline;
	VkPipelineLayout computePipelineLayout;
	ShaderManager computeShaderManager;
	void init(const Scene& scene)
	{
		this->contentManager.init();
		auto pContentManager = std::make_shared<ContentManager>(this->contentManager);

		this->commandManager = CommandManager(pContentManager);
		this->commandManager.init();
		auto pCommandManager = std::make_shared<CommandManager>(this->commandManager);

		this->bufferManager = SSBOBufferManager(pContentManager, pCommandManager);
		this->bufferManager.setData(scene, this->spp);
		this->bufferManager.init();

		this->storageImageManager = StorageImageManager(pContentManager, pCommandManager);
		this->storageImageManager.setExtent(VkExtent2D{scene.camera.width, scene.camera.height});
		this->storageImageManager.init();


		this->computeShaderManager = ShaderManager(pContentManager);
		this->computeShaderManager.setShaderName("compute.spv");
		this->computeShaderManager.init();

		this->textureManager = TextureManager(pContentManager, pCommandManager);
		this->textureManager.init();
		auto pTextureManager = std::make_shared<TextureManager>(this->textureManager);

		createDescriptorSetLayout();
		createDescriptorPool();
		createDescriptorSets();

		VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
		computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		computeShaderStageInfo.pNext = nullptr;
		computeShaderStageInfo.flags = 0;
		computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		computeShaderStageInfo.module = this->computeShaderManager.module;
		computeShaderStageInfo.pName = "main";
		computeShaderStageInfo.pSpecializationInfo = nullptr;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

		if (vkCreatePipelineLayout(contentManager.device, &pipelineLayoutInfo, nullptr, &computePipelineLayout) !=
			VK_SUCCESS)
		{
			throw std::runtime_error("failed to create compute pipeline layout!");
		}

		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.layout = computePipelineLayout;
		pipelineInfo.stage = computeShaderStageInfo;

		if (vkCreateComputePipelines(
				contentManager.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create compute pipeline!");
		}
	}

	void draw()
	{
		auto commandBuffer = commandManager.beginComputeCommands();

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
		vkCmdBindDescriptorSets(
			commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &descriptorSet, 0, 0);

		vkCmdDispatch(commandBuffer, 1024, 1024, 1);

		commandManager.endComputeCommands(commandBuffer);
	}

	VkDescriptorSetLayout descriptorSetLayout;
	void createDescriptorSetLayout()
	{
		std::array<VkDescriptorSetLayoutBinding, 9> layoutBindings{};
		for (size_t i = 0; i < 8; i++)
		{
			layoutBindings[i].binding = i;
			layoutBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			layoutBindings[i].descriptorCount = 1;
			layoutBindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			layoutBindings[i].pImmutableSamplers = nullptr;
		}

		layoutBindings[8].binding = 8;
		layoutBindings[8].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		layoutBindings[8].descriptorCount = 1;
		layoutBindings[8].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		layoutBindings[8].pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.pNext = nullptr;
		layoutInfo.flags = 0;
		layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
		layoutInfo.pBindings = layoutBindings.data();

		if (vkCreateDescriptorSetLayout(contentManager.device, &layoutInfo, nullptr, &this->descriptorSetLayout) !=
			VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create descriptor set layout!");
		}
	}

	VkDescriptorPool descriptorPool;
	void createDescriptorPool()
	{
		std::array<VkDescriptorPoolSize, 2> poolSizes;
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSizes[0].descriptorCount = 8;

		poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		poolSizes[1].descriptorCount = 1;


		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.pNext = nullptr;
		poolInfo.flags = 0;
		poolInfo.maxSets = 1;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();

		if (vkCreateDescriptorPool(contentManager.device, &poolInfo, nullptr, &this->descriptorPool) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create descriptor pool!");
		}
	}

	VkDescriptorSet descriptorSet;
	void createDescriptorSets()
	{
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.descriptorPool = this->descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &descriptorSetLayout;

		if (vkAllocateDescriptorSets(contentManager.device, &allocInfo, &this->descriptorSet) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate descriptor sets!");
		}

		std::array<VkDescriptorBufferInfo, 8> bufferInfo{};
		for (size_t i = 0; i < 8; i++)
		{
			bufferInfo[i].buffer = this->bufferManager.buffers[i];
			bufferInfo[i].offset = 0;
			bufferInfo[i].range = VK_WHOLE_SIZE;
		}

		std::array<VkDescriptorImageInfo, 1> storageImageInfo{};
		storageImageInfo[0].imageView = storageImageManager.imageView;
		storageImageInfo[0].sampler = VK_NULL_HANDLE;
		storageImageInfo[0].imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		std::array<VkWriteDescriptorSet, 9> descriptorWrites{};
		for (size_t i = 0; i < 8; i++)
		{
			descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[i].pNext = nullptr;
			descriptorWrites[i].dstSet = this->descriptorSet;
			descriptorWrites[i].dstBinding = static_cast<uint32_t>(i);
			descriptorWrites[i].dstArrayElement = 0;
			descriptorWrites[i].descriptorCount = 1;
			descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorWrites[i].pBufferInfo = &bufferInfo[i];
			descriptorWrites[i].pImageInfo = nullptr;
			descriptorWrites[i].pTexelBufferView = nullptr;
		}

		descriptorWrites[8].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[8].pNext = nullptr;
		descriptorWrites[8].dstSet = this->descriptorSet;
		descriptorWrites[8].dstBinding = 8;
		descriptorWrites[8].dstArrayElement = 0;
		descriptorWrites[8].descriptorCount = 1;
		descriptorWrites[8].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		descriptorWrites[8].pBufferInfo = nullptr;
		descriptorWrites[8].pImageInfo = &storageImageInfo[0];
		descriptorWrites[8].pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(
			contentManager.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}

	VkShaderModule createShaderModule(const std::vector<char>& code)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(contentManager.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create shader module!");
		}

		return shaderModule;
	}

	StorageImageManager storageImageManager;

	struct SSBOVertex
	{
		alignas(16) glm::vec3 position;
		alignas(16) glm::vec3 normal;
		alignas(16) glm::vec2 texture;

		void operator=(const Vertex& vertex)
		{
			this->normal = vertex.normal;
			this->position = vertex.position;
			this->texture = vertex.texture;
		}
	};

	struct SSBOBoundingBox
	{
		alignas(16) glm::vec3 min;
		alignas(16) glm::vec3 max;

		void operator=(const BoundingBox& box)
		{
			this->min = box.getMin();
			this->max = box.getMax();
		}
	};

	struct SSBOBVH
	{
		SSBOBoundingBox box;
		int left;
		int right;
		int index;
		float area;
	};

	struct SSBOTriangle
	{
		alignas(16) SSBOVertex vertex1;
		alignas(16) SSBOVertex vertex2;
		alignas(16) SSBOVertex vertex3;
		alignas(16) glm::vec3 normal;
		float area;

		void operator=(const Triangle& triangle)
		{
			this->vertex1 = triangle.vertex1;
			this->vertex2 = triangle.vertex2;
			this->vertex3 = triangle.vertex3;
			this->normal = triangle.normal;
			this->area = triangle.area;
		}
	};

	struct SSBOOMaterial
	{
		alignas(16) glm::vec3 Ka;
		alignas(16) glm::vec3 Kd;
		alignas(16) glm::vec3 Ks;
		alignas(16) glm::vec3 Tr;
		float Ns;
		float Ni;
		int type;
		int texture_index;

		void operator=(const Material& material)
		{
			this->Ka = material.Ka;
			this->Kd = material.Kd;
			this->Ks = material.Ks;
			this->Tr = material.Tr;
			this->Ns = material.Ns;
			this->Ni = material.Ni;
			this->type = (int)material.type;
			this->texture_index = -1;
		}
	};

	struct SSBOObject
	{
		SSBOBoundingBox box;
		alignas(16) glm::vec3 radiance;
		float area;
		int material_index;
		int bvh_index;
		bool is_light;

		void operator=(const Object& object)
		{
			this->box = object.bounding_box;
			this->area = object.area;
			this->is_light = object.is_light;
			this->radiance = object.radiance;
			this->bvh_index = 0;
		}
	};

	struct SSBOScene
	{
		alignas(16) glm::vec3 position;
		alignas(16) glm::vec3 look;
		alignas(16) glm::vec3 up;

		alignas(16) glm::vec3 Ka;
		float fov;

		int width;
		int height;

		int max_depth;
		int spp;
	};

	class SSBOBufferManager : public BufferManager
	{
	public:
		SSBOBufferManager() = default;
		SSBOBufferManager(const ContentManagerSPtr& pContentManager, const CommandManagerSPtr& pCommandManager)
		{
			this->pContentManager = pContentManager;
			this->pCommandManager = pCommandManager;
		}
		std::array<VkBuffer, 9> buffers;
		std::array<VkDeviceMemory, 9> memories;

		std::vector<float> random_table;
		std::vector<SSBOBVH> bvhs;
		std::vector<SSBOBVH> scene_bvhs;
		std::vector<int> light_object_indexs;
		std::vector<SSBOTriangle> triangles;
		std::vector<SSBOObject> objects;
		std::vector<SSBOOMaterial> materials;
		SSBOScene scene;
		std::string scene_name;

		void setData(const Scene& scene, int spp)
		{
			this->scene.position = scene.camera.position;
			this->scene.look = scene.camera.look;
			this->scene.up = scene.camera.up;
			this->scene.Ka = scene.Ka;
			this->scene.fov = scene.camera.fov;
			this->scene.width = scene.camera.width;
			this->scene.height = scene.camera.height;
			this->scene.max_depth = scene.max_depth;
			this->scene.spp = spp;

			this->scene_name = scene.name;

			for (int i = 0; i < scene.camera.width * scene.camera.height; i++)
			{
				this->random_table.push_back(getRandomNumber(0, 1));
			}

			for (auto& object : scene.objects)
			{
				int begin_size = this->bvhs.size();

				SSBOBVH bvh;
				SSBOTriangle triangle;
				for (auto& node : object.bvh)
				{
					bvh.box = node.bounding_box;
					bvh.left = node.left + begin_size;
					bvh.right = node.right + begin_size;
					bvh.area = node.area;

					if (node.leaf_node_flag)
					{
						triangle = node.triangle;
						this->triangles.push_back(triangle);
						bvh.index = this->triangles.size() - 1;
					}
					else
					{
						bvh.index = -1;
					}
					this->bvhs.push_back(bvh);
				}

				SSBOOMaterial temp_material;
				temp_material = object.material;
				if (object.material.texture_load_flag)
				{
					// TODO
				}
				else
				{
					temp_material.texture_index = -1;
				}
				this->materials.push_back(temp_material);

				SSBOObject temp_object;
				temp_object = object;
				temp_object.bvh_index = begin_size;
				temp_object.material_index = this->materials.size() - 1;

				this->objects.push_back(temp_object);
			}

			SSBOBVH bvh;
			for (auto& node : scene.bvh)
			{
				bvh.box = node.bounding_box;
				bvh.left = node.left;
				bvh.right = node.right;
				bvh.index = node.object_index;
				bvh.area = node.area;
				this->scene_bvhs.push_back(bvh);
			}
			this->light_object_indexs = scene.light_object_index;
		}

		void init()
		{
			createDeviceLocalBuffer(this->random_table.size() * sizeof(float),
									this->random_table.data(),
									VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
									this->buffers[0],
									this->memories[0]);

			createDeviceLocalBuffer(this->bvhs.size() * sizeof(SSBOBVH),
									this->bvhs.data(),
									VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
									this->buffers[1],
									this->memories[1]);

			createDeviceLocalBuffer(this->triangles.size() * sizeof(SSBOTriangle),
									this->triangles.data(),
									VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
									this->buffers[2],
									this->memories[2]);

			createDeviceLocalBuffer(this->objects.size() * sizeof(SSBOObject),
									this->objects.data(),
									VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
									this->buffers[3],
									this->memories[3]);

			createDeviceLocalBuffer(this->materials.size() * sizeof(SSBOOMaterial),
									this->materials.data(),
									VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
									this->buffers[4],
									this->memories[4]);

			createDeviceLocalBuffer(this->scene_bvhs.size() * sizeof(SSBOBVH),
									this->scene_bvhs.data(),
									VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
									this->buffers[5],
									this->memories[5]);

			createDeviceLocalBuffer(this->light_object_indexs.size() * sizeof(int),
									this->light_object_indexs.data(),
									VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
									this->buffers[6],
									this->memories[6]);

			createDeviceLocalBuffer(
				sizeof(SSBOScene), &scene, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, this->buffers[7], this->memories[7]);

			createBuffer(sizeof(SSBOScene),
						 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
						 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
						 this->buffers[8],
						 this->memories[8]);
		}

		void clear()
		{
			for (int i = 0; i < 5; i++)
			{
				vkDestroyBuffer(pContentManager->device, buffers[i], nullptr);
				vkFreeMemory(pContentManager->device, memories[i], nullptr);
			}
		}
	};

	void saveResult();

	ContentManager contentManager{};
	CommandManager commandManager{};
	SSBOBufferManager bufferManager{};
	TextureManager textureManager{};
};