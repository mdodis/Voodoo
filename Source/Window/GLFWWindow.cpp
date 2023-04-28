#define GLFW_INCLUDE_VULKAN
#include "GLFWWindow.h"
#define MOK_GLFW_VIRTUAL_KEYCODES_AUTO_INCLUDE 0
#include "Compat/GLFWVirtualKeycodes.h"

namespace win {

    Result<void, EWindowError> GLFWWindow::init(i32 width, i32 height)
    {
        if (glfwInit() == GLFW_FALSE) {
            return Err(WindowError::Register);
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow(width, height, "VKXWindow", NULL, NULL);
        if (!window) return Err(WindowError::Unknown);

        glfwGetCursorPos(window, &last_xpos, &last_ypos);

        glfwSetWindowUserPointer(window, (void*)this);
        // Bind events
        glfwSetKeyCallback(window, GLFWWindow::callback_keyboard);
        glfwSetCursorPosCallback(window, GLFWWindow::callback_cursor_pos);
        //        glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_FALSE);

        if (glfwRawMouseMotionSupported()) {
            print(LIT("Raw mouse motion: supported\n"));
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        }

        ASSERT(glfwVulkanSupported() == GLFW_TRUE);

        is_open = true;
        return Ok<void>();
    }

    void GLFWWindow::get_extents(i32& x, i32& y)
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

    void GLFWWindow::destroy() { glfwDestroyWindow(window); }

    Result<VkSurfaceKHR, VkResult> GLFWWindow::create_surface(
        VkInstance instance)
    {
        VkSurfaceKHR surface;
        VkResult     result =
            glfwCreateWindowSurface(instance, window, NULL, &surface);

        if (result != VK_SUCCESS) return Err(result);

        return Ok(surface);
    }

    void GLFWWindow::set_lock_cursor(bool value)
    {
        glfwSetInputMode(
            window,
            GLFW_CURSOR,
            value ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        return;
    }

    Slice<const char*> GLFWWindow::get_required_extensions()
    {
        u32          count;
        const char** extensions = glfwGetRequiredInstanceExtensions(&count);
        ASSERT(extensions != NULL);
        return slice(extensions, count);
    }

    EWindowSupportFlags GLFWWindow::get_support_flags()
    {
        return EWindowSupportFlags(0);
    }

    void GLFWWindow::callback_keyboard(
        ::GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        GLFWWindow* self = (GLFWWindow*)glfwGetWindowUserPointer(window);
        if (action == GLFW_REPEAT) return;

        bool down = (action == GLFW_PRESS);
        self->input->send_input(glfw_key_to_input_key(key), down);
    }

    void GLFWWindow::callback_cursor_pos(
        GLFWwindow* window, double xpos, double ypos)
    {
        GLFWWindow* self = (GLFWWindow*)glfwGetWindowUserPointer(window);

        self->input
            ->send_axis_delta(InputAxis::MouseX, (xpos - self->last_xpos));
        self->input
            ->send_axis_delta(InputAxis::MouseY, (ypos - self->last_ypos));

        self->last_xpos = xpos;
        self->last_ypos = ypos;
    }

}  // namespace win
