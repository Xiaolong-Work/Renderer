# Find all source file
file(GLOB_RECURSE SOURCES "*.cpp")

# Add source file
add_library(Rasterizer ${SOURCES})

# Add include file
target_include_directories(Rasterizer PUBLIC ${CMAKE_SOURCE_DIR}/include/rasterizer/cpu)
target_include_directories(Rasterizer PUBLIC ${CMAKE_SOURCE_DIR}/include/rasterizer/vulkan)
target_include_directories(Rasterizer PUBLIC ${CMAKE_SOURCE_DIR}/include/model)
target_include_directories(Rasterizer PUBLIC ${CMAKE_SOURCE_DIR}/include/vulkan_manager)

# Add third-party libraries
target_include_directories(Rasterizer PUBLIC ${CMAKE_SOURCE_DIR}/3rd_party/glm)
target_include_directories(Rasterizer PUBLIC ${CMAKE_SOURCE_DIR}/3rd_party/stb)

target_link_libraries(Rasterizer PUBLIC Model)
target_link_libraries(Rasterizer PUBLIC VulkanManager)