cmake_minimum_required(VERSION 3.10)

project(ShaderCompilation)

find_package(Vulkan REQUIRED)
message(STATUS "Vulkan include dirs: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan lib dirs: ${Vulkan_LIBRARIES}")

set(VULKAN_BIN_DIR "${Vulkan_INCLUDE_DIRS}/../Bin")
message(STATUS "Vulkan SDK Bin Directory: ${VULKAN_BIN_DIR}")

set(SHADER_COMPILING "${VULKAN_BIN_DIR}/glslc.exe")

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/shaders/comp.spv
    COMMAND ${SHADER_COMPILING} -O --target-env=vulkan1.3 ${CMAKE_SOURCE_DIR}/shaders/pathTracing.comp -o ${CMAKE_SOURCE_DIR}/shaders/comp.spv
    DEPENDS ${CMAKE_SOURCE_DIR}/shaders/pathTracing.comp
    COMMENT "Compiling compute shader"
	VERBATIM
)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/shaders/vert.spv
    COMMAND ${SHADER_COMPILING} -O ${CMAKE_SOURCE_DIR}/shaders/triangle.vert -o ${CMAKE_SOURCE_DIR}/shaders/vert.spv
    DEPENDS ${CMAKE_SOURCE_DIR}/shaders/triangle.vert
    COMMENT "Compiling vertex shader"
	VERBATIM
)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/shaders/frag.spv
    COMMAND ${SHADER_COMPILING} -O ${CMAKE_SOURCE_DIR}/shaders/triangle.frag -o ${CMAKE_SOURCE_DIR}/shaders/frag.spv
    DEPENDS ${CMAKE_SOURCE_DIR}/shaders/triangle.frag
    COMMENT "Compiling fragment shader"
	VERBATIM
)

add_custom_target(
    CompileShaders ALL
    DEPENDS ${CMAKE_BINARY_DIR}/shaders/comp.spv
	DEPENDS ${CMAKE_BINARY_DIR}/shaders/vert.spv
	DEPENDS ${CMAKE_BINARY_DIR}/shaders/frag.spv
)
