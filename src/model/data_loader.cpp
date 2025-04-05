#include <data_loader.h>
#include <obj_loader.h>
#include <tinyxml2.h>

DataLoader::DataLoader(const std::string& data_path, const std::string& name)
{
	this->data_path = data_path;
	this->name = name;
}

void DataLoader::load()
{
	loadObjFile();
	loadXmlFile();
}

void DataLoader::loadObjFile()
{
	/* Use obj loader to load obj file */
	objl::Loader loader;
	loader.LoadFile(this->data_path + this->name + ".obj");
	auto& meshes = loader.LoadedMeshes;

	for (auto& mesh : meshes)
	{
		Object object{};

		object.name = mesh.MeshName;

		/* Read in material data */
		auto& material = mesh.MeshMaterial;
		object.material.name = material.name;
		object.material.kd = Vector3f{material.Kd.X, material.Kd.Y, material.Kd.Z};
		object.material.ks = Vector3f{material.Ks.X, material.Ks.Y, material.Ks.Z};
		object.material.ns = material.Ns;
		object.material.ni = material.Ni;

		/* Determine the material type */
		if (object.material.ns > 1.0f)
		{
			if (object.material.kd == Vector3f{0.0f, 0.0f, 0.0f} && object.material.ks == Vector3f{1.0f, 1.0f, 1.0f})
			{
				object.material.type = MaterialType::Specular;
			}
			else
			{
				object.material.type = MaterialType::Glossy;
			}
		}
		else if (object.material.ni > 1.0f)
		{
			object.material.type = MaterialType::Refraction;
		}
		else
		{
			object.material.type = MaterialType::Diffuse;
		}

		/* Convert data format */
		for (int i = 0; i < mesh.Vertices.size(); i += 3)
		{
			Vertex vertices[3];

			for (int j = 0; j < 3; j++)
			{
				auto& vertex = mesh.Vertices[i + j];

				vertices[j].position = Point{vertex.Position.X, vertex.Position.Y, vertex.Position.Z};
				vertices[j].normal = Direction{vertex.Normal.X, vertex.Normal.Y, vertex.Normal.Z};
				vertices[j].texture = Coordinate2D{vertex.TextureCoordinate.X, vertex.TextureCoordinate.Y};
			}

			Triangle triangle{vertices[0], vertices[1], vertices[2]};
			object.mesh.emplace_back(triangle);

			object.bounding_box.unionBox(triangle.getBoundingBox());
			object.area += triangle.area;
		}

		objects.push_back(object);
	}
}

void DataLoader::loadXmlFile()
{
	/* Load the xml file */
	tinyxml2::XMLDocument file;
	auto path = this->data_path + this->name + ".xml";
	auto error = file.LoadFile(path.c_str());
	if (error != tinyxml2::XMLError::XML_SUCCESS)
	{
		std::cerr << "Could not open XML file." << std::endl;
		return;
	}

	std::map<std::string, CameraType> index;
	index["perspective"] = CameraType::Perspective;
	index["orthographic"] = CameraType::Orthographic;

	/* camera label */
	auto element = file.FirstChildElement();
	assert(element->Name() != "camera");

	camera.type = index[std::string(element->FindAttribute("type")->Value())];
	camera.width = std::atoi(element->FindAttribute("width")->Value());
	camera.height = std::atoi(element->FindAttribute("height")->Value());
	camera.fov = std::atof(element->FindAttribute("fovy")->Value());

	float x, y, z;

	/* eye label */
	element++;
	assert(element->Name() != "eye");

	x = std::atof(element->FindAttribute("x")->Value());
	y = std::atof(element->FindAttribute("y")->Value());
	z = std::atof(element->FindAttribute("z")->Value());
	camera.position = Vector3f{x, y, z};

	/* look at label */
	element++;
	assert(element->Name() != "lookat");

	x = std::atof(element->FindAttribute("x")->Value());
	y = std::atof(element->FindAttribute("y")->Value());
	z = std::atof(element->FindAttribute("z")->Value());
	camera.look = Direction{x, y, z};

	/* up label */
	element++;
	assert(element->Name() != "up");

	x = std::atof(element->FindAttribute("x")->Value());
	y = std::atof(element->FindAttribute("y")->Value());
	z = std::atof(element->FindAttribute("z")->Value());
	camera.up = Direction{x, y, z};

	/* light label */
	while (element != file.LastChildElement())
	{
		element++;
		assert(element->Name() != "light");

		std::string name = std::string(element->FindAttribute("mtlname")->Value());
		for (auto& object : objects)
		{
			if (object.material.name == name)
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
