#include "App.h"

#include <set>

#include "VulkanCommon.h"

auto Validation_Layers = arr<const char*>("VK_LAYER_KHRONOS_validation");
auto Device_Extensions = arr<const char*>(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

static VK_PROC_DEBUG_CALLBACK(debug_callback)
{
    if (severity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        print(LIT("Vulkan: {}\n"), Str(callback_data->pMessage));
    }
    return VK_FALSE;
}

Result<void, VkResult> App::init()
{
    arena = Arena(&allocator, KILOBYTES(4));

    swap_chain_images.alloc    = &arena;
    swap_chain_views.alloc     = &arena;
    framebuffers.alloc         = &arena;
    cmd_buffers.alloc          = &arena;
    sems_image_available.alloc = &arena;
    sems_render_finished.alloc = &arena;
    fences_in_flight.alloc     = &arena;

    CREATE_SCOPED_ARENA(&allocator, temp_alloc, KILOBYTES(4));

    // Get available extensions
    {
        u32 count = 0;
        VK_RETURN_IF_ERR(vkEnumerateInstanceExtensionProperties(0, &count, 0));

        TArray<VkExtensionProperties> properties(&System_Allocator);
        properties.init_range(count);

        VK_RETURN_IF_ERR(
            vkEnumerateInstanceExtensionProperties(0, &count, properties.data));

        print(LIT("Extensions:\n"));
        for (auto& property : properties) {
            print(
                LIT("\t {}: {}\n"),
                property.specVersion,
                Str(property.extensionName));
        }
    }

    // Create instance
    {
        Slice<const char*> extensions = win32_get_required_extensions();

        if (config.validation_layers) {
            if (!check_validation_layers(temp_alloc)) {
                return Err(VK_ERROR_UNKNOWN);
            }
        }

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
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = config.validation_layers ? &debug_create_info : 0,
            .pApplicationInfo = &app_info,
            .enabledLayerCount =
                (config.validation_layers) ? Validation_Layers.count() : 0,
            .ppEnabledLayerNames     = Validation_Layers.elements,
            .enabledExtensionCount   = (u32)extensions.count,
            .ppEnabledExtensionNames = extensions.ptr,
        };

        VK_RETURN_IF_ERR(vkCreateInstance(&create_info, 0, &instance));
    }

    // Create surface
    {
        auto create_surface_result = window.create_surface(instance);
        if (!create_surface_result.ok()) {
            return Err(create_surface_result.err());
        }

        surface = create_surface_result.value();
    }

    // Pick physical device
    {
        u32 device_count;
        VK_RETURN_IF_ERR(
            vkEnumeratePhysicalDevices(instance, &device_count, 0));

        TArray<VkPhysicalDevice> physical_devices(&temp_alloc);
        physical_devices.init_range(device_count);
        VK_RETURN_IF_ERR(vkEnumeratePhysicalDevices(
            instance,
            &device_count,
            physical_devices.data));

        for (VkPhysicalDevice& device : physical_devices) {
            if (is_physical_device_suitable(device, temp_alloc)) {
                physical_device = device;
                break;
            }
        }

        if (physical_device == VK_NULL_HANDLE) {
            return Err(VK_ERROR_UNKNOWN);
        }
    }

    auto family_indices_result =
        find_queue_family_indices(temp_alloc, physical_device);
    if (!family_indices_result.ok()) {
        return Err(family_indices_result.err());
    }

    QueueFamilyIndices family_indices = family_indices_result.value();

    // Create logical device & queues
    {
        std::set<int> unique_indices = {
            family_indices.graphics,
            family_indices.present,
        };

        float                           queue_priority = 1.0f;
        TArray<VkDeviceQueueCreateInfo> queue_create_infos(&temp_alloc);

        for (int index : unique_indices) {
            VkDeviceQueueCreateInfo queue_create_info = {
                .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = (u32)index,
                .queueCount       = 1,
                .pQueuePriorities = &queue_priority};
            queue_create_infos.add(queue_create_info);
        }

        VkPhysicalDeviceFeatures features    = {0};
        VkDeviceCreateInfo       create_info = {
                  .sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                  .queueCreateInfoCount = (u32)queue_create_infos.size,
                  .pQueueCreateInfos    = queue_create_infos.data,
                  .enabledLayerCount =
                config.validation_layers ? Validation_Layers.count() : 0,
                  .ppEnabledLayerNames     = Validation_Layers.elements,
                  .enabledExtensionCount   = Device_Extensions.count(),
                  .ppEnabledExtensionNames = Device_Extensions.elements,
                  .pEnabledFeatures        = &features,
        };

        VK_RETURN_IF_ERR(
            vkCreateDevice(physical_device, &create_info, 0, &device));

        vkGetDeviceQueue(device, family_indices.graphics, 0, &graphics_queue);
        vkGetDeviceQueue(device, family_indices.present, 0, &present_queue);
    }

    // Create Swap Chain
    {
        u32 num_images;

        SwapChainSupportDetails details =
            query_swap_chain_support(temp_alloc, physical_device);
        DEFER(details.release());

        // Choose surface format
        surface_format = details.formats[0];
        for (const VkSurfaceFormatKHR& format : details.formats) {
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
                format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                surface_format = format;
                break;
            }
        }

        // Choose present mode
        present_mode = VK_PRESENT_MODE_FIFO_KHR;
        for (const VkPresentModeKHR& mode : details.present_modes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                present_mode = mode;
                break;
            }
        }

        // Choose extents
        if (details.capabilities.currentExtent.width != NumProps<u32>::max) {
            extent = details.capabilities.currentExtent;
        } else {
            Vec2i      d             = window.get_extents();
            VkExtent2D actual_extent = {
                .width  = (u32)d.x,
                .height = (u32)d.y,
            };

            actual_extent.width = clamp(
                actual_extent.width,
                details.capabilities.minImageExtent.width,
                details.capabilities.maxImageExtent.width);

            actual_extent.height = clamp(
                actual_extent.height,
                details.capabilities.minImageExtent.height,
                details.capabilities.maxImageExtent.height);

            extent = actual_extent;
        }

        num_images = details.capabilities.minImageCount + 1;
        if ((details.capabilities.maxImageCount > 0) &&
            (num_images > details.capabilities.maxImageCount))
        {
            num_images = details.capabilities.maxImageCount;
        }

        // Create swap chain
        VkSwapchainCreateInfoKHR create_info = {
            .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface          = surface,
            .minImageCount    = num_images,
            .imageFormat      = surface_format.format,
            .imageColorSpace  = surface_format.colorSpace,
            .imageExtent      = extent,
            .imageArrayLayers = 1,
            .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .preTransform     = details.capabilities.currentTransform,
            .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode      = present_mode,
            .clipped          = VK_TRUE,
            .oldSwapchain     = VK_NULL_HANDLE,
        };

        u32 queue_family_indices[] = {
            (u32)family_indices.graphics,
            (u32)family_indices.present,
        };

        if (family_indices.graphics != family_indices.present) {
            create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = 2;
            create_info.pQueueFamilyIndices   = queue_family_indices;
        } else {
            create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
            create_info.queueFamilyIndexCount = 0;
            create_info.pQueueFamilyIndices   = 0;
        }

        VK_RETURN_IF_ERR(
            vkCreateSwapchainKHR(device, &create_info, 0, &swap_chain));

        vkGetSwapchainImagesKHR(device, swap_chain, &num_images, 0);
        swap_chain_images.init_range(num_images);
        vkGetSwapchainImagesKHR(
            device,
            swap_chain,
            &num_images,
            swap_chain_images.data);
    }

    print(LIT("{} swap chain images\n"), swap_chain_images.size);

    // Create image views
    {
        swap_chain_views.init_range(swap_chain_images.size);
        for (u64 i = 0; i < swap_chain_images.size; ++i) {
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

            VK_RETURN_IF_ERR(vkCreateImageView(
                device,
                &create_info,
                0,
                &swap_chain_views[i]));
        }
    }

    return Ok<void>();
}

void App::recompile_shaders()
{
    sh_vertex =
        load_shader("./Shaders/Basic.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);

    sh_fragment =
        load_shader("./Shaders/Basic.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

Shader App::load_shader(Str path, VkShaderStageFlagBits stage)
{
    Shader result;
    result.stage = stage;
    Raw code     = dump_file(path);

    VkShaderModuleCreateInfo create_info = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code.size,
        .pCode    = (u32*)code.buffer,
    };

    VK_CHECK(vkCreateShaderModule(device, &create_info, 0, &result.module));
    return result;
}

void App::init_pipeline()
{
    recompile_shaders();

    VkPipelineShaderStageCreateInfo vertex_create_info = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = sh_vertex.stage,
        .module = sh_vertex.module,
        .pName  = "main",
    };

    VkPipelineShaderStageCreateInfo fragment_create_info = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = sh_fragment.stage,
        .module = sh_fragment.module,
        .pName  = "main",
    };

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vertex_create_info,
        fragment_create_info,
    };

    auto dynamic_states = arr<VkDynamicState>(
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR);

    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = dynamic_states.count(),
        .pDynamicStates    = dynamic_states.elements,
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount   = 0,
        .vertexAttributeDescriptionCount = 0,
    };

    VkPipelineInputAssemblyStateCreateInfo input_asm_create_info = {
        .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkPipelineViewportStateCreateInfo viewport_create_info = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount  = 1,
    };

    VkPipelineRasterizationStateCreateInfo raster_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .polygonMode      = VK_POLYGON_MODE_FILL,
        .cullMode         = VK_CULL_MODE_BACK_BIT,
        .frontFace        = VK_FRONT_FACE_CLOCKWISE,
        .lineWidth        = 1.f,
    };

    VkPipelineMultisampleStateCreateInfo msaa_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable   = VK_FALSE,
        .minSampleShading      = 1.0f,
        .pSampleMask           = 0,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable      = VK_FALSE,
    };

    VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .blendEnable         = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp        = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp        = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo color_blend_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable   = VK_FALSE,
        .logicOp         = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments    = &color_blend_attachment,
        .blendConstants  = {0, 0, 0, 0},
    };

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = 0,
        .pSetLayouts            = 0,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = 0,
    };

    VK_CHECK(vkCreatePipelineLayout(
        device,
        &pipeline_layout_create_info,
        0,
        &pipeline_layout));

    VkAttachmentDescription color_attachment = {
        .format         = surface_format.format,
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

    VkSubpassDependency dependency = {
        .srcSubpass    = VK_SUBPASS_EXTERNAL,
        .dstSubpass    = 0,
        .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo render_pass_create_info = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments    = &color_attachment,
        .subpassCount    = 1,
        .pSubpasses      = &subpass,
        .dependencyCount = 1,
        .pDependencies   = &dependency,
    };

    VK_CHECK(
        vkCreateRenderPass(device, &render_pass_create_info, 0, &render_pass));

    VkGraphicsPipelineCreateInfo gfx_create_info = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount          = 2,
        .pStages             = shader_stages,
        .pVertexInputState   = &vertex_input_create_info,
        .pInputAssemblyState = &input_asm_create_info,
        .pViewportState      = &viewport_create_info,
        .pRasterizationState = &raster_create_info,
        .pMultisampleState   = &msaa_create_info,
        .pDepthStencilState  = 0,
        .pColorBlendState    = &color_blend_create_info,
        .pDynamicState       = &dynamic_state_create_info,
        .layout              = pipeline_layout,
        .renderPass          = render_pass,
        .subpass             = 0,
        .basePipelineHandle  = VK_NULL_HANDLE,
        .basePipelineIndex   = -1,
    };

    VK_CHECK(vkCreateGraphicsPipelines(
        device,
        VK_NULL_HANDLE,
        1,
        &gfx_create_info,
        0,
        &graphics_pipeline));

    // create framebuffers
    framebuffers.init_range(swap_chain_views.size);

    for (int i = 0; i < framebuffers.size; ++i) {
        VkImageView attachments[] = {swap_chain_views[i]};

        VkFramebufferCreateInfo fb_create_info = {
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass      = render_pass,
            .attachmentCount = 1,
            .pAttachments    = attachments,
            .width           = extent.width,
            .height          = extent.height,
            .layers          = 1,
        };

        VK_CHECK(
            vkCreateFramebuffer(device, &fb_create_info, 0, &framebuffers[i]));
    }

    // Create command pool
    QueueFamilyIndices families =
        find_queue_family_indices(arena, physical_device).unwrap();

    VkCommandPoolCreateInfo cmd_pool_create_info = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = (u32)families.graphics,
    };

    VK_CHECK(vkCreateCommandPool(device, &cmd_pool_create_info, 0, &cmd_pool));

    // Create cmd buffers
    {
        cmd_buffers.init(max_frames_in_flight);
        for (int i = 0; i < max_frames_in_flight; ++i) {
            cmd_buffers.add(create_cmd_buffer(cmd_pool).unwrap());
        }
    }

    // Create sync objects

    {
        sems_image_available.init(max_frames_in_flight);

        VkSemaphoreCreateInfo sem_create_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };

        for (int i = 0; i < max_frames_in_flight; ++i) {
            VkSemaphore sem;
            VK_CHECK(vkCreateSemaphore(device, &sem_create_info, 0, &sem));
            sems_image_available.add(sem);
        }
    }

    {
        sems_render_finished.init(max_frames_in_flight);
        VkSemaphoreCreateInfo sem_create_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        for (int i = 0; i < max_frames_in_flight; ++i) {
            VkSemaphore sem;
            VK_CHECK(vkCreateSemaphore(device, &sem_create_info, 0, &sem));
            sems_render_finished.add(sem);
        }
    }

    {
        fences_in_flight.init(max_frames_in_flight);

        VkFenceCreateInfo fence_create_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        for (int i = 0; i < max_frames_in_flight; ++i) {
            VkFence fence;
            VK_CHECK(vkCreateFence(device, &fence_create_info, 0, &fence));
            fences_in_flight.add(fence);
        }
    }
}

Result<VkCommandBuffer, VkResult> App::create_cmd_buffer(VkCommandPool pool)
{
    VkCommandBufferAllocateInfo info = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = cmd_pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer buffer;
    VK_RETURN_IF_ERR(vkAllocateCommandBuffers(device, &info, &buffer));

    return Ok(buffer);
}

void App::record_cmd_buffer(VkCommandBuffer buffer, u32 image_idx)
{
    VkCommandBufferBeginInfo begin_info = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags            = 0,
        .pInheritanceInfo = 0,
    };

    VK_CHECK(vkBeginCommandBuffer(buffer, &begin_info));

    VkClearValue clear_color = {
        .color = {.float32 = {0, 0, 0, 1}},
    };

    VkRenderPassBeginInfo render_pass_begin_info = {
        .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass  = render_pass,
        .framebuffer = framebuffers[image_idx],
        .renderArea =
            {
                .offset = {0, 0},
                .extent = extent,
            },
        .clearValueCount = 1,
        .pClearValues    = &clear_color,
    };

    vkCmdBeginRenderPass(
        buffer,
        &render_pass_begin_info,
        VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(
        buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        graphics_pipeline);

    VkViewport viewport = {
        .x        = 0.f,
        .y        = 0.f,
        .width    = (float)extent.width,
        .height   = (float)extent.height,
        .minDepth = 0.f,
        .maxDepth = 1.f,
    };

    VkRect2D scissor = {
        .offset =
            {
                .x = 0,
                .y = 0,
            },
        .extent = extent,
    };

    vkCmdSetViewport(buffer, 0, 1, &viewport);
    vkCmdSetScissor(buffer, 0, 1, &scissor);

    vkCmdDraw(buffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(buffer);

    VK_CHECK(vkEndCommandBuffer(buffer));
}

void App::deinit()
{
    for (int i = 0; i < max_frames_in_flight; ++i) {
        vkDestroySemaphore(device, sems_image_available[i], 0);
        vkDestroySemaphore(device, sems_render_finished[i], 0);
        vkDestroyFence(device, fences_in_flight[i], 0);
    }

    vkDestroyCommandPool(device, cmd_pool, 0);
    for (auto fb : framebuffers) {
        vkDestroyFramebuffer(device, fb, 0);
    }
    vkDestroyPipeline(device, graphics_pipeline, 0);
    vkDestroyPipelineLayout(device, pipeline_layout, 0);
    vkDestroyRenderPass(device, render_pass, 0);
    for (auto view : swap_chain_views) {
        vkDestroyImageView(device, view, nullptr);
    }
    vkDestroySwapchainKHR(device, swap_chain, 0);
    vkDestroySurfaceKHR(instance, surface, 0);
    window.destroy();
    vkDestroyDevice(device, 0);
    vkDestroyInstance(instance, 0);
    arena.release_base();
}

bool App::check_validation_layers(IAllocator& allocator)
{
    TArray<VkLayerProperties> props(&allocator);
    DEFER(props.release());

    u32 layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, 0);

    props.init_range(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, props.data);

    for (const char* layer : Validation_Layers) {
        bool found = false;

        for (const auto& prop : props) {
            if (strcmp(prop.layerName, layer) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            return false;
        }
    }

    return true;
}

bool App::is_physical_device_suitable(
    VkPhysicalDevice& device, IAllocator& allocator)
{
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures   features;

    vkGetPhysicalDeviceProperties(device, &properties);
    vkGetPhysicalDeviceFeatures(device, &features);

    u32 num_extensions;
    vkEnumerateDeviceExtensionProperties(device, 0, &num_extensions, 0);

    TArray<VkExtensionProperties> extensions(&allocator);
    extensions.init_range(num_extensions);
    DEFER(extensions.release());
    vkEnumerateDeviceExtensionProperties(
        device,
        0,
        &num_extensions,
        extensions.data);

    for (const char* required_extension : Device_Extensions) {
        bool found = false;
        for (VkExtensionProperties& props : extensions) {
            if (strcmp(props.extensionName, required_extension) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            return false;
        }
    }

    SwapChainSupportDetails details =
        query_swap_chain_support(allocator, device);
    DEFER(details.release());

    if ((details.formats.size == 0) || (details.present_modes.size == 0)) {
        return false;
    }

    if (config.use_discrete_gpu) {
        return properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    } else {
        return properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
    }
}

Result<QueueFamilyIndices, VkResult> App::find_queue_family_indices(
    IAllocator& allocator, VkPhysicalDevice device)
{
    QueueFamilyIndices families = {.graphics = -1, .present = -1};

    u32 family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, 0);

    TArray<VkQueueFamilyProperties> properties(&allocator);
    properties.init_range(family_count);
    DEFER(properties.release());

    vkGetPhysicalDeviceQueueFamilyProperties(
        device,
        &family_count,
        properties.data);

    for (int i = 0; i < properties.size; ++i) {
        VkQueueFamilyProperties& family = properties[i];

        // Graphics
        if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            families.graphics = i;
        }

        // Presentation
        VkBool32 surface_supported = 0;
        vkGetPhysicalDeviceSurfaceSupportKHR(
            device,
            i,
            surface,
            &surface_supported);

        if (surface_supported) {
            families.present = i;
        }
    }

    if ((families.graphics == -1) || (families.present == -1)) {
        return Err(VK_ERROR_UNKNOWN);
    }

    return Ok(families);
}

SwapChainSupportDetails App::query_swap_chain_support(
    IAllocator& allocator, VkPhysicalDevice qdevice)
{
    SwapChainSupportDetails result(allocator);

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        qdevice,
        surface,
        &result.capabilities);

    u32 format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(qdevice, surface, &format_count, 0);

    if (format_count != 0) {
        result.formats.init_range(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            qdevice,
            surface,
            &format_count,
            result.formats.data);
    }

    u32 present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        qdevice,
        surface,
        &present_mode_count,
        0);

    if (present_mode_count != 0) {
        result.present_modes.init_range(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            qdevice,
            surface,
            &present_mode_count,
            result.present_modes.data);
    }

    return result;
}
