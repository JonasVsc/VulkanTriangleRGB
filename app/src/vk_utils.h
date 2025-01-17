#pragma once

#include "vk_defines.h"

namespace vkutil
{

    VkShaderModule load_shader_module(VkDevice device, const char *file_path);

    void transition_image_layout(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags2 srcAccessMask, VkAccessFlags2 dstAccessMask, VkPipelineStageFlags2 srcStage, VkPipelineStageFlags2 dstStage);

};