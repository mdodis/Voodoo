#include "Win32ImGui.h"

#include <windows.h>

#include "backends/imgui_impl_win32.h"

void win32_imgui_init(Win32::HWND hwnd) { ImGui_ImplWin32_Init((void*)hwnd); }

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool win32_imgui_wndproc(
    Win32::HWND   hwnd,
    Win32::UINT   msg,
    Win32::WPARAM wparam,
    Win32::LPARAM lparam)
{
    if (ImGui_ImplWin32_WndProcHandler(
            (HWND)hwnd,
            (UINT)msg,
            (WPARAM)wparam,
            (LPARAM)lparam))
    {
        return true;
    }
    return false;
}