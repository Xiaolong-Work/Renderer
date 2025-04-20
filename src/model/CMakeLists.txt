# Find all source file
file(GLOB SOURCES "*.cpp")

# Add source file
add_library(Model ${SOURCES})

# Add include file
target_include_directories(Model PUBLIC ${CMAKE_SOURCE_DIR}/include/model)

# Add third-party libraries
target_include_directories(Model PUBLIC ${CMAKE_SOURCE_DIR}/3rd_party/glm)
target_include_directories(Model PUBLIC ${CMAKE_SOURCE_DIR}/3rd_party/stb)
target_include_directories(Model PUBLIC ${CMAKE_SOURCE_DIR}/3rd_party/tinyobjloader)
target_include_directories(Model PUBLIC ${CMAKE_SOURCE_DIR}/3rd_party/tinyxml2)
target_include_directories(Model PUBLIC ${CMAKE_SOURCE_DIR}/3rd_party/tinygltf)

target_sources(Model PUBLIC ${CMAKE_SOURCE_DIR}/3rd_party/tinyxml2/tinyxml2.cpp)
