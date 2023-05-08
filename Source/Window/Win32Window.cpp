#include "Win32Window.h"

#include "Compat/Win32VirtualKeycodes.h"
#include "Win32Vulkan.h"

namespace win {
    static WIN32_DECLARE_WNDPROC(wnd_proc_handler);

    Result<void, EWindowError> Win32Window::init(i32 width, i32 height)
    {
        Win32::SetProcessDPIAware();

        // Register class
        wnd_class            = {};
        wnd_class.size       = sizeof(wnd_class);
        wnd_class.cursor     = Win32::LoadCursorA(0, Win32::Cursor::A_Arrow);
        wnd_class.class_name = "VKXWindow";
        wnd_class.instance   = Win32::GetModuleHandleA(0);
        wnd_class.wndproc    = wnd_proc_handler;

        if (!Win32::RegisterClassExA(&wnd_class)) {
            return Err(WindowError::Register);
        }

        // Create window
        auto        style = Win32::Style::OverlappedWindow;
        Win32::RECT rect  = {
             .left   = 0,
             .top    = 0,
             .right  = width,
             .bottom = height,
        };

        Win32::AdjustWindowRect(&rect, style, 0);

        hwnd = Win32::CreateWindowExA(
            0,
            wnd_class.class_name,
            "Voodoo",
            style,
            Win32::DefaultWindowPos,
            Win32::DefaultWindowPos,
            rect.right - rect.left,
            rect.bottom - rect.top,
            NULL,
            NULL,
            wnd_class.instance,
            0);

        Win32::SetWindowLongPtrA(
            hwnd,
            Win32::WindowLongPointer::UserData,
            (Win32::LONG_PTR)this);

        if (!hwnd) {
            return Err(WindowError::InvalidFormat);
        }

        Win32::ShowWindow(hwnd, Win32::ShowMode::Show);
        Win32::UpdateWindow(hwnd);
        is_open = true;

        // Raw mouse input
        Win32::RAWINPUTDEVICE rid[1];
        rid[0].usUsagePage = Win32::HIDUsagePage::Generic;
        rid[0].usUsage     = Win32::HIDUsage::Mouse;
        rid[0].dwFlags     = Win32::HIDMode::InputSink;
        rid[0].hwndTarget  = hwnd;
        Win32::RegisterRawInputDevices(rid, 1, sizeof(rid));

        // Initialize drag and drop
        dnd.parent        = this;
        dnd_handle        = Win32::register_dnd(hwnd, &dnd);
        is_dragging_files = false;

        hooks.post_init.broadcast((void*)&hwnd);

        return Ok<void>();
    }

    void Win32Window::get_extents(i32& x, i32& y)
    {
        Win32::RECT r;
        Win32::GetClientRect(hwnd, &r);
        x = r.right - r.left;
        y = r.bottom - r.top;
    }

    void Win32Window::poll()
    {
        needs_resize = false;

        Win32::MSG message;
        while (Win32::PeekMessageA(
            &message,
            0,
            0,
            0,
            Win32::PeekMessageOption::Remove))
        {
            Win32::TranslateMessage(&message);
            Win32::DispatchMessageA(&message);
        }

        if (needs_resize) {
            on_resized.call_safe();
        }
    }

    void Win32Window::destroy() { Win32::DestroyWindow(hwnd); }

    Result<VkSurfaceKHR, VkResult> Win32Window::create_surface(
        VkInstance instance)
    {
        return win32_create_surface(instance, hwnd, Win32::GetModuleHandleA(0));
    }

    void Win32Window::set_lock_cursor(bool value)
    {
        cursor_locked = value;
        Win32::ShowCursor(!cursor_locked);
    }

    Slice<const char*> Win32Window::get_required_extensions()
    {
        return win32_get_required_extensions();
    }

    EWindowSupportFlags Win32Window::get_support_flags()
    {
        return EWindowSupportFlags(WindowSupportFlags::DragAndDrop);
    }

    WIN32_DECLARE_WNDPROC(Win32Window::wnd_proc)
    {
        if (!cursor_locked) {
            bool                     handled = false;
            Win32::WndProcParameters params  = {
                 .hwnd   = hwnd,
                 .msg    = msg,
                 .wparam = wparam,
                 .lparam = lparam,
            };
            hooks.pre_poll.broadcast((void*)&hwnd, (void*)&params, &handled);

            if (handled) return 0;
        }

        Win32::LRESULT result = 0;
        switch (msg) {
            case Win32::Message::Destroy: {
                Win32::PostQuitMessage(0);
                is_open = false;
            } break;

            case Win32::Message::Size: {
                needs_resize = true;
            } break;

            case Win32::Message::KeyDown:
            case Win32::Message::KeyUp: {
                bool     down = msg == Win32::Message::KeyDown;
                InputKey key  = wparam_to_input_key(wparam);

                input->send_input(key, down);
            } break;

            case Win32::Message::Input: {
                unsigned               size = sizeof(Win32::RAWINPUT);
                static Win32::RAWINPUT raw[sizeof(Win32::RAWINPUT)];
                GetRawInputData(
                    (Win32::HRAWINPUT)lparam,
                    Win32::RawInputCommand::Input,
                    raw,
                    &size,
                    sizeof(Win32::RAWINPUTHEADER));

                if (raw->data.mouse.usFlags &
                    Win32::RawInputMouseState::MoveAbsolute)
                {
                    print(LIT("ABSOLUTE MOVE\n"));
                }

                if (raw->header.dwType == Win32::RawInputDataType::Mouse) {
                    input->send_axis_delta(
                        InputAxis::MouseX,
                        (float)raw->data.mouse.lLastX);
                    input->send_axis_delta(
                        InputAxis::MouseY,
                        (float)raw->data.mouse.lLastY);
                }

            } break;

            default: {
                result = Win32::DefWindowProcA(hwnd, msg, wparam, lparam);
            } break;
        }

        if (cursor_locked) {
            Win32::RECT r;
            Win32::GetClientRect(hwnd, &r);
            Win32::SetCursorPos(
                r.left + (r.right - r.left) / 2,
                r.top + (r.bottom - r.top) / 2);
        }
        return result;
    }

    WIN32_DECLARE_WNDPROC(wnd_proc_handler)
    {
        Win32Window* w = (Win32Window*)
            Win32::GetWindowLongPtrA(hwnd, Win32::WindowLongPointer::UserData);
        if (w == 0) {
            return Win32::DefWindowProcA(hwnd, msg, wparam, lparam);
        }
        return w->wnd_proc(hwnd, msg, wparam, lparam);
    }

    Win32::HRESULT DnDHandler::drag_enter(
        Win32::DataObject& data_object, Win32::DWORD key_state)
    {
        parent->is_dragging_files = true;
        parent->hooks.drag_enter.broadcast((void*)&parent->hwnd);
        return Win32::HResult::Ok;
    }

    void DnDHandler::drag_leave() { parent->is_dragging_files = false; }

    Win32::HRESULT DnDHandler::drag_over(
        Win32::DWORD key_state, Win32::POINT point)
    {
        parent->hooks.drag_over
            .broadcast((void*)&parent->hwnd, point.x, point.y);
        return Win32::HResult::Ok;
    }

    void DnDHandler::drag_drop_begin() { clear(); }

    Win32::HRESULT DnDHandler::drag_drop(Str path)
    {
        dropped_files.add(path.clone(System_Allocator));
        return Win32::HResult::Ok;
    }

    void DnDHandler::drag_drop_finish()
    {
        parent->hooks.drag_drop_finish.broadcast((void*)&parent->hwnd);
        parent->just_dropped_files = true;
    }

    void DnDHandler::clear()
    {
        for (Str s : dropped_files) {
            System_Allocator.release((umm)s.data);
        }
        dropped_files.empty();
    }

}  // namespace win
