cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

# Enable hot reload for the MSVC compiler
if (MSVC)
    add_compile_options(
        "$<$<CONFIG:Debug>:/ZI>"
        "$<$<CONFIG:Debug>:/hotpatch>"
        "$<$<CONFIG:Debug>:/MP>"
    )
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /INCREMENTAL:NO")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /INCREMENTAL:NO")
endif()

# Add executable file
project (Renderer LANGUAGES CXX)
add_executable(Renderer src/main.cpp)

# Setting the C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set_target_properties(Renderer PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
)

# Add include file
find_package(Vulkan REQUIRED)
include_directories(
	${Vulkan_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/external/glfw/include
	${CMAKE_SOURCE_DIR}/external/glm
)

# Add source directory
add_subdirectory(shaders)
add_subdirectory(src/model)
add_subdirectory(src/vulkan_manager)
add_subdirectory(src/path_tracing)
add_subdirectory(src/rasterizer)
add_subdirectory(external/glm)
add_subdirectory(external/glfw)
add_subdirectory(external/nrd)

# Link library
target_link_libraries(Renderer PathTracing)
target_link_libraries(Renderer VulkanManager)
target_link_libraries(Renderer Rasterizer)
target_link_libraries(Renderer glfw)


# OpenMp support
if (MSVC)
    target_compile_options(PathTracing PRIVATE /openmp)
	target_compile_options(Rasterizer PRIVATE /openmp)
	target_compile_options(VulkanManager PRIVATE /openmp)
else()
	find_package(OpenMP REQUIRED)
    target_link_libraries(PathTracing PRIVATE OpenMP::OpenMP_CXX)
	 target_link_libraries(Rasterizer PRIVATE OpenMP::OpenMP_CXX)
endif()

# Macro Definition
target_compile_definitions(Renderer PRIVATE ROOT_DIR="${CMAKE_SOURCE_DIR}")
target_compile_definitions(Model PRIVATE ROOT_DIR="${CMAKE_SOURCE_DIR}")
target_compile_definitions(PathTracing PRIVATE ROOT_DIR="${CMAKE_SOURCE_DIR}")
target_compile_definitions(Rasterizer PRIVATE ROOT_DIR="${CMAKE_SOURCE_DIR}")
target_compile_definitions(VulkanManager PRIVATE ROOT_DIR="${CMAKE_SOURCE_DIR}")
target_compile_definitions(VulkanManager PRIVATE SHADERS_DIR="${CMAKE_BINARY_DIR}/shaders/")

set(CMAKE_VERBOSE_MAKEFILE ON)
add_dependencies(Renderer CompileShaders)

if (MSVC)
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} /Zi)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi /DEBUG")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG")
endif()

target_include_directories(Renderer PUBLIC
    external/imgui
    external/imgui/backends
)

target_sources(Renderer PUBLIC
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
	external/imgui/backends/imgui_impl_vulkan.cpp
)