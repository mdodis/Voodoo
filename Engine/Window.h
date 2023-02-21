#include "Compat/Win32Extra.h"
#include "Result.h"

struct Window {
    Win32::HWND        hwnd;
    Win32::WNDCLASSEXA wnd_class;
    bool is_open;
    
    Result<void, Win32::DWORD> init(i32 width, i32 height);
    void poll();
    void destroy();

    WIN32_DECLARE_WNDPROC(wnd_proc);

};