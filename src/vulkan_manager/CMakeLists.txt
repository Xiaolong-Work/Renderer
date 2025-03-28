cmake_minimum_required(VERSION 3.10)

# Add Vulkan surpport
find_package(Vulkan REQUIRED)
message(STATUS "Vulkan include dirs: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan lib dirs: ${Vulkan_LIBRARIES}")

# Find all source file
file(GLOB SOURCES "*.cpp")

# Add source file
add_library(VulkanManager ${SOURCES})

# Add include file
target_include_directories(VulkanManager PUBLIC
    ${Vulkan_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include/vulkan_manager
	${CMAKE_SOURCE_DIR}/include/model
    ${CMAKE_SOURCE_DIR}/3rd_party/glm
    ${CMAKE_SOURCE_DIR}/3rd_party/stb
    ${CMAKE_SOURCE_DIR}/3rd_party/glfw/include
    ${CMAKE_SOURCE_DIR}/3rd_party/tinyobjloader
)

target_link_libraries(VulkanManager PUBLIC ${Vulkan_LIBRARIES})