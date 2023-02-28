#pragma once
#include "DeletionQueue.h"
#include "Memory/Base.h"
#include "Mesh.h"
#include "VulkanCommon.h"
#include "Window.h"
#include "vk_mem_alloc.h"

struct MeshPushConstants {
    Vec4 data;
    Mat4 transform;
};

struct Engine {
    Window*     window;
    bool        validation_layers = false;
    IAllocator& allocator;

    bool       is_initialized = false;
    VkExtent2D extent         = {0, 0};

    // Rendering Objects

    VkInstance            instance;
    VkSurfaceKHR          surface;
    VkPhysicalDevice      physical_device;
    VkDevice              device;
    VkSwapchainKHR        swap_chain;
    VkFormat              swap_chain_image_format;
    TArray<VkImage>       swap_chain_images;
    TArray<VkImageView>   swap_chain_image_views;
    VkCommandPool         cmd_pool;
    VkCommandBuffer       main_cmd_buffer;
    VkRenderPass          render_pass;
    TArray<VkFramebuffer> framebuffers;
    VkPipelineLayout      triangle_layout;
    VkPipeline            pipeline;
    VmaAllocator          vmalloc;
    // Sync Objects

    VkSemaphore sem_render, sem_present;
    VkFence     fnc_render;

    // Statistics
    u32 frame_num = 0;

    DeletionQueue main_deletion_queue;

    Mesh triangle_mesh;

    struct {
        u32     family;
        VkQueue queue;
    } graphics;

    struct {
        u32     family;
        VkQueue queue;
    } presentation;

    void           init();
    void           deinit();
    void           draw();
    VkShaderModule load_shader(Str path);
    void           upload_mesh(Mesh& mesh);

private:
    void init_default_renderpass();
    void init_pipelines();
    void init_framebuffers();
    void init_sync_objects();
    void init_default_meshes();
};

struct PipelineBuilder {
    PipelineBuilder(IAllocator& allocator) : shader_stages(&allocator)
    {
        init_defaults();
    }

    TArray<VkPipelineShaderStageCreateInfo> shader_stages;
    VkPipelineVertexInputStateCreateInfo    vertex_input_info;
    VkPipelineInputAssemblyStateCreateInfo  asm_create_info;
    VkViewport                              viewport;
    VkRect2D                                scissor;
    VkPipelineRasterizationStateCreateInfo  rasterizer_state;
    VkPipelineMultisampleStateCreateInfo    multisample_state;
    VkPipelineColorBlendAttachmentState     color_blend_attachment_state;
    VkPipelineLayout                        layout;
    VkRenderPass                            render_pass;

    PipelineBuilder& add_shader_stage(
        VkShaderStageFlagBits stage, VkShaderModule module);
    PipelineBuilder& set_primitive_topology(VkPrimitiveTopology topology);
    PipelineBuilder& set_polygon_mode(VkPolygonMode mode);
    PipelineBuilder& set_viewport(
        float x, float y, float w, float h, float min_d, float max_d);
    PipelineBuilder& set_scissor(i32 x, i32 y, u32 w, u32 h);
    PipelineBuilder& set_render_pass(VkRenderPass render_pass);
    PipelineBuilder& set_layout(VkPipelineLayout layout);
    PipelineBuilder& set_vertex_input_bindings(
        Slice<VkVertexInputBindingDescription> bindings);
    PipelineBuilder& set_vertex_input_attributes(
        Slice<VkVertexInputAttributeDescription> attributes);
    PipelineBuilder& set_vertex_input_info(VertexInputInfo& info);

    void                         init_defaults();
    Result<VkPipeline, VkResult> build(VkDevice device);
};