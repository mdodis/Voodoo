#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Containers/Map.h"
#include "DeletionQueue.h"
#include "Memory/Base.h"
#include "Mesh.h"
#include "VulkanCommon.h"
#include "Window.h"
#include "vk_mem_alloc.h"

struct MeshPushConstants {
    Vec4      data;
    glm::mat4 transform;
};

struct AllocatedImage {
    VkImage       image;
    VmaAllocation allocation;
};

struct Texture {
    AllocatedImage image;
    VkImageView    view;
};

struct Material {
    VkPipeline       pipeline        = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
};

static _inline bool operator==(Material& left, Material& right)
{
    return left.pipeline == right.pipeline;
}

struct RenderObject {
    Mesh*     mesh;
    Material* material;
    glm::mat4 transform;
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
};

struct UploadContext {
    VkFence         fnc_upload;
    VkCommandPool   pool;
    VkCommandBuffer buffer;
};

struct Engine {
    Window*     window;
    Input*      input;
    bool        validation_layers = false;
    IAllocator& allocator;

    static constexpr int num_overlap_frames = 2;
    bool                 is_initialized     = false;
    VkExtent2D           extent             = {0, 0};

    // Rendering Objects

    VkInstance                 instance;
    VkSurfaceKHR               surface;
    VkPhysicalDevice           physical_device;
    VkPhysicalDeviceProperties physical_device_properties;
    VkDevice                   device;
    VkSwapchainKHR             swap_chain;
    VkFormat                   swap_chain_image_format;
    TArray<VkImage>            swap_chain_images;
    TArray<VkImageView>        swap_chain_image_views;
    VkRenderPass               render_pass;
    TArray<VkFramebuffer>      framebuffers;
    VkPipelineLayout           triangle_layout;
    VkPipeline                 pipeline;
    VmaAllocator               vmalloc;
    VkDescriptorPool           descriptor_pool;
    VkDescriptorSetLayout      global_set_layout;
    VkDescriptorSetLayout      object_set_layout;

    FrameData frames[num_overlap_frames];

    // Depth Buffer
    AllocatedImage depth_image;
    VkImageView    depth_image_view;
    VkFormat       depth_image_format = VK_FORMAT_D32_SFLOAT;

    // Statistics
    u32 frame_num = 0;

    DeletionQueue main_deletion_queue;
    DeletionQueue swap_chain_deletion_queue;

    // Scene Management
    TMap<Str, Mesh>      meshes;
    TMap<Str, Material>  materials;
    TMap<Str, Texture>   textures;
    TArray<RenderObject> render_objects;

    /** GPU Global Instance Data */
    struct {
        AllocatedBuffer buffer;
        u32             total_buffer_size;
    } global;

    UploadContext upload;

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

    struct {
        glm::vec3 position        = {0, 0, -5};
        glm::quat rotation        = {1, 0, 0, 0};
        glm::vec3 velocity        = {0, 0, 0};
        glm::vec3 input_direction = {0, 0, 0};
        glm::vec3 move_velocity   = {0, 0, 0};
        float     acceleration    = 0.05f;
        float     yaw = 0.f, pitch = 0.f;
        bool      has_focus = false;
    } debug_camera;

    void init();
    void deinit();
    void draw();

    /**
     * Used to update debug camera
     */
    void           update();
    VkShaderModule load_shader(Str path);
    Material*      create_material(
             VkPipeline pipeline, VkPipelineLayout layout, Str id);
    Mesh*                            get_mesh(Str id);
    Material*                        get_material(Str id);
    void                             upload_mesh(Mesh& mesh);
    Result<AllocatedImage, VkResult> upload_image_from_file(const char* path);

private:
    void init_imgui();
    void init_default_renderpass();
    void init_pipelines();
    void init_framebuffers();
    void init_descriptors();
    void init_sync_objects();
    void init_default_meshes();
    void init_default_images();
    void init_commands();
    void init_input();
    void recreate_swapchain();

    void on_resize_presentation();

    Result<AllocatedBuffer, VkResult> create_buffer(
        size_t             alloc_size,
        VkBufferUsageFlags usage,
        VmaMemoryUsage     memory_usage);

    FrameData& get_current_frame();

    size_t pad_uniform_buffer_size(size_t original_size);

    using ImmediateSubmitDelegate = Delegate<void, VkCommandBuffer>;

    FORWARD_DELEGATE_LAMBDA_TEMPLATE()
    void immediate_submit_lambda(
        FORWARD_DELEGATE_LAMBDA_SIG(ImmediateSubmitDelegate))
    {
        ImmediateSubmitDelegate delegate;
        FORWARD_DELEGATE_LAMBDA_BODY(delegate);
        immediate_submit(std::move(delegate));
    }

    void immediate_submit(ImmediateSubmitDelegate&& submit_delegate);

    // Debug Camera
    void on_debug_camera_forward();
    void on_debug_camera_back();
    void on_debug_camera_right();
    void on_debug_camera_left();
    void on_debug_camera_up();
    void on_debug_camera_down();
    void on_debug_camera_toggle_control();
    void on_debug_camera_mousex(float value);
    void on_debug_camera_mousey(float value);
    void debug_camera_update_rotation();
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
