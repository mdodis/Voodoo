#include "Delegates.h"
#include "Math/CG.h"
#include "Result.h"
#include "VulkanCommon.h"

struct Window {
    Win32::HWND        hwnd;
    Win32::WNDCLASSEXA wnd_class;
    bool               is_open;
    bool               needs_resize = false;

    Result<void, Win32::DWORD>     init(i32 width, i32 height);
    Vec2i                          get_extents() const;
    void                           poll();
    void                           destroy();
    Result<VkSurfaceKHR, VkResult> create_surface(VkInstance instance);

    Delegate<void> on_resized;

    WIN32_DECLARE_WNDPROC(wnd_proc);
};