#pragma once
#include "Base.h"
#include "Delegates.h"
#include "Input.h"
#include "Types.h"
#include "VulkanCommon.h"

namespace win {
    namespace WindowError {
        enum Type : u32
        {
            Unknown = 0,
            InvalidFormat,
            Register
        };
    }
    typedef WindowError::Type EWindowError;

    namespace WindowSupportFlags {
        enum Type : u32
        {
            DragAndDrop = 1 << 0,
        };
    }
    typedef WindowSupportFlags::Type EWindowSupportFlags;

    struct Window {
        Input*         input;
        bool           is_open;
        bool           is_dragging_files  = false;
        bool           just_dropped_files = false;
        Delegate<void> on_resized;

        virtual Result<void, EWindowError>     init(i32 width, i32 height) = 0;
        virtual void                           get_extents(i32& x, i32& y) = 0;
        virtual void                           poll()                      = 0;
        virtual void                           destroy()                   = 0;
        virtual Result<VkSurfaceKHR, VkResult> create_surface(
            VkInstance instance)                                = 0;
        virtual void                set_lock_cursor(bool value) = 0;
        virtual Slice<const char*>  get_required_extensions()   = 0;
        virtual EWindowSupportFlags get_support_flags()         = 0;
    };

    Window* create_window(IAllocator& allocator);
}  // namespace win

PROC_FMT_ENUM(win::WindowError, {
    FMT_ENUM_CASE(win::WindowError, Unknown);
    FMT_ENUM_CASE(win::WindowError, InvalidFormat);
    FMT_ENUM_CASE(win::WindowError, Register);
})
