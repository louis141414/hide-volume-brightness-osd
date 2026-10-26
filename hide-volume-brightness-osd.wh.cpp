// ==WindhawkMod==
// @id              hide-volume-brightness-osd
// @name            Hide Volume & Brightness OSD
// @description     Hides Windows volume, brightness and media OSDs on Windows 10/11. Blocks the OSD from being shown.
// @version         2.1
// @author          NietNormaleMeneer
// @github          https://github.com/louis141414
// @homepage        https://ko-fi.com/nietnormalemeneer
// @include         explorer.exe
// @include         ShellExperienceHost.exe
// @include         ShellHost.exe
// @compilerOptions -luser32
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Hide Volume & Brightness OSD

Blocks the Windows volume / brightness / media OSDs from appearing.

Designed for **Windows 11** (also works on Windows 10).

### Support
**[https://ko-fi.com/nietnormalemeneer](https://ko-fi.com/nietnormalemeneer)**

Source: [https://github.com/louis141414/hide-volume-brightness-osd](https://github.com/louis141414/hide-volume-brightness-osd)
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- enabled: true
  $name: Enable mod
  $description: Turn the OSD hiding on or off.
*/
// ==/WindhawkModSettings==

#include <windows.h>

bool g_enabled = true;
UINT_PTR g_timerId = 0;

// -------------------- Detection --------------------

bool IsModernOsdHost(HWND hwnd)
{
    if (!hwnd || !IsWindow(hwnd))
        return false;

    wchar_t cls[256]{};
    if (!GetClassNameW(hwnd, cls, 256))
        return false;

    if (_wcsicmp(cls, L"XamlExplorerHostIslandWindow") != 0)
        return false;

    HWND bridge = FindWindowExW(hwnd, nullptr,
        L"Windows.UI.Composition.DesktopWindowContentBridge", nullptr);
    if (!bridge)
        return false;

    if (!FindWindowExW(bridge, nullptr, L"DesktopWindowXamlSource", nullptr))
        return false;

    RECT rc{};
    if (!GetWindowRect(hwnd, &rc))
        return false;

    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    return (w > 40 && w < 1200 && h > 20 && h < 600);
}

bool IsClassicOsdHost(HWND hwnd)
{
    if (!hwnd || !IsWindow(hwnd))
        return false;

    wchar_t cls[256]{};
    if (!GetClassNameW(hwnd, cls, 256))
        return false;

    if (_wcsicmp(cls, L"NativeHWNDHost") != 0)
        return false;

    if (!FindWindowExW(hwnd, nullptr, L"DirectUIHWND", nullptr))
        return false;

    RECT rc{};
    if (!GetWindowRect(hwnd, &rc))
        return false;

    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    return (w > 40 && w < 1200 && h > 20 && h < 600);
}

bool IsOsdWindow(HWND hwnd)
{
    return IsModernOsdHost(hwnd) || IsClassicOsdHost(hwnd);
}

void ForceHide(HWND hwnd)
{
    if (!hwnd || !IsWindow(hwnd))
        return;

    ShowWindow(hwnd, SW_HIDE);
    ShowWindow(hwnd, SW_MINIMIZE);

    SetWindowPos(hwnd, nullptr, -32000, -32000, 0, 0,
        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void HideAllOsds()
{
    if (!g_enabled)
        return;

    HWND hwnd = nullptr;
    while ((hwnd = FindWindowExW(nullptr, hwnd, L"XamlExplorerHostIslandWindow", nullptr)) != nullptr)
    {
        if (IsModernOsdHost(hwnd))
        {
            ForceHide(hwnd);
            Wh_Log(L"Hidden modern OSD hwnd=%p", hwnd);
        }
    }

    hwnd = nullptr;
    while ((hwnd = FindWindowExW(nullptr, hwnd, L"NativeHWNDHost", nullptr)) != nullptr)
    {
        if (IsClassicOsdHost(hwnd))
        {
            ForceHide(hwnd);
            Wh_Log(L"Hidden classic OSD hwnd=%p", hwnd);
        }
    }
}

// -------------------- Hooks --------------------

using ShowWindow_t = decltype(&ShowWindow);
ShowWindow_t ShowWindow_Original;

BOOL WINAPI ShowWindow_Hook(HWND hWnd, int nCmdShow)
{
    if (g_enabled && nCmdShow != SW_HIDE && nCmdShow != SW_MINIMIZE && IsOsdWindow(hWnd))
    {
        Wh_Log(L"Blocked ShowWindow on OSD hwnd=%p cmd=%d", hWnd, nCmdShow);
        return ShowWindow_Original(hWnd, SW_HIDE);
    }
    return ShowWindow_Original(hWnd, nCmdShow);
}

using SetWindowPos_t = decltype(&SetWindowPos);
SetWindowPos_t SetWindowPos_Original;

BOOL WINAPI SetWindowPos_Hook(HWND hWnd, HWND hWndInsertAfter,
    int X, int Y, int cx, int cy, UINT uFlags)
{
    if (g_enabled && IsOsdWindow(hWnd))
    {
        if (!(uFlags & SWP_NOMOVE))
        {
            X = -32000;
            Y = -32000;
        }
        uFlags |= SWP_NOACTIVATE;
        uFlags &= ~SWP_SHOWWINDOW;
        uFlags |= SWP_HIDEWINDOW;

        Wh_Log(L"Blocked/redirected SetWindowPos on OSD hwnd=%p", hWnd);
    }
    return SetWindowPos_Original(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
}

// -------------------- Timer --------------------

void CALLBACK TimerProc(HWND, UINT, UINT_PTR, DWORD)
{
    HideAllOsds();
}

void StartTimer()
{
    if (g_timerId)
    {
        KillTimer(nullptr, g_timerId);
        g_timerId = 0;
    }
    if (g_enabled)
        g_timerId = SetTimer(nullptr, 0, 80, TimerProc);
}

void StopTimer()
{
    if (g_timerId)
    {
        KillTimer(nullptr, g_timerId);
        g_timerId = 0;
    }
}

// -------------------- Entry points --------------------

void LoadSettings()
{
    g_enabled = Wh_GetIntSetting(L"enabled") != 0;
}

BOOL Wh_ModInit()
{
    Wh_Log(L"Hide Volume & Brightness OSD v2.1 (Win11) – Init");

    LoadSettings();

    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32)
    {
        void* pShowWindow = (void*)GetProcAddress(user32, "ShowWindow");
        void* pSetWindowPos = (void*)GetProcAddress(user32, "SetWindowPos");

        if (pShowWindow)
            Wh_SetFunctionHook(pShowWindow, (void*)ShowWindow_Hook, (void**)&ShowWindow_Original);

        if (pSetWindowPos)
            Wh_SetFunctionHook(pSetWindowPos, (void*)SetWindowPos_Hook, (void**)&SetWindowPos_Original);
    }

    StartTimer();
    HideAllOsds();

    return TRUE;
}

void Wh_ModUninit()
{
    Wh_Log(L"Hide Volume & Brightness OSD – Uninit");
    StopTimer();
}

void Wh_ModSettingsChanged()
{
    LoadSettings();
    StartTimer();
    if (g_enabled)
        HideAllOsds();
}
