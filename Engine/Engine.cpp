#include "Engine.h"

#include "Memory/Extras.h"

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

void Engine::init()
{
    swap_chain_images.alloc      = &allocator;
    swap_chain_image_views.alloc = &allocator;
    framebuffers.alloc           = &allocator;
    main_deletion_queue          = DeletionQueue(allocator);
    CREATE_SCOPED_ARENA(&allocator, temp_alloc, KILOBYTES(4));

    Slice<const char*> required_window_ext = win32_get_required_extensions();

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
            .apiVersion         = VK_API_VERSION_1_0,
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

        CreateDeviceWithQueuesInfo info = {
            .surface             = surface,
            .extensions          = slice(Device_Extensions),
            .family_requirements = slice(requirements),
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

    // Create swap chain & views
    {
        SAVE_ARENA(temp_alloc);
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

        main_deletion_queue.add(
            DeletionQueue::DeletionDelegate::create_lambda([this]() {
                vkDestroySwapchainKHR(this->device, this->swap_chain, 0);
            }));
    }

    // Create graphics command pool
    {
        VkCommandPoolCreateInfo create_info = {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = graphics.family,
        };

        VK_CHECK(vkCreateCommandPool(device, &create_info, 0, &cmd_pool));

        main_deletion_queue.add(
            DeletionQueue::DeletionDelegate::create_lambda([this]() {
                vkDestroyCommandPool(this->device, this->cmd_pool, 0);
            }));
    }

    // Create main cmd buffer
    {
        VkCommandBufferAllocateInfo create_info = {
            .sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = cmd_pool,
            .level       = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        VK_CHECK(
            vkAllocateCommandBuffers(device, &create_info, &main_cmd_buffer));
    }

    init_default_renderpass();
    init_pipelines();
    init_framebuffers();
    init_sync_objects();
    init_default_meshes();

    is_initialized = true;
}

void Engine::init_pipelines()
{
    // Load shaders
    VkShaderModule basic_shader_vert, basic_shader_frag;

    basic_shader_vert = load_shader(LIT("Shaders/Basic.vert.spv"));
    basic_shader_frag = load_shader(LIT("Shaders/Basic.frag.spv"));

    // Create layout
    {
        VkPushConstantRange push_constant = {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset     = 0,
            .size       = sizeof(MeshPushConstants),
        };

        VkPipelineLayoutCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges    = &push_constant,
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
            .set_primitive_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .set_polygon_mode(VK_POLYGON_MODE_FILL)
            .set_viewport(0, 0, extent.width, extent.height, 0.0f, 1.0f)
            .set_scissor(0, 0, extent.width, extent.height)
            .set_render_pass(render_pass)
            .set_layout(triangle_layout)
            .set_vertex_input_info(vertex_input_info)
            .build(device)
            .unwrap();

    main_deletion_queue.add(
        DeletionQueue::DeletionDelegate::create_lambda([this]() {
            vkDestroyPipeline(device, pipeline, 0);
            vkDestroyPipelineLayout(device, triangle_layout, 0);
        }));

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

    VkSubpassDescription subpass = {
        .pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &color_attachment_ref,
    };

    VkRenderPassCreateInfo create_info = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments    = &color_attachment,
        .subpassCount    = 1,
        .pSubpasses      = &subpass,
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
        .attachmentCount = 1,
        .width           = extent.width,
        .height          = extent.height,
        .layers          = 1,
    };

    const u32 swapchain_image_count = (u32)swap_chain_images.size;
    framebuffers.init_range(swapchain_image_count);

    for (u32 i = 0; i < swapchain_image_count; ++i) {
        create_info.pAttachments = &swap_chain_image_views[i];
        VK_CHECK(
            vkCreateFramebuffer(device, &create_info, 0, &framebuffers[i]));

        main_deletion_queue.add(
            DeletionQueue::DeletionDelegate::create_lambda([this, i]() {
                vkDestroyFramebuffer(this->device, framebuffers[i], 0);
                vkDestroyImageView(this->device, swap_chain_image_views[i], 0);
            }));
    }
}

void Engine::init_sync_objects()
{
    VkFenceCreateInfo fnc_create_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    VK_CHECK(vkCreateFence(device, &fnc_create_info, 0, &fnc_render));

    main_deletion_queue.add(DeletionQueue::DeletionDelegate::create_lambda(
        [this]() { vkDestroyFence(this->device, this->fnc_render, 0); }));

    VkSemaphoreCreateInfo sem_create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VK_CHECK(vkCreateSemaphore(device, &sem_create_info, 0, &sem_present));
    VK_CHECK(vkCreateSemaphore(device, &sem_create_info, 0, &sem_render));

    main_deletion_queue.add(
        DeletionQueue::DeletionDelegate::create_lambda([this]() {
            vkDestroySemaphore(this->device, this->sem_render, 0);
            vkDestroySemaphore(this->device, this->sem_present, 0);
        }));
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
    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size  = sizeof(Vertex) * mesh.vertices.count,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    };

    VmaAllocationCreateInfo alloc_info = {
        .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
    };

    VK_CHECK(vmaCreateBuffer(
        vmalloc,
        &buffer_info,
        &alloc_info,
        &mesh.gpu_buffer.buffer,
        &mesh.gpu_buffer.allocation,
        0));

    void* data;
    vmaMapMemory(vmalloc, mesh.gpu_buffer.allocation, &data);
    memcpy(data, mesh.vertices.ptr, mesh.vertices.size());
    vmaUnmapMemory(vmalloc, mesh.gpu_buffer.allocation);
}

void Engine::draw()
{
    // Wait for previous frame to finish
    VK_CHECK(vkWaitForFences(device, 1, &fnc_render, VK_TRUE, 1000000));
    VK_CHECK(vkResetFences(device, 1, &fnc_render));

    // Get next image
    u32 next_image_index;
    VK_CHECK(vkAcquireNextImageKHR(
        device,
        swap_chain,
        1000000000,
        sem_present,
        0,
        &next_image_index));

    // Reset primary cmd buffer
    VK_CHECK(vkResetCommandBuffer(main_cmd_buffer, 0));

    VkCommandBuffer cmd = main_cmd_buffer;

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

    VkRenderPassBeginInfo rp_begin_info = {
        .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass  = render_pass,
        .framebuffer = framebuffers[next_image_index],
        .renderArea =
            {
                .offset = {0, 0},
                .extent = extent,
            },
        .clearValueCount = 1,
        .pClearValues    = &clear_value,
    };
    vkCmdBeginRenderPass(cmd, &rp_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    Vec3 camera_pos = {0, 0, 2.f};

    Mat4 view       = translation(camera_pos * -1.0f);
    Mat4 projection = perspective(
        70.f * To_Radians,
        (float(extent.width) / float(extent.height)),
        0.1f,
        200.0f);

    Mat4 model = rotation_yaw((0.4f * frame_num) * To_Radians) *
                 translation(Vec3{0, 0.f, -2.0f});
    Mat4 transform = model * projection;

    MeshPushConstants constants;
    constants.transform = transform;

    vkCmdPushConstants(
        cmd,
        triangle_layout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(MeshPushConstants),
        &constants);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &monke_mesh.gpu_buffer.buffer, &offset);

    vkCmdDraw(cmd, (u32)monke_mesh.vertices.count, 1, 0, 0);

    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);

    // Submit
    VkPipelineStageFlags wait_stage =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info = {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &sem_present,
        .pWaitDstStageMask    = &wait_stage,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &cmd,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &sem_render,
    };
    VK_CHECK(vkQueueSubmit(graphics.queue, 1, &submit_info, fnc_render));

    // Display image
    VkPresentInfoKHR present_info = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &sem_render,
        .swapchainCount     = 1,
        .pSwapchains        = &swap_chain,
        .pImageIndices      = &next_image_index,
    };

    VK_CHECK(vkQueuePresentKHR(presentation.queue, &present_info));

    frame_num += 1;
}

void Engine::deinit()
{
    if (!is_initialized) return;
    VK_CHECK(vkWaitForFences(device, 1, &fnc_render, VK_TRUE, 1000000));

    main_deletion_queue.flush();

    vkDestroyDevice(device, 0);
    vkDestroySurfaceKHR(instance, surface, 0);
    vkDestroyInstance(instance, 0);
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
    vertex_input_info.vertexBindingDescriptionCount = bindings.count;
    vertex_input_info.pVertexBindingDescriptions    = bindings.ptr;
    return *this;
}
PipelineBuilder& PipelineBuilder::set_vertex_input_attributes(
    Slice<VkVertexInputAttributeDescription> attributes)
{
    vertex_input_info.vertexAttributeDescriptionCount = attributes.count;
    vertex_input_info.pVertexAttributeDescriptions    = attributes.ptr;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_vertex_input_info(VertexInputInfo& info)
{
    set_vertex_input_bindings(info.bindings);
    set_vertex_input_attributes(info.attributes);
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
        .pDepthStencilState  = 0,
        .pColorBlendState    = &color_blend_state_info,
        .layout              = layout,
        .renderPass          = render_pass,
        .subpass             = 0,
        .basePipelineHandle  = VK_NULL_HANDLE,
    };

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
