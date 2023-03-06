#include "Engine.h"

#include <glm/gtc/matrix_transform.hpp>

#include "Memory/Extras.h"
#include "backends/imgui_impl_vulkan.h"

static auto Validation_Layers = arr<const char*>("VK_LAYER_KHRONOS_validation");
static auto Device_Extensions =
    arr<const char*>(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

static VK_PROC_DEBUG_CALLBACK(debug_callback)
{
    if (severity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        print(LIT("Vulkan: {}\n"), Str(callback_data->pMessage));
    }
    return VK_FALSE;
}

// Initialization

void Engine::init()
{
    swap_chain_images.alloc      = &allocator;
    swap_chain_image_views.alloc = &allocator;
    framebuffers.alloc           = &allocator;
    main_deletion_queue          = DeletionQueue(allocator);
    swap_chain_deletion_queue    = DeletionQueue(allocator);
    render_objects.alloc         = &allocator;
    meshes.init(allocator);
    materials.init(allocator);
    CREATE_SCOPED_ARENA(&allocator, temp_alloc, KILOBYTES(4));

    Slice<const char*> required_window_ext = win32_get_required_extensions();
    window->on_resized.bind_raw(this, &Engine::on_resize_presentation);

    // Instance
    {
        SAVE_ARENA(temp_alloc);

        VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = debug_callback,
            .pUserData       = 0,
        };

        VkApplicationInfo app_info = {
            .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName   = "VKX",
            .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
            .pEngineName        = "VKX",
            .engineVersion      = VK_MAKE_VERSION(0, 1, 0),
            .apiVersion         = VK_API_VERSION_1_3,
        };

        VkInstanceCreateInfo create_info = {
            .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo        = &app_info,
            .enabledExtensionCount   = (u32)required_window_ext.count,
            .ppEnabledExtensionNames = required_window_ext.ptr,
        };

        if (validation_layers) {
            ASSERT(
                validation_layers_exist(temp_alloc, slice(Validation_Layers)));
            create_info.enabledLayerCount   = Validation_Layers.count();
            create_info.ppEnabledLayerNames = Validation_Layers.elements;
            create_info.pNext               = &debug_create_info;
        }

        VK_CHECK(vkCreateInstance(&create_info, 0, &instance));
    }

    // Create surface
    surface = window->create_surface(instance).unwrap();

    // Pick physical device
    {
        SAVE_ARENA(temp_alloc);

        PickPhysicalDeviceInfo pick_info = {
            .device_extentions    = slice(Device_Extensions),
            .prefered_device_type = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
            .surface              = surface,
        };
        physical_device =
            pick_physical_device(temp_alloc, instance, pick_info).unwrap();

        print_physical_device_queue_families(physical_device);
    }

    // Create logical device
    {
        SAVE_ARENA(temp_alloc);
        auto requirements = arr<CreateDeviceFamilyRequirement>(
            CreateDeviceFamilyRequirement{
                .flag_bits    = VK_QUEUE_GRAPHICS_BIT,
                .presentation = false,
            },
            CreateDeviceFamilyRequirement{
                .flag_bits    = (VkQueueFlagBits)0,
                .presentation = true,
            });

        VkPhysicalDeviceShaderDrawParametersFeatures shader_draw_params = {
            .sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES,
            .pNext                = 0,
            .shaderDrawParameters = VK_TRUE,
        };

        CreateDeviceWithQueuesInfo info = {
            .surface             = surface,
            .extensions          = slice(Device_Extensions),
            .family_requirements = slice(requirements),
            .next                = (void*)&shader_draw_params,
        };

        if (validation_layers) {
            info.validation_layers = slice(Validation_Layers);
        }
        device = create_device_with_queues(temp_alloc, physical_device, info)
                     .unwrap();

        graphics.family     = info.families[0];
        presentation.family = info.families[1];

        vkGetDeviceQueue(device, graphics.family, 0, &graphics.queue);
        vkGetDeviceQueue(device, presentation.family, 0, &presentation.queue);
    }

    // VM Allocator
    {
        VmaAllocatorCreateInfo create_info = {
            .physicalDevice = physical_device,
            .device         = device,
            .instance       = instance,
        };

        VK_CHECK(vmaCreateAllocator(&create_info, &vmalloc));
    }

    vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
    print(
        LIT("Min GPU Buffer Alignment: {}\n"),
        physical_device_properties.limits.minUniformBufferOffsetAlignment);

    recreate_swapchain();

    init_commands();
    init_input();
    init_default_renderpass();
    init_framebuffers();
    init_sync_objects();
    init_descriptors();
    init_pipelines();
    init_default_meshes();
    // init_imgui();

    is_initialized = true;
}

void Engine::init_commands()
{
    for (int i = 0; i < num_overlap_frames; ++i) {
        VkCommandPool   pool;
        VkCommandBuffer cmd_buffer;
        // Create graphics command pool
        {
            VkCommandPoolCreateInfo create_info = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                .queueFamilyIndex = graphics.family,
            };

            VK_CHECK(vkCreateCommandPool(device, &create_info, 0, &pool));

            main_deletion_queue.add(
                DeletionQueue::DeletionDelegate::create_lambda([this, i]() {
                    vkDestroyCommandPool(device, frames[i].pool, 0);
                }));
        }

        // Create main cmd buffer
        {
            VkCommandBufferAllocateInfo create_info = {
                .sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = pool,
                .level       = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
            };
            VK_CHECK(
                vkAllocateCommandBuffers(device, &create_info, &cmd_buffer));
        }

        frames[i].pool            = pool;
        frames[i].main_cmd_buffer = cmd_buffer;
    }

    // Upload context command pool
    {
        VkCommandPoolCreateInfo create_info = {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = graphics.family,
        };

        VK_CHECK(vkCreateCommandPool(device, &create_info, 0, &upload.pool));

        main_deletion_queue.add(DeletionQueue::DeletionDelegate::create_lambda(
            [this]() { vkDestroyCommandPool(device, upload.pool, 0); }));

        VkCommandBufferAllocateInfo cmd_create_info = {
            .sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = upload.pool,
            .level       = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        VK_CHECK(
            vkAllocateCommandBuffers(device, &cmd_create_info, &upload.buffer));
    }
}

void Engine::init_descriptors()
{
    auto descriptor_pool_sizes = arr<VkDescriptorPoolSize>(
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 20},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10});

    VkDescriptorPoolCreateInfo pool_create_info = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext         = 0,
        .flags         = 0,
        .maxSets       = 10,
        .poolSizeCount = descriptor_pool_sizes.count(),
        .pPoolSizes    = descriptor_pool_sizes.elements,
    };

    VK_CHECK(
        vkCreateDescriptorPool(device, &pool_create_info, 0, &descriptor_pool));

    // Global data at 0
    VkDescriptorSetLayoutBinding global_binding = descriptor_set_layout_binding(
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0);

    VkDescriptorSetLayoutBinding bindings[] = {global_binding};

    VkDescriptorSetLayoutCreateInfo set_info = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext        = 0,
        .flags        = 0,
        .bindingCount = ARRAY_COUNT(bindings),
        .pBindings    = bindings,
    };

    VK_CHECK(
        vkCreateDescriptorSetLayout(device, &set_info, 0, &global_set_layout));
    // Object descriptor set layout

    VkDescriptorSetLayoutBinding object_binding = descriptor_set_layout_binding(
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        VK_SHADER_STAGE_VERTEX_BIT,
        0);

    VkDescriptorSetLayoutCreateInfo set2_info = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext        = 0,
        .flags        = 0,
        .bindingCount = 1,
        .pBindings    = &object_binding,
    };

    VK_CHECK(
        vkCreateDescriptorSetLayout(device, &set2_info, 0, &object_set_layout));

    // global data buffer
    {
        /*
            Buffer
            [Camera, Scene, PAD]
            [Camera, Scene, PAD]
            [Camera, Scene, PAD]
        */

        global.total_buffer_size =
            (u32)pad_uniform_buffer_size(sizeof(GPUGlobalInstanceData)) *
            num_overlap_frames;

        global.buffer =
            create_buffer(
                global.total_buffer_size,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU)
                .unwrap();

        main_deletion_queue.add(
            DeletionQueue::DeletionDelegate::create_lambda([this]() {
                vmaDestroyBuffer(
                    vmalloc,
                    global.buffer.buffer,
                    global.buffer.allocation);
            }));
    }

    for (int i = 0; i < num_overlap_frames; ++i) {
        VkDescriptorSetAllocateInfo alloc_info = {
            .sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext          = 0,
            .descriptorPool = descriptor_pool,
            .descriptorSetCount = 1,
            .pSetLayouts        = &global_set_layout,
        };

        VK_CHECK(vkAllocateDescriptorSets(
            device,
            &alloc_info,
            &frames[i].global_descriptor));

        VkDescriptorSetAllocateInfo object_set_alloc_info = {
            .sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext          = 0,
            .descriptorPool = descriptor_pool,
            .descriptorSetCount = 1,
            .pSetLayouts        = &object_set_layout,
        };

        VK_CHECK(vkAllocateDescriptorSets(
            device,
            &object_set_alloc_info,
            &frames[i].object_descriptor));

        const int max_objects = 10000;
        frames[i].object_buffer =
            create_buffer(
                sizeof(GPUObjectData) * max_objects,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU)
                .unwrap();

        VkDescriptorBufferInfo global_instance_buffer_info = {
            .buffer = global.buffer.buffer,
            .offset = 0,
            .range  = sizeof(GPUGlobalInstanceData),
        };

        VkDescriptorBufferInfo object_buffer_info = {
            .buffer = frames[i].object_buffer.buffer,
            .offset = 0,
            .range  = sizeof(GPUObjectData) * max_objects,
        };

        VkWriteDescriptorSet global_instance_set_write = write_descriptor_set(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            frames[i].global_descriptor,
            &global_instance_buffer_info,
            0);

        VkWriteDescriptorSet object_set_write = write_descriptor_set(
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            frames[i].object_descriptor,
            &object_buffer_info,
            0);

        VkWriteDescriptorSet write_sets[] = {
            global_instance_set_write,
            object_set_write,
        };

        vkUpdateDescriptorSets(
            device,
            ARRAY_COUNT(write_sets),
            write_sets,
            0,
            0);

        main_deletion_queue.add(
            DeletionQueue::DeletionDelegate::create_lambda([this, i]() {
                vmaDestroyBuffer(
                    vmalloc,
                    frames[i].object_buffer.buffer,
                    frames[i].object_buffer.allocation);
            }));
    }

    main_deletion_queue.add(
        DeletionQueue::DeletionDelegate::create_lambda([this]() {
            vkDestroyDescriptorSetLayout(device, object_set_layout, 0);
            vkDestroyDescriptorSetLayout(device, global_set_layout, 0);
            vkDestroyDescriptorPool(device, descriptor_pool, 0);
        }));
}

void Engine::init_pipelines()
{
    // Load shaders
    VkShaderModule basic_shader_vert, basic_shader_frag;

    basic_shader_vert = load_shader(LIT("Shaders/Basic.vert.spv"));
    basic_shader_frag = load_shader(LIT("Shaders/Basic.frag.spv"));

    // Create layout
    {
        auto layouts =
            arr<VkDescriptorSetLayout>(global_set_layout, object_set_layout);

        VkPipelineLayoutCreateInfo create_info = {
            .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = layouts.count(),
            .pSetLayouts    = layouts.elements,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges    = 0,
        };

        VK_CHECK(
            vkCreatePipelineLayout(device, &create_info, 0, &triangle_layout));
    }

    CREATE_SCOPED_ARENA(&allocator, temp_alloc, KILOBYTES(1));

    VertexInputInfo vertex_input_info = Vertex::get_input_info(temp_alloc);

    pipeline =
        PipelineBuilder(temp_alloc)
            .add_shader_stage(VK_SHADER_STAGE_VERTEX_BIT, basic_shader_vert)
            .add_shader_stage(VK_SHADER_STAGE_FRAGMENT_BIT, basic_shader_frag)
            .add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT)
            .add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR)
            .set_primitive_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .set_polygon_mode(VK_POLYGON_MODE_FILL)
            .set_render_pass(render_pass)
            .set_layout(triangle_layout)
            .set_vertex_input_info(vertex_input_info)
            .set_depth_test(true, true, VK_COMPARE_OP_LESS)
            .build(device)
            .unwrap();

    main_deletion_queue.add(
        DeletionQueue::DeletionDelegate::create_lambda([this]() {
            vkDestroyPipeline(device, pipeline, 0);
            vkDestroyPipelineLayout(device, triangle_layout, 0);
        }));

    create_material(pipeline, triangle_layout, LIT("default.mesh"));

    vkDestroyShaderModule(device, basic_shader_vert, 0);
    vkDestroyShaderModule(device, basic_shader_frag, 0);
}

void Engine::init_default_renderpass()
{
    VkAttachmentDescription color_attachment = {
        .format         = swap_chain_image_format,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference color_attachment_ref = {
        .attachment = 0,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentDescription depth_attachment = {
        .format         = depth_image_format,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference depth_attachment_ref = {
        .attachment = 1,
        .layout     = depth_attachment.finalLayout,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount    = 1,
        .pColorAttachments       = &color_attachment_ref,
        .pDepthStencilAttachment = &depth_attachment_ref,
    };

    VkSubpassDependency dependency = {
        .srcSubpass    = VK_SUBPASS_EXTERNAL,
        .dstSubpass    = 0,
        .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkSubpassDependency depth_dependency = {
        .srcSubpass   = VK_SUBPASS_EXTERNAL,
        .dstSubpass   = 0,
        .srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    };

    VkAttachmentDescription attachments[2] = {
        color_attachment,
        depth_attachment,
    };

    VkSubpassDependency dependencies[2] = {
        dependency,
        depth_dependency,
    };

    VkRenderPassCreateInfo create_info = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 2,
        .pAttachments    = attachments,
        .subpassCount    = 1,
        .pSubpasses      = &subpass,
        .dependencyCount = 2,
        .pDependencies   = dependencies,
    };

    VK_CHECK(vkCreateRenderPass(device, &create_info, 0, &render_pass));

    main_deletion_queue.add(DeletionQueue::DeletionDelegate::create_lambda(
        [this]() { vkDestroyRenderPass(this->device, this->render_pass, 0); }));
}

void Engine::init_framebuffers()
{
    VkFramebufferCreateInfo create_info = {
        .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass      = render_pass,
        .attachmentCount = 2,
        .width           = extent.width,
        .height          = extent.height,
        .layers          = 1,
    };

    const u32 swapchain_image_count = (u32)swap_chain_images.size;
    framebuffers.init_range(swapchain_image_count);

    for (u32 i = 0; i < swapchain_image_count; ++i) {
        VkImageView attachments[2] = {
            swap_chain_image_views[i],
            depth_image_view,
        };
        create_info.pAttachments = attachments;
        VK_CHECK(
            vkCreateFramebuffer(device, &create_info, 0, &framebuffers[i]));

        swap_chain_deletion_queue.add(
            DeletionQueue::DeletionDelegate::create_lambda([this, i]() {
                vkDestroyFramebuffer(this->device, framebuffers[i], 0);
                vkDestroyImageView(this->device, swap_chain_image_views[i], 0);
            }));
    }
}

void Engine::init_sync_objects()
{
    for (int i = 0; i < num_overlap_frames; ++i) {
        VkFenceCreateInfo fnc_create_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };

        VK_CHECK(
            vkCreateFence(device, &fnc_create_info, 0, &frames[i].fnc_render));

        main_deletion_queue.add(DeletionQueue::DeletionDelegate::create_lambda(
            [this, i]() { vkDestroyFence(device, frames[i].fnc_render, 0); }));

        VkSemaphoreCreateInfo sem_create_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };

        VK_CHECK(vkCreateSemaphore(
            device,
            &sem_create_info,
            0,
            &frames[i].sem_present));
        VK_CHECK(vkCreateSemaphore(
            device,
            &sem_create_info,
            0,
            &frames[i].sem_render));

        main_deletion_queue.add(
            DeletionQueue::DeletionDelegate::create_lambda([this, i]() {
                vkDestroySemaphore(device, frames[i].sem_render, 0);
                vkDestroySemaphore(device, frames[i].sem_present, 0);
            }));
    }

    // Upload fence
    {
        VkFenceCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = 0,
        };
        VK_CHECK(vkCreateFence(device, &create_info, 0, &upload.fnc_upload));

        main_deletion_queue.add(DeletionQueue::DeletionDelegate::create_lambda(
            [this]() { vkDestroyFence(device, upload.fnc_upload, 0); }));
    }
}

void Engine::init_input()
{
    input->add_digital_continuous_action(
        LIT("debug.camera.fwd"),
        InputKey::W,
        InputDigitalContinuousAction::CallbackDelegate::create_raw(
            this,
            &Engine::on_debug_camera_forward));
    input->add_digital_continuous_action(
        LIT("debug.camera.back"),
        InputKey::S,
        InputDigitalContinuousAction::CallbackDelegate::create_raw(
            this,
            &Engine::on_debug_camera_back));

    input->add_digital_continuous_action(
        LIT("debug.camera.left"),
        InputKey::A,
        InputDigitalContinuousAction::CallbackDelegate::create_raw(
            this,
            &Engine::on_debug_camera_left));
    input->add_digital_continuous_action(
        LIT("debug.camera.right"),
        InputKey::D,
        InputDigitalContinuousAction::CallbackDelegate::create_raw(
            this,
            &Engine::on_debug_camera_right));

    input->add_digital_stroke_action(
        LIT("debug.camera.toggle_control"),
        InputKey::Num1,
        InputDigitalStrokeAction::CallbackDelegate::create_raw(
            this,
            &Engine::on_debug_camera_toggle_control));

    input->add_axis_motion_action(
        LIT("debug.camera.yaw"),
        InputAxis::MouseX,
        -1.0f,
        InputAxisMotionAction::CallbackDelegate::create_raw(
            this,
            &Engine::on_debug_camera_mousex));

    input->add_axis_motion_action(
        LIT("debug.camera.pitch"),
        InputAxis::MouseY,
        -1.0f,
        InputAxisMotionAction::CallbackDelegate::create_raw(
            this,
            &Engine::on_debug_camera_mousey));

    debug_camera_update_rotation();
}

void Engine::init_imgui()
{
    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000},
    };

    VkDescriptorPoolCreateInfo pool_info = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets       = 1000,
        .poolSizeCount = ARRAY_COUNT(pool_sizes),
        .pPoolSizes    = pool_sizes,
    };

    VkDescriptorPool imgui_pool;
    VK_CHECK(vkCreateDescriptorPool(device, &pool_info, nullptr, &imgui_pool));
    ImGui_ImplVulkan_InitInfo vk_init_info = {
        .Instance       = instance,
        .PhysicalDevice = physical_device,
        .Device         = device,
        .QueueFamily    = graphics.family,
        .Queue          = graphics.queue,
        .DescriptorPool = imgui_pool,
        .MinImageCount  = 3,
        .ImageCount     = 3,
        .MSAASamples    = VK_SAMPLE_COUNT_1_BIT,
    };
    ASSERT(ImGui_ImplVulkan_Init(&vk_init_info, render_pass));
}

// Other
FrameData& Engine::get_current_frame()
{
    return frames[frame_num % num_overlap_frames];
}

size_t Engine::pad_uniform_buffer_size(size_t original_size)
{
    size_t min_alignment =
        physical_device_properties.limits.minUniformBufferOffsetAlignment;
    size_t aligned_size = original_size;
    if (min_alignment > 0) {
        aligned_size =
            (aligned_size + min_alignment - 1) & ~(min_alignment - 1);
    }
    return aligned_size;
}

Result<AllocatedBuffer, VkResult> Engine::create_buffer(
    size_t alloc_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage)
{
    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size  = alloc_size,
        .usage = usage,
    };

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage                   = memory_usage;

    AllocatedBuffer result;
    // allocate the buffer
    VK_RETURN_IF_ERR(vmaCreateBuffer(
        vmalloc,
        &buffer_info,
        &alloc_info,
        &result.buffer,
        &result.allocation,
        nullptr));
    return Ok(result);
}

void Engine::immediate_submit(ImmediateSubmitDelegate&& submit_delegate)
{
    VkCommandBuffer          cmd        = upload.buffer;
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));
    submit_delegate.call(cmd);
    VK_CHECK(vkEndCommandBuffer(cmd));

    VkSubmitInfo submit_info = {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount   = 0,
        .pWaitSemaphores      = 0,
        .pWaitDstStageMask    = 0,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &cmd,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores    = 0,
    };
    VK_CHECK(vkQueueSubmit(graphics.queue, 1, &submit_info, upload.fnc_upload));

    vkWaitForFences(device, 1, &upload.fnc_upload, VK_TRUE, 9999999999);
    vkResetFences(device, 1, &upload.fnc_upload);

    vkResetCommandPool(device, upload.pool, 0);
}

// Debug Camera
void Engine::on_debug_camera_forward()
{
    if (!debug_camera.has_focus) return;
    glm::vec3 fwd = glm::normalize(debug_camera.rotation * glm::vec3(0, 0, -1));
    debug_camera.position += 0.1f * fwd;
}
void Engine::on_debug_camera_back()
{
    if (!debug_camera.has_focus) return;
    glm::vec3 fwd = glm::normalize(debug_camera.rotation * glm::vec3(0, 0, -1));
    debug_camera.position -= 0.1f * fwd;
}

void Engine::on_debug_camera_right()
{
    if (!debug_camera.has_focus) return;
    glm::vec3 fwd = glm::normalize(debug_camera.rotation * glm::vec3(0, 0, -1));
    glm::vec3 right = glm::normalize(glm::cross(fwd, glm::vec3(0, 1, 0)));

    debug_camera.position += 0.1f * right;
}
void Engine::on_debug_camera_left()
{
    if (!debug_camera.has_focus) return;
    glm::vec3 fwd = glm::normalize(debug_camera.rotation * glm::vec3(0, 0, -1));
    glm::vec3 right = glm::normalize(glm::cross(fwd, glm::vec3(0, 1, 0)));

    debug_camera.position -= 0.1f * right;
}

void Engine::on_debug_camera_mousex(float value)
{
    if (!debug_camera.has_focus) return;
    debug_camera.yaw += value * 0.1f;
    while (debug_camera.yaw > 360.f) {
        debug_camera.yaw -= 360.f;
    }

    while (debug_camera.yaw < 0.f) {
        debug_camera.yaw += 360.f;
    }

    debug_camera_update_rotation();
}
void Engine::on_debug_camera_mousey(float value)
{
    if (!debug_camera.has_focus) return;
    debug_camera.pitch += value * 0.1f;
    debug_camera.pitch = glm::clamp(debug_camera.pitch, -89.f, 89.f);
    debug_camera_update_rotation();
}

void Engine::debug_camera_update_rotation()
{
    debug_camera.rotation = glm::normalize(
        glm::angleAxis(glm::radians(debug_camera.yaw), glm::vec3(0, 1, 0)) *
        glm::angleAxis(glm::radians(debug_camera.pitch), glm::vec3(1, 0, 0)));

    // debug_camera.rotation =
    //     glm::angleAxis(glm::radians(debug_camera.yaw), glm::vec3(0, 1, 0)) *
    //     glm::angleAxis(glm::radians(debug_camera.pitch), glm::vec3(1, 0, 0));
}

void Engine::on_debug_camera_toggle_control()
{
    print(LIT("YEET\n"));
    debug_camera.has_focus = !debug_camera.has_focus;
    window->set_lock_cursor(debug_camera.has_focus);
}

void Engine::on_resize_presentation()
{
    VK_CHECK(vkDeviceWaitIdle(device));
    recreate_swapchain();
    init_framebuffers();
}

VkShaderModule Engine::load_shader(Str path)
{
    CREATE_SCOPED_ARENA(&allocator, temp_alloc, KILOBYTES(1));
    auto load_result = load_shader_binary(temp_alloc, device, path);

    if (!load_result.ok()) {
        print(LIT("Failed to load & create shader module {}\n"), path);
        return VK_NULL_HANDLE;
    }

    print(LIT("Loaded shader module binary {}\n"), path);
    return load_result.value();
}

void Engine::upload_mesh(Mesh& mesh)
{
    const size_t buffer_size = mesh.vertices.size();

    VkBufferCreateInfo staging_buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size  = buffer_size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    };

    VmaAllocationCreateInfo staging_alloc_info = {
        .usage = VMA_MEMORY_USAGE_CPU_ONLY,
    };

    AllocatedBuffer staging_buffer;

    VK_CHECK(vmaCreateBuffer(
        vmalloc,
        &staging_buffer_info,
        &staging_alloc_info,
        &staging_buffer.buffer,
        &staging_buffer.allocation,
        0));
    void* data;
    vmaMapMemory(vmalloc, staging_buffer.allocation, &data);
    memcpy(data, mesh.vertices.ptr, mesh.vertices.size());
    vmaUnmapMemory(vmalloc, staging_buffer.allocation);

    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size  = buffer_size,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    };

    VmaAllocationCreateInfo alloc_info = {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
    };

    VK_CHECK(vmaCreateBuffer(
        vmalloc,
        &buffer_info,
        &alloc_info,
        &mesh.gpu_buffer.buffer,
        &mesh.gpu_buffer.allocation,
        0));

    immediate_submit_lambda([=](VkCommandBuffer cmd) {
        VkBufferCopy copy = {
            .srcOffset = 0,
            .dstOffset = 0,
            .size      = buffer_size,
        };
        vkCmdCopyBuffer(
            cmd,
            staging_buffer.buffer,
            mesh.gpu_buffer.buffer,
            1,
            &copy);
    });

    main_deletion_queue.add(
        DeletionQueue::DeletionDelegate::create_lambda([=]() {
            vmaDestroyBuffer(
                vmalloc,
                mesh.gpu_buffer.buffer,
                mesh.gpu_buffer.allocation);
        }));

    vmaDestroyBuffer(vmalloc, staging_buffer.buffer, staging_buffer.allocation);
}

void Engine::draw()
{
    FrameData& frame = get_current_frame();

    // Wait for previous frame to finish
    VK_CHECK(
        vkWaitForFences(device, 1, &frame.fnc_render, VK_TRUE, (u64)1.6e+7));

    // Get next image
    u32      next_image_index;
    VkResult next_image_result = vkAcquireNextImageKHR(
        device,
        swap_chain,
        1000000000,
        frame.sem_present,
        0,
        &next_image_index);

    // VK_SUBOPTIMAL_KHR considered non-error
    if (!(next_image_result == VK_SUCCESS ||
          next_image_result == VK_SUBOPTIMAL_KHR))
    {
        if (next_image_result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreate_swapchain();
            init_framebuffers();
            return;
        }
    }

    VK_CHECK(vkResetFences(device, 1, &frame.fnc_render));

    VkCommandBuffer cmd = frame.main_cmd_buffer;

    // Reset primary cmd buffer
    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    // Begin commands
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));

    // Flash clear color
    float        flash       = abs(sinf(float(frame_num) / 120.0f));
    VkClearValue clear_value = {
        .color = {0, 0, flash, 1.0f},
    };

    VkClearValue depth_clear_value = {
        .depthStencil =
            {
                .depth = 1.f,
            },
    };

    VkClearValue clear_values[2] = {
        clear_value,
        depth_clear_value,
    };

    VkRenderPassBeginInfo rp_begin_info = {
        .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass  = render_pass,
        .framebuffer = framebuffers[next_image_index],
        .renderArea =
            {
                .offset = {0, 0},
                .extent = extent,
            },
        .clearValueCount = ARRAY_COUNT(clear_values),
        .pClearValues    = clear_values,
    };
    vkCmdBeginRenderPass(cmd, &rp_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {
        .x        = 0,
        .y        = 0,
        .width    = (float)extent.width,
        .height   = (float)extent.height,
        .minDepth = 0.f,
        .maxDepth = 1.f,
    };

    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {.offset = {0, 0}, .extent = extent};
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    glm::quat q              = debug_camera.rotation;
    glm::vec3 camera_forward = glm::normalize(q * glm::vec3(0, 0, -1));
    glm::vec3 camera_right =
        glm::normalize(glm::cross(glm::vec3(0, 1, 0), camera_forward));
    glm::vec3 camera_up =
        glm::normalize(glm::cross(camera_forward, camera_right));
    glm::vec3 camera_target = debug_camera.position + camera_forward;

    glm::mat4 view =
        glm::lookAt(debug_camera.position, camera_target, camera_up);
    glm::mat4 proj = glm::perspective(
        glm::radians(70.f),
        (float(extent.width) / float(extent.height)),
        0.1f,
        200.0f);
    proj[1][1] *= -1;

    int frame_idx = frame_num % num_overlap_frames;

    // Write global data
    {
        GPUGlobalInstanceData global_instance_data;
        global_instance_data.camera = {
            .view     = view,
            .proj     = proj,
            .viewproj = proj * view,
        };

        float framed               = (frame_num / 120.f);
        global_instance_data.scene = {
            .ambient_color = {glm::sin(framed), 0, cos(framed), 1},
        };

        u8* global_instance_data_ptr;
        VK_CHECK(vmaMapMemory(
            vmalloc,
            global.buffer.allocation,
            (void**)&global_instance_data_ptr));

        global_instance_data_ptr +=
            pad_uniform_buffer_size(sizeof(GPUGlobalInstanceData)) * frame_idx;
        memcpy(
            global_instance_data_ptr,
            &global_instance_data,
            sizeof(GPUGlobalInstanceData));

        vmaUnmapMemory(vmalloc, global.buffer.allocation);
    }

    // Write object data
    {
        void* object_data;
        VK_CHECK(vmaMapMemory(
            vmalloc,
            frame.object_buffer.allocation,
            &object_data));
        GPUObjectData* object_ssbo = (GPUObjectData*)object_data;
        for (u64 i = 0; i < render_objects.size; ++i) {
            RenderObject& ro     = render_objects[i];
            object_ssbo[i].model = ro.transform;
        }
        vmaUnmapMemory(vmalloc, frame.object_buffer.allocation);
    }

    Material* last_material = 0;

    for (u64 i = 0; i < render_objects.size; ++i) {
        RenderObject& ro = render_objects[i];
        if ((last_material == 0) || (*last_material != *ro.material)) {
            vkCmdBindPipeline(
                cmd,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                ro.material->pipeline);

            u32 uniform_offsets[] = {
                // Global
                u32(pad_uniform_buffer_size(sizeof(GPUGlobalInstanceData))) *
                    frame_idx,
            };

            vkCmdBindDescriptorSets(
                cmd,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                ro.material->pipeline_layout,
                0,
                1,
                &frame.global_descriptor,
                ARRAY_COUNT(uniform_offsets),
                uniform_offsets);

            vkCmdBindDescriptorSets(
                cmd,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                ro.material->pipeline_layout,
                1,
                1,
                &frame.object_descriptor,
                0,
                nullptr);
        }

        last_material = ro.material;

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &ro.mesh->gpu_buffer.buffer, &offset);
        vkCmdDraw(cmd, (u32)ro.mesh->vertices.count, 1, 0, u32(i));
    }

    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);

    // Submit
    VkPipelineStageFlags wait_stage =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info = {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &frame.sem_present,
        .pWaitDstStageMask    = &wait_stage,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &cmd,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &frame.sem_render,
    };
    VK_CHECK(vkQueueSubmit(graphics.queue, 1, &submit_info, frame.fnc_render));

    // Display image
    VkPresentInfoKHR present_info = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &frame.sem_render,
        .swapchainCount     = 1,
        .pSwapchains        = &swap_chain,
        .pImageIndices      = &next_image_index,
    };

    VK_CHECK(vkQueuePresentKHR(presentation.queue, &present_info));

    frame_num += 1;
}

void Engine::recreate_swapchain()
{
    vkDeviceWaitIdle(device);

    swap_chain_deletion_queue.flush();
    Vec2i new_size = window->get_extents();
    extent         = {.width = (u32)new_size.x, .height = (u32)new_size.y};
    CREATE_SCOPED_ARENA(&allocator, temp_alloc, KILOBYTES(1));

    // Create swap chain & views
    {
        VkSurfaceFormatKHR  surface_format;
        VkPresentModeKHR    present_mode;
        u32                 num_images;
        CreateSwapchainInfo info = {
            .physical_device = physical_device,
            .format          = VK_FORMAT_B8G8R8A8_SRGB,
            .color_space     = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
            .surface         = surface,
            .graphics_family = graphics.family,
            .present_family  = presentation.family,
            .extent          = extent,
            .surface_format  = surface_format,
            .present_mode    = present_mode,
            .num_images      = num_images,
        };
        swap_chain = create_swapchain(temp_alloc, device, info).unwrap();
        swap_chain_image_format = surface_format.format;
        swap_chain_image_views.init_range(num_images);

        vkGetSwapchainImagesKHR(device, swap_chain, &num_images, 0);
        swap_chain_images.init_range(num_images);
        vkGetSwapchainImagesKHR(
            device,
            swap_chain,
            &num_images,
            swap_chain_images.data);

        for (u32 i = 0; i < num_images; ++i) {
            VkImageViewCreateInfo create_info = {
                .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image    = swap_chain_images[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format   = surface_format.format,
                .components =
                    {
                        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                    },
                .subresourceRange =
                    {
                        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel   = 0,
                        .levelCount     = 1,
                        .baseArrayLayer = 0,
                        .layerCount     = 1,
                    },
            };

            VK_CHECK(vkCreateImageView(
                device,
                &create_info,
                0,
                &swap_chain_image_views[i]));
        }

        swap_chain_deletion_queue.add(
            DeletionQueue::DeletionDelegate::create_lambda([this]() {
                vkDestroySwapchainKHR(device, swap_chain, 0);
            }));
    }

    // Create depth buffer
    {
        VkImageCreateInfo image_create_info = {
            .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format    = depth_image_format,
            .extent =
                {
                    .width  = extent.width,
                    .height = extent.height,
                    .depth  = 1,
                },
            .mipLevels   = 1,
            .arrayLayers = 1,
            .samples     = VK_SAMPLE_COUNT_1_BIT,
            .tiling      = VK_IMAGE_TILING_OPTIMAL,
            .usage       = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        };

        VmaAllocationCreateInfo image_allocation_info = {
            .usage = VMA_MEMORY_USAGE_GPU_ONLY,
            .requiredFlags =
                VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
        };

        VK_CHECK(vmaCreateImage(
            vmalloc,
            &image_create_info,
            &image_allocation_info,
            &depth_image.image,
            &depth_image.allocation,
            0));

        VkImageViewCreateInfo view_create_info = {
            .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image    = depth_image.image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format   = depth_image_format,
            .subresourceRange =
                {
                    .aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT,
                    .baseMipLevel   = 0,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1,
                },
        };

        VK_CHECK(
            vkCreateImageView(device, &view_create_info, 0, &depth_image_view));

        swap_chain_deletion_queue.add(
            DeletionQueue::DeletionDelegate::create_lambda([this]() {
                vkDestroyImageView(device, depth_image_view, 0);
                vmaDestroyImage(
                    vmalloc,
                    depth_image.image,
                    depth_image.allocation);
            }));
    }
}

void Engine::deinit()
{
    if (!is_initialized) return;
    for (int i = 0; i < num_overlap_frames; ++i) {
        VK_CHECK(vkWaitForFences(
            device,
            1,
            &frames[i].fnc_render,
            VK_TRUE,
            1000000));
    }

    swap_chain_deletion_queue.flush();
    main_deletion_queue.flush();
    vmaDestroyAllocator(vmalloc);
    vkDestroyDevice(device, 0);
    vkDestroySurfaceKHR(instance, surface, 0);
    vkDestroyInstance(instance, 0);
}

Material* Engine::create_material(
    VkPipeline pipeline, VkPipelineLayout layout, Str id)
{
    Material mt = {
        .pipeline        = pipeline,
        .pipeline_layout = layout,
    };
    materials.add(id, mt);

    return &materials[id];
}

Mesh* Engine::get_mesh(Str id)
{
    if (meshes.contains(id)) {
        return &meshes[id];
    }
    return 0;
}

Material* Engine::get_material(Str id)
{
    if (materials.contains(id)) {
        return &materials[id];
    }
    return 0;
}

void Engine::init_default_meshes()
{
    Vertex* vertices = alloc_array<Vertex, 3>(allocator);

    vertices[0] = {
        .position = {1, 1, 0},
        .color    = {0, 1, 0},
    };

    vertices[1] = {
        .position = {-1, 1, 0},
        .color    = {0, 1, 0},
    };

    vertices[2] = {
        .position = {0, -1, 0},
        .color    = {0, 1, 0},
    };

    triangle_mesh.vertices = slice(vertices, 3);
    upload_mesh(triangle_mesh);

    monke_mesh =
        Mesh::load_from_file(allocator, "Assets/monkey_smooth.obj").unwrap();
    upload_mesh(monke_mesh);

    meshes.add(LIT("triangle"), triangle_mesh);
    meshes.add(LIT("monke"), monke_mesh);
}

void PipelineBuilder::init_defaults()
{
    vertex_input_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = 0,
        .vertexBindingDescriptionCount   = 0,
        .vertexAttributeDescriptionCount = 0,
    };

    asm_create_info = {
        .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext    = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    rasterizer_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .depthClampEnable        = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode             = VK_POLYGON_MODE_FILL,
        .cullMode                = VK_CULL_MODE_NONE,
        .frontFace               = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable         = VK_FALSE,
        .depthBiasConstantFactor = 0.f,
        .depthBiasClamp          = 0.f,
        .depthBiasSlopeFactor    = 0.f,
        .lineWidth               = 1.0f,
    };

    depth_stencil_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .depthTestEnable       = VK_FALSE,
        .depthWriteEnable      = VK_FALSE,
        .depthCompareOp        = VK_COMPARE_OP_ALWAYS,
        .depthBoundsTestEnable = VK_FALSE,
        .minDepthBounds        = 0.f,
        .maxDepthBounds        = 1.f,
    };

    multisample_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable   = VK_FALSE,
        .minSampleShading      = 1.0f,
        .pSampleMask           = 0,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable      = VK_FALSE,
    };

    color_blend_attachment_state = {
        .blendEnable         = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp        = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp        = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    dynamic_state_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .dynamicStateCount = 0,
        .pDynamicStates    = 0,
    };
}

PipelineBuilder& PipelineBuilder::add_shader_stage(
    VkShaderStageFlagBits stage, VkShaderModule module)
{
    VkPipelineShaderStageCreateInfo create_info = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = stage,
        .module = module,
        .pName  = "main",
    };

    shader_stages.add(create_info);
    return *this;
}

PipelineBuilder& PipelineBuilder::set_primitive_topology(
    VkPrimitiveTopology topology)
{
    asm_create_info.topology = topology;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_polygon_mode(VkPolygonMode mode)
{
    rasterizer_state.polygonMode = mode;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_viewport(
    float x, float y, float w, float h, float min_d, float max_d)
{
    viewport = {
        .x        = x,
        .y        = y,
        .width    = w,
        .height   = h,
        .minDepth = min_d,
        .maxDepth = max_d,
    };

    return *this;
}

PipelineBuilder& PipelineBuilder::set_scissor(i32 x, i32 y, u32 w, u32 h)
{
    scissor = {
        .offset = {x, y},
        .extent = {w, h},
    };
    return *this;
}

PipelineBuilder& PipelineBuilder::set_render_pass(VkRenderPass render_pass)
{
    this->render_pass = render_pass;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_layout(VkPipelineLayout layout)
{
    this->layout = layout;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_vertex_input_bindings(
    Slice<VkVertexInputBindingDescription> bindings)
{
    vertex_input_info.vertexBindingDescriptionCount = (u32)bindings.count;
    vertex_input_info.pVertexBindingDescriptions    = bindings.ptr;
    return *this;
}
PipelineBuilder& PipelineBuilder::set_vertex_input_attributes(
    Slice<VkVertexInputAttributeDescription> attributes)
{
    vertex_input_info.vertexAttributeDescriptionCount = (u32)attributes.count;
    vertex_input_info.pVertexAttributeDescriptions    = attributes.ptr;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_vertex_input_info(VertexInputInfo& info)
{
    set_vertex_input_bindings(info.bindings);
    set_vertex_input_attributes(info.attributes);
    return *this;
}

PipelineBuilder& PipelineBuilder::set_depth_test(
    bool enabled, bool write, VkCompareOp compare_op)
{
    depth_stencil_state.depthTestEnable  = enabled ? VK_TRUE : VK_FALSE;
    depth_stencil_state.depthWriteEnable = write ? VK_TRUE : VK_FALSE;
    depth_stencil_state.depthCompareOp   = compare_op;
    return *this;
}

PipelineBuilder& PipelineBuilder::add_dynamic_state(
    VkDynamicState dynamic_state)
{
    dynamic_states.add(dynamic_state);
    return *this;
}

Result<VkPipeline, VkResult> PipelineBuilder::build(VkDevice device)
{
    VkPipelineViewportStateCreateInfo viewport_info = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports    = &viewport,
        .scissorCount  = 1,
        .pScissors     = &scissor,
    };

    VkPipelineColorBlendStateCreateInfo color_blend_state_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable   = VK_FALSE,
        .logicOp         = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments    = &color_blend_attachment_state,
    };

    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = 0,
        .flags               = 0,
        .stageCount          = (u32)shader_stages.size,
        .pStages             = shader_stages.data,
        .pVertexInputState   = &vertex_input_info,
        .pInputAssemblyState = &asm_create_info,
        .pTessellationState  = 0,
        .pViewportState      = &viewport_info,
        .pRasterizationState = &rasterizer_state,
        .pMultisampleState   = &multisample_state,
        .pDepthStencilState  = &depth_stencil_state,
        .pColorBlendState    = &color_blend_state_info,
        .pDynamicState       = &dynamic_state_info,
        .layout              = layout,
        .renderPass          = render_pass,
        .subpass             = 0,
        .basePipelineHandle  = VK_NULL_HANDLE,
    };

    if (dynamic_states.size != 0) {
        dynamic_state_info.dynamicStateCount = (u32)dynamic_states.size;
        dynamic_state_info.pDynamicStates    = dynamic_states.data;
    }

    VkPipeline result;
    VK_RETURN_IF_ERR(vkCreateGraphicsPipelines(
        device,
        VK_NULL_HANDLE,
        1,
        &pipeline_info,
        0,
        &result));

    return Ok(result);
}
