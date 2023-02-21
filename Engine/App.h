#pragma once
#include "Window.h"
#include <vulkan/vulkan.h>
#include "Result.h"
#include "Containers/Array.h"

struct App {
    Window window;
    VkInstance instance;

    Result<void, VkResult> init_vulkan();
};
