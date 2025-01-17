cmake_minimum_required(VERSION 3.28)
project(application)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_BUILD_TYPE Debug)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
endif()


add_executable(pseudo3d src/main.cpp
    "src/configurations.h"     "src/pre-compiled-header.h"
    "src/vk_defines.h"
    "src/vk_defines.cpp"
    "src/vk_utils.h"                       
    "src/vk_utils.cpp"                       
    "src/vk_engine.h"                       
    "src/vk_engine.cpp"
    "src/vk_resources.h"
    "src/vk_resources.cpp"
    "src/vk_mesh.h"
    "src/vk_mesh.cpp"
)

target_precompile_headers(pseudo3d PRIVATE "src/pre-compiled-header.h")
target_link_libraries(pseudo3d glfw glm ${VULKAN_SDK}/Lib/vulkan-1.lib VulkanMemoryAllocator fmt imgui)
target_include_directories(pseudo3d SYSTEM PRIVATE include src glfw ${VULKAN_SDK}/Include ../vendor/VulkanMemoryAllocator/include ../vendor)



file(COPY ${CMAKE_SOURCE_DIR}/app/assets DESTINATION ${CMAKE_BINARY_DIR}/app)