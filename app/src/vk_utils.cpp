#include "pre-compiled-header.h"
#include "vk_utils.h"

/**
 * @brief Load shader file in Vulkan Shader Module
 * @param device The vulkan device
 * @param file_path The shader file path
 */
VkShaderModule vkutil::load_shader_module(VkDevice device, const char *file_path)
{
    std::ifstream file(file_path, std::ios::ate | std::ios::binary);
    if(!file.is_open())
        throw std::runtime_error("Failed to open file");

    size_t file_size = static_cast<size_t>(file.tellg());

    std::vector<uint32_t> buffer(file_size / sizeof(uint32_t));
    file.seekg(0);

    file.read((char*)buffer.data(), file_size);
    file.close();

    VkShaderModuleCreateInfo module_info = {
        .sType 	  = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = file_size,
        .pCode 	  = buffer.data()
    };

	VkShaderModule shader_module;
    vkCreateShaderModule(device, &module_info, nullptr, &shader_module);

	return shader_module;
}

/**
 * @brief Transitions an image layout in a Vulkan command buffer.
 * @param cmd The command buffer to record the barrier into.
 * @param image The Vulkan image to transition.
 * @param oldLayout The current layout of the image.
 * @param newLayout The desired new layout of the image.
 * @param srcAccessMask The source access mask, specifying which access types are being transitioned from.
 * @param dstAccessMask The destination access mask, specifying which access types are being transitioned to.
 * @param srcStage The pipeline stage that must happen before the transition.
 * @param dstStage The pipeline stage that must happen after the transition.
 */
void vkutil::transition_image_layout(
    VkCommandBuffer cmd,
    VkImage image,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkAccessFlags2 srcAccessMask,
    VkAccessFlags2 dstAccessMask,
    VkPipelineStageFlags2 srcStage,
    VkPipelineStageFlags2 dstStage)
{
    // Initialize the VkImageMemoryBarrier2 structure
	VkImageMemoryBarrier2 image_barrier{
	    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,

	    // Specify the pipeline stages and access masks for the barrier
	    .srcStageMask  = srcStage,             // Source pipeline stage mask
	    .srcAccessMask = srcAccessMask,        // Source access mask
	    .dstStageMask  = dstStage,             // Destination pipeline stage mask
	    .dstAccessMask = dstAccessMask,        // Destination access mask

	    // Specify the old and new layouts of the image
	    .oldLayout = oldLayout,        // Current layout of the image
	    .newLayout = newLayout,        // Target layout of the image

	    // We are not changing the ownership between queues
	    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

	    // Specify the image to be affected by this barrier
	    .image = image,

	    // Define the subresource range (which parts of the image are affected)
	    .subresourceRange = {
	        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,        // Affects the color aspect of the image
	        .baseMipLevel   = 0,                                // Start at mip level 0
	        .levelCount     = 1,                                // Number of mip levels affected
	        .baseArrayLayer = 0,                                // Start at array layer 0
	        .layerCount     = 1                                 // Number of array layers affected
	    }};

	// Initialize the VkDependencyInfo structure
	VkDependencyInfo dependency_info{
	    .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
	    .dependencyFlags         = 0,                    // No special dependency flags
	    .imageMemoryBarrierCount = 1,                    // Number of image memory barriers
	    .pImageMemoryBarriers    = &image_barrier        // Pointer to the image memory barrier(s)
	};

	// Record the pipeline barrier into the command buffer
	vkCmdPipelineBarrier2(cmd, &dependency_info);
}
