#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vk_mem_alloc.h>
#include <vulkan/vk_enum_string_helper.h>
#include <fmt/core.h>


#define VK_CHECK(x)                                                        \
do 																           \
{                                                                          \
	VkResult err = x;                                                      \
	if (err) {                                                             \
			fmt::print("Detected Vulkan error: {}", string_VkResult(err)); \
		abort();                                                           \
	}                                                                      \
} while (0)