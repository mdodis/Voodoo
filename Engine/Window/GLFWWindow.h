#pragma once
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "Window.h"

namespace win {

    struct GLFWWindow : public Window {
        ::GLFWwindow* window;

        // Window interface
        virtual Result<void, EWindowError> init(i32 width, i32 height) override;
        virtual void                       get_extents(i32& x, i32& y) override;
        virtual void                       poll() override;
        virtual void                       destroy() override;
        virtual void                       imgui_new_frame() override;
        virtual Result<VkSurfaceKHR, VkResult> create_surface(
            VkInstance instance) override;
        virtual void                set_lock_cursor(bool value) override;
        virtual Slice<const char*>  get_required_extensions() override;
        virtual EWindowSupportFlags get_support_flags() override;
        // ~ Window interface

    private:
        static void callback_keyboard(
            ::GLFWwindow* window, int key, int scancode, int action, int mods);
        static void callback_cursor_pos(
            ::GLFWwindow* window, double xpos, double ypos);

        double last_xpos, last_ypos;
    };

}  // namespace win
