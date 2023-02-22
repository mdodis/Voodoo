#include "Result.h"
#include "VulkanCommon.h"

struct Window {
    Win32::HWND        hwnd;
    Win32::WNDCLASSEXA wnd_class;
    bool               is_open;

    Result<void, Win32::DWORD>     init(i32 width, i32 height);
    void                           poll();
    void                           destroy();
    Result<VkSurfaceKHR, VkResult> create_surface(VkInstance instance);

    WIN32_DECLARE_WNDPROC(wnd_proc);
};