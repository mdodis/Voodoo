#pragma once
#include "Math/CG.h"
#include "Result.h"
#include "VulkanCommon.h"
#include "vk_mem_alloc.h"

struct AllocatedBuffer {
    VkBuffer      buffer;
    VmaAllocation allocation;
};

struct VertexInputInfo {
    Slice<VkVertexInputBindingDescription>   bindings;
    Slice<VkVertexInputAttributeDescription> attributes;
    VkPipelineVertexInputStateCreateFlags    flags = 0;
};

struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec3 color;

    static VertexInputInfo get_input_info(IAllocator& allocator);
};

struct Mesh {
    Slice<Vertex>   vertices;
    AllocatedBuffer gpu_buffer;

    static Result<Mesh, Str> load_from_file(
        IAllocator& allocator, const char* path);
};