#pragma once
#pragma once
#include "Compat/Win32Base.h"
#include "Win32DnD.h"
#include "Window.h"

namespace win {

    struct DnDHandler : public Win32::IDnD {
        virtual Win32::HRESULT drag_enter(
            Win32::DataObject& data_object, Win32::DWORD key_state) override;
        virtual void           drag_leave() override;
        virtual Win32::HRESULT drag_over(
            Win32::DWORD key_state, Win32::POINT point) override;
        virtual void           drag_drop_begin() override;
        virtual Win32::HRESULT drag_drop(Str path) override;
        virtual void           drag_drop_finish() override;

        void clear();

        struct Win32Window* parent;
        TArray<Str>         dropped_files{&System_Allocator};
    };

    struct Win32Window : public Window {
        Win32::HWND        hwnd;
        Win32::WNDCLASSEXA wnd_class;
        Win32::DnDHandle   dnd_handle;
        DnDHandler         dnd;

        // Window interface
        virtual Result<void, EWindowError> init(i32 width, i32 height) override;
        virtual void                       get_extents(i32& x, i32& y) override;
        virtual void                       poll() override;
        virtual void                       destroy() override;
        virtual void                       imgui_new_frame() override;
        virtual void                       imgui_process() override;
        virtual Result<VkSurfaceKHR, VkResult> create_surface(
            VkInstance instance) override;
        virtual void                set_lock_cursor(bool value) override;
        virtual Slice<const char*>  get_required_extensions() override;
        virtual EWindowSupportFlags get_support_flags() override;
        // ~ Window Interface

        WIN32_DECLARE_WNDPROC(wnd_proc);

    private:
        bool needs_resize       = false;
        bool cursor_locked      = false;
    };

}  // namespace win
