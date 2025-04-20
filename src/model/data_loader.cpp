#include <data_loader.h>
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
