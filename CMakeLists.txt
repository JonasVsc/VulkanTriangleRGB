cmake_minimum_required(VERSION 3.28)
project(vklearn)

# set
set(VULKAN_SDK "C:\\VulkanSDK\\1.3.296.0")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define a biblioteca estática do Dear ImGui
add_library(imgui STATIC
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/backends/imgui_impl_glfw.cpp
    vendor/imgui/backends/imgui_impl_vulkan.cpp
)
# Inclui os diretórios de cabeçalhos do ImGui
target_include_directories(imgui PUBLIC
    vendor/imgui
    vendor/imgui/backends
    ${VULKAN_SDK}/Include
)
target_link_libraries(imgui PUBLIC glfw ${VULKAN_SDK}/Lib/vulkan-1.lib)

# Third Parthies
add_subdirectory(vendor/glfw)
add_subdirectory(vendor/VulkanMemoryAllocator)
add_subdirectory(vendor/glm)
add_subdirectory(vendor/fmt)

# Projects
add_subdirectory(app)
