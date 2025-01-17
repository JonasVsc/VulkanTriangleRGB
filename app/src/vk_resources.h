#pragma once

#include "vk_defines.h"

struct AllocatedBuffer 
{
    VkBuffer buffer;
    VmaAllocation allocation;
};


namespace vkrsc 
{
    AllocatedBuffer create_buffer(VmaAllocator allocator, size_t buffer_size, VkBufferUsageFlags buffer_usage, VmaMemoryUsage memory_usage, VmaAllocationCreateFlags memory_flags);
}