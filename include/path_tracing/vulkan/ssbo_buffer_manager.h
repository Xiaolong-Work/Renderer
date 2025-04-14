#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <buffer_manager.h>

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
	alignas(16) glm::vec3 ka;
	alignas(16) glm::vec3 kd;
	alignas(16) glm::vec3 ks;
	alignas(16) glm::vec3 tr;
	float ns;
	float ni;
	int type;
	int texture_index;

	void operator=(const Material& material)
	{
		this->ka = material.ka;
		this->kd = material.kd;
		this->ks = material.ks;
		this->tr = material.tr;
		this->ns = material.ns;
		this->ni = material.ni;
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

	alignas(16) glm::vec3 ka;
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
	SSBOBufferManager(const ContextManagerSPtr& context_manager_sptr, const CommandManagerSPtr& command_manager_sptr)
	{
		this->context_manager_sptr = context_manager_sptr;
		this->command_manager_sptr = command_manager_sptr;
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

	std::vector<std::string> texture_paths;

	void setData(const PathTracingScene& scene, int spp)
	{
		this->scene.position = scene.camera.position;
		this->scene.look = scene.camera.look;
		this->scene.up = scene.camera.up;
		this->scene.ka = Vector3f{0.0f};
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
			if (object.material.diffuse_texture != -1)
			{
				this->texture_paths.push_back(scene.textures[object.material.diffuse_texture].getPath());
				temp_material.texture_index = this->texture_paths.size() - 1;
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
			vkDestroyBuffer(context_manager_sptr->device, buffers[i], nullptr);
			vkFreeMemory(context_manager_sptr->device, memories[i], nullptr);
		}
	}
};