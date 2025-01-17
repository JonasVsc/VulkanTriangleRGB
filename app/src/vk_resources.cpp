#include "pre-compiled-header.h"
#include "vk_resources.h"

AllocatedBuffer vkrsc::create_buffer(VmaAllocator allocator, size_t buffer_size, VkBufferUsageFlags buffer_usage, VmaMemoryUsage memory_usage, VmaAllocationCreateFlags memory_flags)
{
    AllocatedBuffer new_buffer;

	VkBufferCreateInfo buffer_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = buffer_size,
		.usage = buffer_usage
	};

	VmaAllocationCreateInfo alloc_info = {
		.flags = memory_flags,
		.usage = memory_usage
	};

	VK_CHECK(vmaCreateBuffer(allocator, &buffer_info, &alloc_info, &new_buffer.buffer, &new_buffer.allocation, nullptr));
	
	return new_buffer;
}
