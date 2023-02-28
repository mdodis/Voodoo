#include "Mesh.h"

#include "Containers/Extras.h"

VertexInputInfo Vertex::get_input_info(IAllocator& allocator)
{
    VertexInputInfo result = {
        .bindings = alloc_slice<VkVertexInputBindingDescription, 1>(allocator),
        .attributes =
            alloc_slice<VkVertexInputAttributeDescription, 3>(allocator),
    };

    result.bindings[0] = {
        .binding   = 0,
        .stride    = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    result.attributes[0] = {
        .location = 0,
        .binding  = 0,
        .format   = VK_FORMAT_R32G32B32_SFLOAT,
        .offset   = OFFSET_OF(Vertex, position),
    };

    result.attributes[1] = {
        .location = 1,
        .binding  = 0,
        .format   = VK_FORMAT_R32G32B32_SFLOAT,
        .offset   = OFFSET_OF(Vertex, normal),
    };

    result.attributes[2] = {
        .location = 2,
        .binding  = 0,
        .format   = VK_FORMAT_R32G32B32_SFLOAT,
        .offset   = OFFSET_OF(Vertex, color),
    };

    return result;
}
