#include "Window.h"

#include "Memory/Extras.h"
#if OS_MSWINDOWS
#include "Win32Window.h"
#elif OS_LINUX
#include "GLFWWindow.h"
#endif

namespace win {
    Window* create_window(Allocator& allocator)
    {
        Window* result = 0;
#if OS_MSWINDOWS
        Win32Window* win = alloc<Win32Window>(allocator);
        result           = (Window*)win;
#elif OS_LINUX
        GLFWWindow* glfw = alloc<GLFWWindow>(allocator);
        result           = (Window*)glfw;
#endif
        return result;
    }
}  // namespace win
