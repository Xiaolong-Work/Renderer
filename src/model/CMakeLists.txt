# Find all source file
file(GLOB SOURCES "*.cpp")

# Add source file
add_library(Model ${SOURCES})

# Add include file
target_include_directories(Model PUBLIC ${CMAKE_SOURCE_DIR}/include/model)

# Add third-party libraries
target_include_directories(Model PUBLIC ${CMAKE_SOURCE_DIR}/external/glm)
target_include_directories(Model PUBLIC ${CMAKE_SOURCE_DIR}/external/stb)
target_include_directories(Model PUBLIC ${CMAKE_SOURCE_DIR}/external/tinyobjloader)
target_include_directories(Model PUBLIC ${CMAKE_SOURCE_DIR}/external/tinyxml2)
target_include_directories(Model PUBLIC ${CMAKE_SOURCE_DIR}/external/tinygltf)

target_sources(Model PUBLIC ${CMAKE_SOURCE_DIR}/external/tinyxml2/tinyxml2.cpp)
