#pragma once
#define MOK_WIN32_NO_FUNCTIONS
#include "Compat/Win32Base.h"

void win32_imgui_init(Win32::HWND hwnd);

bool win32_imgui_wndproc(
    Win32::HWND   hwnd,
    Win32::UINT   msg,
    Win32::WPARAM wparam,
    Win32::LPARAM lparam);