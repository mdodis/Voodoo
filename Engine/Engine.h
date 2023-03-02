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

struct AllocatedImage {
    VkImage       image;
    VmaAllocation allocation;
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

    // Depth Buffer
    AllocatedImage depth_image;
    VkImageView    depth_image_view;
    VkFormat       depth_image_format = VK_FORMAT_D32_SFLOAT;

    // Sync Objects

    VkSemaphore sem_render, sem_present;
    VkFence     fnc_render;

    // Statistics
    u32 frame_num = 0;

    DeletionQueue main_deletion_queue;
    DeletionQueue swap_chain_deletion_queue;

    Mesh triangle_mesh;
    Mesh monke_mesh;  // monke

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

    void recreate_swapchain();

    void on_resize_presentation();
};

struct PipelineBuilder {
    PipelineBuilder(IAllocator& allocator)
        : shader_stages(&allocator), dynamic_states(&allocator)
    {
        init_defaults();
    }

    TArray<VkPipelineShaderStageCreateInfo> shader_stages;
    TArray<VkDynamicState>                  dynamic_states;
    VkPipelineVertexInputStateCreateInfo    vertex_input_info;
    VkPipelineInputAssemblyStateCreateInfo  asm_create_info;
    VkViewport                              viewport;
    VkRect2D                                scissor;
    VkPipelineRasterizationStateCreateInfo  rasterizer_state;
    VkPipelineDepthStencilStateCreateInfo   depth_stencil_state;
    VkPipelineMultisampleStateCreateInfo    multisample_state;
    VkPipelineColorBlendAttachmentState     color_blend_attachment_state;
    VkPipelineDynamicStateCreateInfo        dynamic_state_info;
    VkPipelineLayout                        layout;
    VkRenderPass                            render_pass;

    bool has_depth_test  = false;
    bool has_depth_write = false;

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
    PipelineBuilder& set_depth_test(
        bool enabled, bool write, VkCompareOp compare_op);
    PipelineBuilder& add_dynamic_state(VkDynamicState dynamic_state);

    void                         init_defaults();
    Result<VkPipeline, VkResult> build(VkDevice device);
};