cmake_minimum_required(VERSION 3.10)

project(ShaderCompilation)

find_package(Vulkan REQUIRED)
message(STATUS "Vulkan include dirs: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan lib dirs: ${Vulkan_LIBRARIES}")

set(VULKAN_BIN_DIR "${Vulkan_INCLUDE_DIRS}/../Bin")
message(STATUS "Vulkan SDK Bin Directory: ${VULKAN_BIN_DIR}")

set(SHADER_COMPILING "${VULKAN_BIN_DIR}/glslc.exe")

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/shaders/compute.spv
    COMMAND ${SHADER_COMPILING} -O --target-env=vulkan1.3 ${CMAKE_SOURCE_DIR}/shaders/path_tracing.comp -o ${CMAKE_SOURCE_DIR}/shaders/compute.spv
    DEPENDS ${CMAKE_SOURCE_DIR}/shaders/path_tracing.comp
    COMMENT "Compiling compute shader"
	VERBATIM
)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/shaders/vertex.spv
    COMMAND ${SHADER_COMPILING} -O --target-env=vulkan1.3 ${CMAKE_SOURCE_DIR}/shaders/rasterize.vert -o ${CMAKE_SOURCE_DIR}/shaders/vertex.spv
    DEPENDS ${CMAKE_SOURCE_DIR}/shaders/rasterize.vert
    COMMENT "Compiling vertex shader"
	VERBATIM
)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/shaders/fragment.spv
    COMMAND ${SHADER_COMPILING} -O --target-env=vulkan1.3 ${CMAKE_SOURCE_DIR}/shaders/rasterize.frag -o ${CMAKE_SOURCE_DIR}/shaders/fragment.spv
    DEPENDS ${CMAKE_SOURCE_DIR}/shaders/rasterize.frag
    COMMENT "Compiling fragment shader"
	VERBATIM
)

add_custom_target(
    CompileShaders ALL
    DEPENDS ${CMAKE_BINARY_DIR}/shaders/compute.spv
	DEPENDS ${CMAKE_BINARY_DIR}/shaders/vertex.spv
	DEPENDS ${CMAKE_BINARY_DIR}/shaders/fragment.spv
)
