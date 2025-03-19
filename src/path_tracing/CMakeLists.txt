# Find all source file
file(GLOB_RECURSE SOURCES "*.cpp")

# Add source file
add_library(PathTracing ${SOURCES})

# Add include file
target_include_directories(PathTracing PUBLIC ${CMAKE_SOURCE_DIR}/include/path_tracing/cpu)
target_include_directories(PathTracing PUBLIC ${CMAKE_SOURCE_DIR}/include/path_tracing/vulkan)
target_include_directories(PathTracing PUBLIC ${CMAKE_SOURCE_DIR}/include/model)
target_include_directories(PathTracing PUBLIC ${CMAKE_SOURCE_DIR}/include/vulkan_manager)

# Add third-party libraries
target_include_directories(PathTracing PUBLIC ${CMAKE_SOURCE_DIR}/3rd_party/glm)
target_include_directories(PathTracing PUBLIC ${CMAKE_SOURCE_DIR}/3rd_party/stb)

target_link_libraries(PathTracing PUBLIC Model)
target_link_libraries(PathTracing PUBLIC VulkanManager)