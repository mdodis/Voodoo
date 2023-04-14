#pragma once
#include <glm/glm.hpp>

#include "Containers/Array.h"
#include "EngineTypes.h"

struct ImmediateDrawQueue {
    struct Vertex {
        glm::vec3 position;
    };

    struct GPUCameraData {
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 viewproj;
    };

    struct GPUObjectData {
        glm::mat4 matrix;
        glm::vec4 color;
        glm::vec4 unused_0;
        glm::vec4 unused_1;
        glm::vec4 unused_2;
    };

    struct Box {
        glm::vec3 center;
        glm::vec3 extents;
    };

    struct Cylinder {
        glm::vec3 center;
        glm::vec3 extents;
        glm::vec3 forward;
    };

    void init(
        VkDevice                      device,
        VkRenderPass                  render_pass,
        VMA&                          vma,
        struct DescriptorLayoutCache* cache,
        struct DescriptorAllocator*   allocator);
    void deinit();

    void draw(VkCommandBuffer cmd, glm::mat4& view, glm::mat4& proj);
    void clear();

    _inline void box(glm::vec3 center, glm::vec3 extents)
    {
        boxes.add(Box{center, extents});
    }

    _inline void cylinder(glm::vec3 center, glm::vec3 forward)
    {
        Cylinder c = {
            .center  = center,
            .extents = glm::vec3(1.0f, 1.0f, 1.0f),
            .forward = forward,
        };

        cylinders.add(c);
    }

    TArray<Box>      boxes{&System_Allocator};
    TArray<Cylinder> cylinders{&System_Allocator};

    AllocatedBuffer       object_buffer;
    AllocatedBuffer       global_buffer;
    AllocatedBuffer       cube_buffer;
    AllocatedBuffer       cylinder_buffer;
    VMA*                  pvma;
    VkDevice              device;
    VkPipeline            pipeline;
    VkPipelineLayout      pipeline_layout;
    VkDescriptorSet       global_set;
    VkDescriptorSetLayout global_set_layout;
    VkDescriptorSet       object_set;
    VkDescriptorSetLayout object_set_layout;

    static constexpr int Num_Overlap_Frames = 2;
    static constexpr int Max_Objects        = 100;
};