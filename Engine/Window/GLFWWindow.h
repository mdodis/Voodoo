#pragma once
#define GLFW_INCLUDE_VULKAN
#include "Window.h"
#include "GLFW/glfw3.h"

namespace win {

    struct GLFWWindow : public Window {
        ::GLFWwindow *window;

        // Window interface
        virtual Result<void, EWindowError> init(i32 width, i32 height) override;
        virtual void get_extents(i32 &x, i32 &y) override;
        virtual void poll() override;
        virtual void destroy() override;
        virtual void imgui_new_frame() override;
        virtual Result<VkSurfaceKHR, VkResult> create_surface(VkInstance instance) override;
        virtual void set_lock_cursor(bool value) override;
        virtual Slice<const char *> get_required_extensions() override;
        // ~ Window interface

    };

}
