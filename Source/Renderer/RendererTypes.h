#pragma once
#include <glm/glm.hpp>

#include "VMA.h"
#include "VulkanCommon/VulkanCommon.h"

struct MeshPushConstants {
    glm::vec4 data;
    glm::mat4 transform;
};

struct Texture {
    AllocatedImage image;
    VkImageView    view;
};

struct GPUCameraData {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewproj;
};

struct GPUSceneData {
    glm::vec4 fog_color;  // w: exponent
    glm::vec4 ambient_color;
};

struct GPUObjectData {
    glm::mat4 model;
};

struct GPUGlobalInstanceData {
    GPUCameraData camera;
    GPUSceneData  scene;
};

struct FrameData {
    VkSemaphore     sem_present, sem_render;
    VkFence         fnc_render;
    VkCommandPool   pool;
    VkCommandBuffer main_cmd_buffer;
    VkDescriptorSet global_descriptor;
    AllocatedBuffer object_buffer;
    VkDescriptorSet object_descriptor;
    AllocatedBuffer indirect_buffer;
};

struct UploadContext {
    VkFence         fnc_upload;
    VkCommandPool   pool;
    VkCommandBuffer buffer;
};

struct IndirectBatch {
    struct Mesh*             mesh;
    struct MaterialInstance* material;
    u32                      first;
    u32                      count;
};