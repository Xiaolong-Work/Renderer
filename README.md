# Renderer

A modern renderer supporting rasterization, path tracing, Vulkan-based rasterization, and Vulkan compute shader-based path tracing.

## Key Features

- **Rasterization Rendering**: Efficient CPU/GPU rasterization pipeline
- **Path Tracing**: Classic Monte Carlo light transport simulation
- **Vulkan Rasterization**: High-performance rendering using Vulkan API
- **Vulkan Compute Shader Path Tracing**: Optimized path tracing with compute shaders

## Setup

### 1. Install Vulkan SDK

Download and install the [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) from the official Vulkan website. Ensure the environment variables are correctly configured.

### 2. Clone the Repository

This project uses Git submodules for dependency management. Clone it using:

```
git clone --recursive https://github.com/Xiaolong-Work/Renderer.git
```

If you already cloned the repository without submodules, run:

```
git submodule update --init --recursive
```

### 3. Open the Project in Visual Studio

1. Open **Visual Studio (2022 recommended)**
2. Click **"Open a Local Folder"** and select the **Renderer** project root directory
3. Wait for **CMake configuration** to complete
4. Select **x64-Debug** or **x64-Release** configuration
5. Click **"Build"**, then **"Run"**

## Results

| SPP = 1                                                      | SPP = 16                                                     | SPP = 512                                                    |
| ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ |
| ![img](https://raw.githubusercontent.com/Xiaolong-Work/Renderer/refs/heads/main/results/cornell-box_spp_1_depth_5_cpu.bmp) | ![img](https://raw.githubusercontent.com/Xiaolong-Work/Renderer/refs/heads/main/results/cornell-box_spp_16_depth_5_cpu.bmp) | ![img](https://raw.githubusercontent.com/Xiaolong-Work/Renderer/refs/heads/main/results/cornell-box_spp_512_depth_5_cpu.bmp) |

