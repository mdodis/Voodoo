#pragma once
#include "Memory/Base.h"
#include "VulkanCommon.h"
#include "Window.h"

struct Engine {
    Window*     window;
    bool        validation_layers = false;
    IAllocator& allocator;

    bool       is_initialized = false;
    int        current_frame  = 0;
    VkExtent2D extent         = {0, 0};

    VkInstance       instance;
    VkSurfaceKHR     surface;
    VkPhysicalDevice physical_device;
    VkDevice         device;
    VkCommandPool    cmd_pool;
    VkCommandBuffer  main_cmd_buffer;

    struct {
        u32     family;
        VkQueue queue;
    } graphics;

    struct {
        u32     family;
        VkQueue queue;
    } presentation;

    void init();
    void deinit();
};