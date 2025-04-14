#include <data_io.h>

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
		std::cerr << "Could not open object file." << std::endl;
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

		this->materials[i].transferToPBR();

		if (material.diffuse_texname != "")
		{
			auto diffuse_texture_path = path + material.diffuse_texname;

			if (this->texture_index.find(diffuse_texture_path) == this->texture_index.end())
			{
				this->texture_index[diffuse_texture_path] = this->texture_index.size();
			}

			this->materials[i].diffuse_texture = this->texture_index[diffuse_texture_path];
		}
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
				break;
			}
		}
	}
}

void InputOutput::loadGlbFile(const std::string& path)
{
	//tinygltf::TinyGLTF loader;
	//tinygltf::Model model;
	//std::string err, warn;

	//bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, "model.glb");
	//if (!warn.empty())
	//	std::cout << "Warn: " << warn << std::endl;
	//if (!err.empty())
	//	std::cerr << "Err: " << err << std::endl;
	//if (!ret)
	//	throw std::runtime_error("Failed to load glb");

	//for (const auto& mesh : model.meshes)
	//{
	//	for (const auto& primitive : mesh.primitives)
	//	{
	//		const auto& positionAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
	//		const auto& positionBufferView = model.bufferViews[positionAccessor.bufferView];
	//		const auto& positionBuffer = model.buffers[positionBufferView.buffer];
	//		const unsigned char* posData =
	//			positionBuffer.data.data() + positionBufferView.byteOffset + positionAccessor.byteOffset;

	//		// 读取 posData -> 顶点数据
	//	}
	//}
}

void InputOutput::generateScene(Scene& scene)
{
	scene.name = std::move(this->name);
	scene.camera = std::move(this->camera);
	scene.objects = std::move(this->objects);
	scene.materials = std::move(this->materials);

	scene.textures.resize(texture_index.size());
	for (auto& texture : texture_index)
	{
		scene.textures[texture.second].setTexture(texture.first);
	}
	this->texture_index.clear();
}
