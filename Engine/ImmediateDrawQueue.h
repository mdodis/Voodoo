#pragma once
#include <glm/glm.hpp>

#include "VulkanCommon.h"

struct ImmediateDrawQueue {
    struct Vertex {
        glm::vec3 position;
    };

    void init(
        VkDevice                     device,
        VkRenderPass                 render_pass,
        Slice<VkDescriptorSetLayout> descriptors);
    void deinit();

    void submit_cube();

    VkPipeline       pipeline;
    VkPipelineLayout pipeline_layout;
};