cmake_minimum_required(VERSION 3.10)

project(ShaderCompilation)

find_package(Vulkan REQUIRED)
message(STATUS "Vulkan include dirs: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan lib dirs: ${Vulkan_LIBRARIES}")

set(VULKAN_BIN_DIR "${Vulkan_INCLUDE_DIRS}/../Bin")
message(STATUS "Vulkan SDK Bin Directory: ${VULKAN_BIN_DIR}")

set(SHADER_COMPILING "${VULKAN_BIN_DIR}/glslc.exe")


# Supported shader types
set(SHADER_EXTS "vert" "frag" "geom" "comp")

# Store all .spv output files
set(SPV_FILES "")

# Iterate over all shader types
foreach(EXT ${SHADER_EXTS})
    file(GLOB SHADERS "*.${EXT}")

    foreach(SHADER ${SHADERS})
        get_filename_component(SHADER_NAME ${SHADER} NAME_WE)

        # Generates a file name in the format _ext.spv
        set(SPV_OUTPUT "${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}_${EXT}.spv")

        add_custom_command(
            OUTPUT ${SPV_OUTPUT}
            COMMAND ${SHADER_COMPILING} -O --target-env=vulkan1.3 ${SHADER} -o ${SPV_OUTPUT}
            DEPENDS ${SHADER}
            COMMENT "Compiling ${SHADER} to ${SPV_OUTPUT}"
            VERBATIM
        )

        list(APPEND SPV_FILES ${SPV_OUTPUT})
    endforeach()
endforeach()


# Create CMake targets to use all SPV files as build targets
add_custom_target(CompileShaders ALL DEPENDS ${SPV_FILES})
