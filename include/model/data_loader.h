#pragma once

#include <cassert>
#include <sstream>

#include <camera.h>
#include <object.h>
#include <utils.h>

/**
 * @class DataLoader
 * @brief A class for loading scene data from files.
 */
class DataLoader
{
public:
	/**
	 * @brief Default constructor for an uninitialized data loader..
	 */
	DataLoader() = default;

	/**
	 * @brief Constructs a DataLoader with a specified file path and object name.
	 *
	 * @param[in] data_path The path to the scene data file.
	 * @param[in] name The name of the object being loaded.
	 */
	DataLoader(const std::string& data_path, const std::string& name);

	/**
	 * @brief Loads scene data into the provided Scene object.
	 */
	void load();

	/* Loaded scene camera information */
	Camera camera;

	/* Loaded scene object information */
	std::vector<Object> objects;

protected:
	/**
	 * @brief Loads a scene from an .obj file.
	 */
	void loadObjFile();

	/**
	 * @brief Loads a scene from an .xml file.
	 */
	void loadXmlFile();

private:
	/* The file path to the scene data. */
	std::string data_path;

	/* The name of the object being loaded. */
	std::string name;
};
