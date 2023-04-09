#pragma once
#include <glm/glm.hpp>

#include "VulkanCommon.h"

struct ImmediateDrawQueue {
    struct Vertex {
        glm::vec3 position;
    };

    void init(VkDevice device, VkRenderPass render_pass);
    void deinit();

    void submit_cube()
};