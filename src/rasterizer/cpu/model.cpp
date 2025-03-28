#include <model.h>

Model::Model(const std::string& object_filename)
{
	loadObject(object_filename);
	return;
}

Model::Model(const std::string& object_filename, const std::string& texture_filepath)
{
	loadObject(object_filename);
	loadTexture(texture_filepath);
	return;
}

bool Model::loadObject(const std::string& object_filename)
{
	std::ifstream in;
	in.open(object_filename, std::ifstream::in);
	if (in.fail())
	{
		std::cerr << "Load model fail, Can not open the model file!" << std::endl;
		return false;
	}

	std::string line;
	while (!in.eof())
	{
		std::getline(in, line);
		std::istringstream iss(line.c_str());

		std::string taget;
		iss >> taget;

		/* Vertex data */
		if (taget == "v")
		{
			float x, y, z;
			iss >> x >> y >> z;
			Vector3f temp{x, y, z};
			this->vertexs.push_back(temp);
			this->vertex_loaded = true;
		}
		/* Texture data */
		else if (taget == "vt")
		{
			float u, v;
			iss >> u >> v;
			Vector2f temp{u, 1.0f - v};
			this->textures.push_back(temp);
			this->texture_loaded = true;
		}
		/* Normal data */
		else if (taget == "vn")
		{
			float x, y, z;
			iss >> x >> y >> z;
			Vector3f temp{x, y, z};
			this->normals.push_back(temp);
			this->normal_loaded = true;
		}
		/* Face data */
		else if (taget == "f")
		{
			std::vector<Vertex> vertics;
			vertics.clear();
			int vertex_index, texture_index, normal_index;
			char temp;
			while (iss >> vertex_index >> temp >> texture_index >> temp >> normal_index)
			{
				vertex_index--;
				texture_index--;
				normal_index--;

				Vertex vertex;
				if (this->vertex_loaded)
				{
					vertex.position = vertexs[vertex_index];
				}

				if (this->texture_loaded)
				{
					vertex.texture = textures[texture_index];
				}

				if (this->normal_loaded)
				{
					vertex.normal = normals[normal_index];
				}

				vertics.push_back(vertex);
			}

			Triangle triangle{vertics[0], vertics[1], vertics[2]};
			this->faces.push_back(triangle);
		}
	}
	return true;
}

bool Model::loadTexture(const std::string& texture_filepath)
{
	this->texture.setTexture(texture_filepath);
	return true;
}

void Model::clear()
{
	this->vertexs.clear();
	this->textures.clear();
	this->normals.clear();
	this->faces.clear();
	this->texture.clear();
	this->vertex_loaded = false;
	this->texture_loaded = false;
	this->normal_loaded = false;
}