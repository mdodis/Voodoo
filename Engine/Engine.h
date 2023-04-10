#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "AssetLibrary/AssetLibrary.h"
#include "Containers/Map.h"
#include "Core/DeletionQueue.h"
#include "DescriptorBuilder.h"
#include "EngineTypes.h"
#include "ImmediateDrawQueue.h"
#include "Memory/Base.h"
#include "Mesh.h"
#include "RenderObject.h"
#include "VulkanCommon.h"
#include "Window/Window.h"
#include "vk_mem_alloc.h"

using namespace win;

struct Engine {
    Window*     window;
    Input*      input;
    bool        validation_layers = false;
    IAllocator& allocator;

    static constexpr int num_overlap_frames = 2;
    bool                 is_initialized     = false;
    VkExtent2D           extent             = {0, 0};

    VMA vma;

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
    VkDescriptorPool           descriptor_pool;
    VkDescriptorSetLayout      global_set_layout;
    VkDescriptorSetLayout      object_set_layout;
    VkDescriptorSetLayout      texture_set_layout;

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
    TMap<Str, Mesh>     meshes;
    TMap<Str, Material> materials;
    TMap<Str, Texture>  textures;
    Slice<RenderObject> render_objects;

    // Immediate
    ImmediateDrawQueue imm;

    /** GPU Global Instance Data */
    struct {
        AllocatedBuffer buffer;
        u32             total_buffer_size;
    } global;

    UploadContext upload;

    Mesh triangle_mesh;
    Mesh monke_mesh;  // monke

    struct {
        DescriptorAllocator   allocator;
        DescriptorLayoutCache cache;
    } desc;

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
    void imgui_new_frame();

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
    Result<AllocatedImage, VkResult> upload_image_from_file(Str path);

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

    FrameData& get_current_frame();

    size_t pad_uniform_buffer_size(size_t original_size);

    using ImmediateSubmitDelegate = Delegate<void, VkCommandBuffer>;

    FORWARD_DELEGATE_LAMBDA_TEMPLATE()
    void immediate_submit_lambda(
        FORWARD_DELEGATE_LAMBDA_SIG(ImmediateSubmitDelegate))
    {
        ImmediateSubmitDelegate delegate =
            FORWARD_DELEGATE_LAMBDA_CREATE(ImmediateSubmitDelegate);
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
