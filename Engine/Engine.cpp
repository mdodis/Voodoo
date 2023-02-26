#include "Engine.h"

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
    }

    // Create graphics command pool
    {
        VkCommandPoolCreateInfo create_info = {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = graphics.family,
        };

        VK_CHECK(vkCreateCommandPool(device, &create_info, 0, &cmd_pool));
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
    init_framebuffers();
    init_sync_objects();

    is_initialized = true;
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
    }
}

void Engine::init_sync_objects()
{
    VkFenceCreateInfo fnc_create_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    VK_CHECK(vkCreateFence(device, &fnc_create_info, 0, &fnc_render));

    VkSemaphoreCreateInfo sem_create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VK_CHECK(vkCreateSemaphore(device, &sem_create_info, 0, &sem_present));
    VK_CHECK(vkCreateSemaphore(device, &sem_create_info, 0, &sem_render));
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

    VkOffset2D            offset        = {};
    VkRenderPassBeginInfo rp_begin_info = {
        .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass  = render_pass,
        .framebuffer = framebuffers[next_image_index],
        .renderArea =
            {
                .offset = offset,
                .extent = extent,
            },
        .clearValueCount = 1,
        .pClearValues    = &clear_value,
    };
    vkCmdBeginRenderPass(cmd, &rp_begin_info, VK_SUBPASS_CONTENTS_INLINE);

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
    vkDestroyCommandPool(device, cmd_pool, 0);
    vkDestroySwapchainKHR(device, swap_chain, 0);
    vkDestroyRenderPass(device, render_pass, 0);

    for (u32 i = 0; i < framebuffers.size; ++i) {
        vkDestroyFramebuffer(device, framebuffers[i], 0);
        vkDestroyImageView(device, swap_chain_image_views[i], 0);
    }

    vkDestroyDevice(device, 0);
    vkDestroySurfaceKHR(instance, surface, 0);
    vkDestroyInstance(instance, 0);
}