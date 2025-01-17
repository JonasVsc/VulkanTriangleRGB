#include "pre-compiled-header.h"
#include "vk_mesh.h"

VertexInputDescription Vertex::get_vertex_description()
{
    VertexInputDescription description;

    VkVertexInputBindingDescription binding0 = {
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    description.bindings.push_back(binding0);

    VkVertexInputAttributeDescription attribute0 = {
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, position)
    };

    VkVertexInputAttributeDescription attribute1 = {
        .location = 1,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, normal)
    };

    VkVertexInputAttributeDescription attribute2 = {
        .location = 2,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, color)
    };

    description.attributes.push_back(attribute0);
    description.attributes.push_back(attribute1);
    description.attributes.push_back(attribute2);

    return description;
}
