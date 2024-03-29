#pragma once

#include "Mesh.h"
#include "VulkanCommon/VulkanCommon.h"

struct Material {
    VkPipeline       pipeline        = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    VkDescriptorSet  texture_set     = {VK_NULL_HANDLE};
};

static _inline bool operator==(Material& left, Material& right)
{
    return left.pipeline == right.pipeline;
}

struct RenderObject {
    Mesh*                    mesh;
    struct MaterialInstance* material;
    glm::mat4                transform;
};