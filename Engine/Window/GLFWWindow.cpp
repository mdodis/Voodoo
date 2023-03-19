#define GLFW_INCLUDE_VULKAN
#include "GLFWWindow.h"
#include "backends/imgui_impl_glfw.h"

namespace win {

    Result<void, EWindowError> GLFWWindow::init(i32 width, i32 height)
    {
        if (glfwInit() == GLFW_FALSE) {
            return Err(WindowError::Register);
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow(width, height, "VKXWindow", NULL, NULL);
        if (!window) return Err(WindowError::Unknown);

        ASSERT(glfwVulkanSupported() == GLFW_TRUE);
        ImGui_ImplGlfw_InitForVulkan(window, true);

        is_open = true;
        return Ok<void>();
    }

    void GLFWWindow::get_extents(i32 &x, i32 &y)
    {
        glfwGetWindowSize(window, &x, &y);
    }

    void GLFWWindow::poll()
    {
        glfwPollEvents();
        if (is_open) {
            is_open = !glfwWindowShouldClose(window);
        }
    }

    void GLFWWindow::destroy()
    {
        ImGui_ImplGlfw_Shutdown();
        glfwDestroyWindow(window);
    }

    void GLFWWindow::imgui_new_frame()
    {
        ImGui_ImplGlfw_NewFrame();
    }

    Result<VkSurfaceKHR, VkResult> GLFWWindow::create_surface(VkInstance instance)
    {
        VkSurfaceKHR surface;
        VkResult result = glfwCreateWindowSurface(instance, window, NULL, &surface);

        if (result != VK_SUCCESS) return Err(result);

        return Ok(surface);
    }

    void GLFWWindow::set_lock_cursor(bool value)
    {
        return;
    }

    Slice<const char *> GLFWWindow::get_required_extensions()
    {
        u32 count;
        const char** extensions = glfwGetRequiredInstanceExtensions(&count);
        ASSERT(extensions != NULL);
        return slice(extensions, count);
    }
}
