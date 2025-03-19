cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

# Enable hot reload for the MSVC compiler
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# Add executable file
project (Renderer LANGUAGES CXX)
add_executable(Renderer src/main.cpp)

# Setting the C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add include file
find_package(Vulkan REQUIRED)
include_directories(
	${Vulkan_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/3rd_party/glfw/include
	${CMAKE_SOURCE_DIR}/3rd_party/glm
)

# Add source directory
add_subdirectory(src/path_tracing)
add_subdirectory(src/model)
add_subdirectory(src/vulkan_manager)
add_subdirectory(shaders)
add_subdirectory(3rd_party/glm)
add_subdirectory(3rd_party/glfw)

# Link library
target_link_libraries(Renderer PathTracing)
target_link_libraries(Renderer VulkanManager)
target_link_libraries(Renderer glfw)

# OpenMp support
if (MSVC)
    target_compile_options(PathTracing PRIVATE /openmp)
else()
	find_package(OpenMP REQUIRED)
    target_link_libraries(PathTracing PRIVATE OpenMP::OpenMP_CXX)
endif()

# Macro Definition
target_compile_definitions(Renderer PRIVATE ROOT_DIR="${CMAKE_SOURCE_DIR}")
target_compile_definitions(Model PRIVATE ROOT_DIR="${CMAKE_SOURCE_DIR}")
target_compile_definitions(PathTracing PRIVATE ROOT_DIR="${CMAKE_SOURCE_DIR}")

set(CMAKE_VERBOSE_MAKEFILE ON)
add_dependencies(Renderer CompileShaders)