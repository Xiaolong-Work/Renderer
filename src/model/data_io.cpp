#include <data_io.h>

#define TINYGLTF_IMPLEMENTATION
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <tiny_gltf.h>
#include <tiny_obj_loader.h>
#include <tinyxml2.h>

InputOutput::InputOutput(const std::string& name)
{
	this->name = name;
}

void InputOutput::loadObjFile(const std::string& path)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, error;

	if (!tinyobj::LoadObj(
			&attrib, &shapes, &materials, &warn, &error, (path + this->name + ".obj").c_str(), path.c_str()))
	{
		std::cerr << "Could not open obj file." << std::endl;
		throw std::runtime_error(warn + error);
	}

	this->objects.resize(shapes.size());
	/* Processing object data */
	for (size_t i = 0; i < shapes.size(); i++)
	{
		auto& shape = shapes[i];

		/* Object name */
		this->objects[i].name = shape.name;

		/* Object Material */
		this->objects[i].material_index = shape.mesh.material_ids[0];

		std::map<std::tuple<int, int, int>, Index> deduplication_index;
		deduplication_index.clear();

		/* Object vertices and vertex indices */
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex{};

			if (index.vertex_index != -1)
			{
				vertex.position = {attrib.vertices[3 * index.vertex_index + 0],
								   attrib.vertices[3 * index.vertex_index + 1],
								   attrib.vertices[3 * index.vertex_index + 2]};
			}

			if (index.normal_index != -1)
			{
				vertex.normal = {attrib.normals[3 * index.normal_index + 0],
								 attrib.normals[3 * index.normal_index + 1],
								 attrib.normals[3 * index.normal_index + 2]};
			}

			if (index.texcoord_index != -1)
			{
				vertex.texture = {attrib.texcoords[2 * index.texcoord_index + 0],
								  1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};
			}

			auto key = std::make_tuple(index.vertex_index, index.normal_index, index.texcoord_index);
			if (deduplication_index.find(key) != deduplication_index.end())
			{
				this->objects[i].indices.push_back(deduplication_index[key]);
			}
			else
			{
				this->objects[i].vertices.push_back(vertex);
				this->objects[i].indices.push_back(this->objects[i].vertices.size() - 1);
				deduplication_index[key] = static_cast<uint32_t>(this->objects[i].vertices.size() - 1);
			}
		}
	}

	this->materials.resize(materials.size());
	/* Processing material data */
	for (size_t i = 0; i < materials.size(); i++)
	{
		auto& material = materials[i];

		this->materials[i].name = material.name;
		this->materials[i].ka = Vector3f{material.ambient[0], material.ambient[1], material.ambient[2]};
		this->materials[i].kd = Vector3f{material.diffuse[0], material.diffuse[1], material.diffuse[2]};
		this->materials[i].ks = Vector3f{material.specular[0], material.specular[1], material.specular[2]};
		this->materials[i].tr =
			Vector3f{material.transmittance[0], material.transmittance[1], material.transmittance[2]};

		this->materials[i].ns = material.shininess;
		this->materials[i].ni = material.ior;

		/* Determine the material type */
		if (this->materials[i].ns > 1.0f)
		{
			if (this->materials[i].kd == Vector3f{0.0f, 0.0f, 0.0f} &&
				this->materials[i].ks == Vector3f{1.0f, 1.0f, 1.0f})
			{
				this->materials[i].type = MaterialType::Specular;
			}
			else
			{
				this->materials[i].type = MaterialType::Glossy;
			}
		}
		else if (this->materials[i].ni > 1.0f)
		{
			this->materials[i].type = MaterialType::Refraction;
		}
		else
		{
			this->materials[i].type = MaterialType::Diffuse;
		}

		if (material.diffuse_texname != "")
		{
			auto diffuse_texture_path = path + material.diffuse_texname;

			if (this->texture_index.find(diffuse_texture_path) == this->texture_index.end())
			{
				this->texture_index[diffuse_texture_path] = this->texture_index.size();
			}

			this->materials[i].diffuse_texture = this->texture_index[diffuse_texture_path];
		}

		this->materials[i].transferToPBR();
	}
}

void InputOutput::loadXmlFile(const std::string& path)
{
	/* Load the xml file */
	tinyxml2::XMLDocument file;
	auto error = file.LoadFile((path + this->name + ".xml").c_str());
	if (error != tinyxml2::XMLError::XML_SUCCESS)
	{
		std::cerr << "Could not open xml file." << std::endl;
		return;
	}

	std::map<std::string, CameraType> index;
	index["perspective"] = CameraType::Perspective;
	index["orthographic"] = CameraType::Orthographic;

	/* camera label */
	auto element = file.FirstChildElement();
	assert(element->Name() != "camera");

	this->camera.type = index[std::string(element->FindAttribute("type")->Value())];
	this->camera.width = std::atoi(element->FindAttribute("width")->Value());
	this->camera.height = std::atoi(element->FindAttribute("height")->Value());
	this->camera.fov = std::atof(element->FindAttribute("fovy")->Value());

	float x, y, z;

	/* eye label */
	element++;
	assert(element->Name() != "eye");

	x = std::atof(element->FindAttribute("x")->Value());
	y = std::atof(element->FindAttribute("y")->Value());
	z = std::atof(element->FindAttribute("z")->Value());
	this->camera.position = Vector3f{x, y, z};

	/* look at label */
	element++;
	assert(element->Name() != "lookat");

	x = std::atof(element->FindAttribute("x")->Value());
	y = std::atof(element->FindAttribute("y")->Value());
	z = std::atof(element->FindAttribute("z")->Value());
	this->camera.look = Direction{x, y, z};

	/* up label */
	element++;
	assert(element->Name() != "up");

	x = std::atof(element->FindAttribute("x")->Value());
	y = std::atof(element->FindAttribute("y")->Value());
	z = std::atof(element->FindAttribute("z")->Value());
	this->camera.up = Direction{x, y, z};

	/* light label */
	while (element != file.LastChildElement())
	{
		element++;
		assert(element->Name() != "light");

		std::string name = std::string(element->FindAttribute("mtlname")->Value());
		for (auto& object : objects)
		{
			if (object.name == name)
			{
				char temp;
				std::string radiance_string = std::string(element->FindAttribute("radiance")->Value());
				std::istringstream iss(radiance_string);
				iss >> object.radiance.x >> temp >> object.radiance.y >> temp >> object.radiance.z;
				object.is_light = true;
			}
		}
	}
}

void InputOutput::loadGlbFile(const std::string& path)
{
	tinygltf::TinyGLTF loader;
	tinygltf::Model model;
	std::string warn, error;

	if (!loader.LoadBinaryFromFile(&model, &error, &warn, (path + this->name + ".glb").c_str()))
	{
		std::cerr << "Could not open glb file." << std::endl;
		throw std::runtime_error(warn + error);
	}

	auto get_filter = [](const int mode, SamplerMipMapMode& mipmap) {
		switch (mode)
		{
		case TINYGLTF_TEXTURE_FILTER_NEAREST:
			{
				return SamplerFilterMode::Nearest;
			}
		case TINYGLTF_TEXTURE_FILTER_LINEAR:
			{
				return SamplerFilterMode::Linear;
			}
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
			{
				mipmap = SamplerMipMapMode::Nearest;
				return SamplerFilterMode::Nearest;
			}
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
			{
				mipmap = SamplerMipMapMode::Nearest;
				return SamplerFilterMode::Linear;
			}
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
			{
				mipmap = SamplerMipMapMode::Linear;
				return SamplerFilterMode::Nearest;
			}
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
			{
				mipmap = SamplerMipMapMode::Linear;
				return SamplerFilterMode::Linear;
			}
		default:
			{
				mipmap = SamplerMipMapMode::Undefined;
				return SamplerFilterMode::Undefined;
			}
		}
	};

	auto get_address_mode = [](const int mode) {
		switch (mode)
		{
		case TINYGLTF_TEXTURE_WRAP_REPEAT:
			{
				return SamplerAddressMode::Repeat;
			}
		case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
			{
				return SamplerAddressMode::Clamp_To_Edge;
			}
		case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
			{
				return SamplerAddressMode::Mirrored_Repeat;
			}
		default:
			{
				return SamplerAddressMode::Undefined;
			}
		}
	};

	/* Load the name of the scene */
	if (!model.scenes.empty())
	{
		this->name = model.scenes[model.defaultScene >= 0 ? model.defaultScene : 0].name;
	}

	/* Load the texture data of the scene */
	for (auto& gltf_texture : model.textures)
	{
		Texture texture;
		if (gltf_texture.source >= 0 && gltf_texture.source < model.images.size())
		{
			auto& image = model.images[gltf_texture.source];

			texture.name = image.name;
			texture.width = image.width;
			texture.height = image.height;
			texture.channel = image.component;

			/* Processing texture images with different channels */
			if (texture.channel == 4)
			{
				texture.data = std::move(image.image);
			}
			else if (texture.channel == 3)
			{
				texture.data.resize(texture.width * texture.height * 4);
				for (int i = 0; i < texture.width * texture.height; ++i)
				{
					texture.data[i * 4 + 0] = image.image[i * 3 + 0];
					texture.data[i * 4 + 1] = image.image[i * 3 + 1];
					texture.data[i * 4 + 2] = image.image[i * 3 + 2];
					texture.data[i * 4 + 3] = 255;
				}
			}
			else
			{
				throw std::runtime_error("Unsupported image channel count: " + std::to_string(texture.channel));
			}
		}

		/* Processing Sampler Data */
		if (gltf_texture.sampler >= 0 && gltf_texture.sampler < model.samplers.size())
		{
			auto& sampler = model.samplers[gltf_texture.sampler];

			texture.minify = get_filter(sampler.minFilter, texture.mipmap);
			texture.magnify = get_filter(sampler.magFilter, texture.mipmap);
			texture.address_u = get_address_mode(sampler.wrapS);
			texture.address_v = get_address_mode(sampler.wrapT);
			texture.address_w = texture.address_v;
		}
		this->textures.push_back(texture);
	}

	/* Load the material data of the scene */
	for (auto& gltf_material : model.materials)
	{
		Material material;

		material.name = gltf_material.name;
		material.metallic = gltf_material.pbrMetallicRoughness.metallicFactor;
		material.roughness = gltf_material.pbrMetallicRoughness.roughnessFactor;
		auto& color = gltf_material.pbrMetallicRoughness.baseColorFactor;
		material.albedo = Vector4f{color[0], color[1], color[2], color[3]};
		material.albedo_texture = gltf_material.pbrMetallicRoughness.baseColorTexture.index;

		/* Decoding the specular-glossiness material */
		if (gltf_material.extensions.count("KHR_materials_pbrSpecularGlossiness"))
		{
			auto& extensions = gltf_material.extensions.at("KHR_materials_pbrSpecularGlossiness");

			if (extensions.Has("diffuseFactor"))
			{
				const auto& diffuse = extensions.Get("diffuseFactor").Get<tinygltf::Value::Array>();
				for (int i = 0; i < 3; i++)
				{
					material.kd[i] = static_cast<float>(diffuse[i].Get<double>());
				}
			}
			if (extensions.Has("diffuseTexture"))
			{
				material.diffuse_texture = extensions.Get("diffuseTexture").Get("index").Get<int>();
			}

			if (extensions.Has("glossinessFactor"))
			{
				material.ns = static_cast<float>(extensions.Get("glossinessFactor").Get<double>());
			}
			if (extensions.Has("specularGlossinessTexture"))
			{
				material.specular_texture = extensions.Get("specularGlossinessTexture").Get("index").Get<int>();
			}

			material.transferToPBR();
		}

		/* Decoding the ior */
		if (gltf_material.extensions.count("KHR_materials_ior"))
		{
			auto& extensions = gltf_material.extensions.at("KHR_materials_ior");
			material.ni = extensions.Get("ior").Get<double>();
		}

		/* Decoding the specular */
		if (gltf_material.extensions.count("KHR_materials_specular"))
		{
			auto& extensions = gltf_material.extensions.at("KHR_materials_specular");
			auto& specular = extensions.Get("specularColorFactor").Get<tinygltf::Value::Array>();
			for (int i = 0; i < 3; i++)
			{
				material.ks[i] = static_cast<float>(specular[i].Get<double>());
			}
		}

		/* TODO more extensions */

		this->materials.push_back(material);
	}

	auto get_model_matrix = [](const tinygltf::Node& node) {
		if (!node.matrix.empty() && node.matrix.size() == 16)
		{
			Matrix4f matrix(1.0f);
			for (int i = 0; i < 16; ++i)
			{
				matrix[i / 4][i % 4] = static_cast<float>(node.matrix[i]);
			}

			return matrix;
		}
		else
		{
			Vector3f translation(0.0f);
			if (node.translation.size() == 3)
				translation = glm::vec3(node.translation[0], node.translation[1], node.translation[2]);

			glm::quat rotation = glm::quat(1.0, 0.0, 0.0, 0.0);
			if (node.rotation.size() == 4)
				rotation = glm::quat(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);

			glm::vec3 scale(1.0f);
			if (node.scale.size() == 3)
				scale = glm::vec3(node.scale[0], node.scale[1], node.scale[2]);

			glm::mat4 t = glm::translate(glm::mat4(1.0f), translation);
			glm::mat4 r = glm::mat4_cast(rotation);
			glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);

			return t * r * s;
		}
	};

	/* Parsing node data */
	for (auto& node : model.nodes)
	{
		/* Load the camera of the scene */
		if (node.camera >= 0)
		{
			auto& camera = model.cameras[node.camera];
			this->camera.name = camera.name;
			if (camera.type == "perspective")
			{
				this->camera.aspect_ratio = static_cast<float>(camera.perspective.aspectRatio);
				this->camera.fovy = static_cast<float>(camera.perspective.yfov);
				this->camera.near_plane = static_cast<float>(camera.perspective.znear);
				this->camera.far_plane = static_cast<float>(camera.perspective.zfar);
				this->camera.type = CameraType::Perspective;
			}
		}
		/* Load light source of the scene */
		else if (node.light >= 0)
		{
			auto& light = model.lights[node.light];

			if (light.type == "point")
			{
				PointLight point_light;

				point_light.color = Vector3f(light.color[0], light.color[1], light.color[2]);
				point_light.intensity = static_cast<float>(light.intensity);

				Vector4f position = get_model_matrix(node) * Vector4f(Vector3f(0.0f), 1.0f);
				point_light.position = position / position.w;

				this->point_lights.push_back(point_light);
			}
			else if (light.type == "directional")
			{

			}
		}
		/* Loading model data */
		else if (node.mesh >= 0)
		{
			auto& mesh = model.meshes[node.mesh];
			for (auto& primitive : mesh.primitives)
			{
				Object object;
				object.name = mesh.name;

				auto& position_accessor = model.accessors[primitive.attributes.at("POSITION")];
				auto& normal_accessor = model.accessors[primitive.attributes.at("NORMAL")];
				auto& texture_accessor = primitive.attributes.count("TEXCOORD_0")
											 ? model.accessors[primitive.attributes.at("TEXCOORD_0")]
											 : position_accessor;

				auto& position_buffer_view = model.bufferViews[position_accessor.bufferView];
				auto& position_buffer = model.buffers[position_buffer_view.buffer];

				float* position_data = reinterpret_cast<float*>(
					&position_buffer.data[position_buffer_view.byteOffset + position_accessor.byteOffset]);

				float* normal_data = reinterpret_cast<float*>(
					&model.buffers[model.bufferViews[normal_accessor.bufferView].buffer]
						 .data[model.bufferViews[normal_accessor.bufferView].byteOffset + normal_accessor.byteOffset]);

				float* texture_data =
					reinterpret_cast<float*>(&model.buffers[model.bufferViews[texture_accessor.bufferView].buffer]
												  .data[model.bufferViews[texture_accessor.bufferView].byteOffset +
														texture_accessor.byteOffset]);

				bool has_color = primitive.attributes.count("COLOR_0") > 0;
				std::vector<glm::vec4> colors;
				if (has_color)
				{
					auto& color_accessor = model.accessors[primitive.attributes.at("COLOR_0")];
					auto& color_view = model.bufferViews[color_accessor.bufferView];
					auto& color_buffer = model.buffers[color_view.buffer];

					const uint8_t* color_ptr = &color_buffer.data[color_view.byteOffset + color_accessor.byteOffset];
					colors.resize(color_accessor.count);

					if (color_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
					{
						if (color_accessor.type == TINYGLTF_TYPE_VEC3)
						{
							const float* data = reinterpret_cast<const float*>(color_ptr);
							for (size_t i = 0; i < color_accessor.count; ++i)
							{
								colors[i] = glm::vec4(data[i * 3 + 0], data[i * 3 + 1], data[i * 3 + 2], 1.0f);
							}
						}
						else if (color_accessor.type == TINYGLTF_TYPE_VEC4)
						{
							const float* data = reinterpret_cast<const float*>(color_ptr);
							for (size_t i = 0; i < color_accessor.count; ++i)
							{
								colors[i] =
									glm::vec4(data[i * 4 + 0], data[i * 4 + 1], data[i * 4 + 2], data[i * 4 + 3]);
							}
						}
					}
					else if (color_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
					{
						if (color_accessor.type == TINYGLTF_TYPE_VEC3)
						{
							const uint16_t* data = reinterpret_cast<const uint16_t*>(color_ptr);
							for (size_t i = 0; i < color_accessor.count; ++i)
							{
								colors[i] = glm::vec4(data[i * 3 + 0] / 65535.0f,
													  data[i * 3 + 1] / 65535.0f,
													  data[i * 3 + 2] / 65535.0f,
													  1.0f);
							}
						}
						else if (color_accessor.type == TINYGLTF_TYPE_VEC4)
						{
							const uint16_t* data = reinterpret_cast<const uint16_t*>(color_ptr);
							for (size_t i = 0; i < color_accessor.count; ++i)
							{
								colors[i] = glm::vec4(data[i * 4 + 0] / 65535.0f,
													  data[i * 4 + 1] / 65535.0f,
													  data[i * 4 + 2] / 65535.0f,
													  data[i * 4 + 3] / 65535.0f);
							}
						}
					}
					else
					{
						/* TODO */
					}
				}

				size_t vertex_count = position_accessor.count;
				object.vertices.resize(vertex_count);
				for (size_t i = 0; i < vertex_count; ++i)
				{
					object.vertices[i].position =
						Coordinate3D(position_data[i * 3 + 0], position_data[i * 3 + 1], position_data[i * 3 + 2]);
					object.vertices[i].normal =
						Direction(normal_data[i * 3 + 0], normal_data[i * 3 + 1], normal_data[i * 3 + 2]);
					object.vertices[i].texture = Coordinate2D(texture_data[i * 2 + 0], texture_data[i * 2 + 1]);

					object.vertices[i].color = has_color ? colors[i] : glm::vec4(1.0f);
				}

				const auto& index_accessor = model.accessors[primitive.indices];
				const auto& index_view = model.bufferViews[index_accessor.bufferView];
				const auto& index_buffer = model.buffers[index_view.buffer];
				const void* index_data_ptr = &index_buffer.data[index_view.byteOffset + index_accessor.byteOffset];

				size_t index_count = index_accessor.count;
				object.indices.resize(index_count);
				for (size_t i = 0; i < index_count; ++i)
				{
					if (index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
					{
						object.indices[i] = static_cast<const uint16_t*>(index_data_ptr)[i];
					}

					else if (index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
						object.indices[i] = static_cast<const uint32_t*>(index_data_ptr)[i];
					else if (index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
						object.indices[i] = static_cast<const uint8_t*>(index_data_ptr)[i];
				}

				object.model = get_model_matrix(node);
				object.material_index = primitive.material;
				this->objects.push_back(object);
			}
		}
		else
		{
			/* TODO */
		}
	}
}

void InputOutput::generateScene(Scene& scene)
{
	scene.name = std::move(this->name);
	scene.camera = std::move(this->camera);
	scene.objects = std::move(this->objects);
	scene.materials = std::move(this->materials);
	scene.point_lights = std::move(this->point_lights);

	if (this->textures.empty())
	{
		scene.textures.resize(texture_index.size());
		for (auto& texture : texture_index)
		{
			scene.textures[texture.second].setTexture(texture.first);
		}
		this->texture_index.clear();
	}
	else
	{
		scene.textures = std::move(this->textures);
	}
}
