#pragma once
#include "Window.h"
#include "Compat/Win32Base.h"

namespace win {

    struct Win32Window : public Window {
        Win32::HWND        hwnd;
        Win32::WNDCLASSEXA wnd_class;


        // Window interface
        virtual Result<void, EWindowError> init(i32 width, i32 height) override;
        virtual void get_extents(i32 &x, i32 &y) override;
        virtual void poll() override;
        virtual void destroy() override;
        virtual void imgui_new_frame() override;
        virtual Result<VkSurfaceKHR, VkResult> create_surface(VkInstance instance) override;
        virtual void set_lock_cursor(bool value) override;
        virtual Slice<const char*> get_required_extensions() override;
        // ~ Window Interface

        WIN32_DECLARE_WNDPROC(wnd_proc);

    private:
        bool needs_resize = false;
        bool cursor_locked = false;
    };

}
