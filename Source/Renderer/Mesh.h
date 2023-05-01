#pragma once
#include <glm/glm.hpp>

#include "AssetLibrary/AssetLibrary.h"
#include "Result.h"
#include "VMA.h"
#include "VulkanCommon/VulkanCommon.h"
#include "vk_mem_alloc.h"

struct Vertex {
    glm::vec3              position;
    glm::vec3              normal;
    glm::vec3              color;
    glm::vec2              uv;
    static VertexInputInfo get_input_info(Allocator& allocator);
};

struct Mesh {
    Slice<Vertex>   vertices;
    Slice<u32>      indices;
    AllocatedBuffer gpu_buffer;
    AllocatedBuffer gpu_index_buffer;

    static Result<Mesh, EAssetLoadError> load_from_asset(
        Allocator& allocator, Str path);
};

static _inline bool operator==(Mesh& left, Mesh& right)
{
    return left.gpu_buffer.buffer == right.gpu_buffer.buffer;
}