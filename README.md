# Vulkan Triangle Renderer with Dynamic Rendering
This project demonstrates how to render a triangle using Vulkan and allows for dynamic vertex color changes via ImGui.

## Libraries Used
- Vulkan Memory Allocator: Simplifies Vulkan memory management.
- ImGui: Provides an easy-to-use graphical interface to modify the vertex colors interactively.
- GLM: A library for mathematics, particularly for transformations and projections.
- GLFW: Used for creating windows and handling user input.
- FMT: A modern, fast, and type-safe formatting library.

## Applied Concepts
This project makes use of the following Vulkan concepts:

**Dynamic Rendering:** unuses the creation of renderpasses and framebuffers

**Staging Buffer:** Efficiently transfers data from CPU to GPU memory for vertex data.

**Push Constants:** A mechanism to pass small amounts of data to shaders during pipeline execution.

**Graphics Pipeline:** Defines how the GPU processes the rendering of objects, including shaders, blending, and rasterization.

**Fences and Semaphores:** Synchronization primitives to manage execution order between CPU and GPU tasks.

**Engine Architecture:** A custom engine structure to handle Vulkan initialization, resource management, and rendering logic.

# Features
- Render a triangle on the screen using Vulkan.
- Dynamically change the color of the triangle's vertices using ImGui.
- Implement Vulkan concepts like staging buffers, push constants, and synchronization mechanisms.

Prerequisites
To compile and run this project, you need the following installed:
Vulkan SDK: Make sure the Vulkan SDK is installed and properly set up on your system.
...
