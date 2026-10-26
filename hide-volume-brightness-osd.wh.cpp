// ==WindhawkMod==
// @id              hide-volume-brightness-osd
// @name            Hide Volume & Brightness OSD
// @description     Hides Windows volume, brightness and media OSDs. No admin required. Works on Windows 11 25H2.
// @version         3.0
// @author          NietNormaleMeneer
// @github          https://github.com/louis141414
// @homepage        https://ko-fi.com/nietnormalemeneer
// @include         explorer.exe
// @compilerOptions -luser32
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Hide Volume & Brightness OSD

Hides the Windows volume / brightness / media OSDs.

Uses a **system-wide accessibility event hook** (no admin, works even when the OSD lives in another process).

Tested approach for Windows 11 25H2.

### Support
**[https://ko-fi.com/nietnormalemeneer](https://ko-fi.com/nietnormalemeneer)**

Source: [https://github.com/louis141414/hide-volume-brightness-osd](https://github.com/louis141414/hide-volume-brightness-osd)
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- enabled: true
  $name: Enable mod
  $description: Turn OSD hiding on or off.
- aggressive: true
  $name: Aggressive mode
  $description: Hide more XAML host windows that look like OSDs. Turn off if something else disappears.
*/
// ==/WindhawkModSettings==

#include <windows.h>

bool g_enabled = true;
bool g_aggressive = true;

HWINEVENTHOOK g_showHook = nullptr;
UINT_PTR g_timerId = 0;

// -------------------- Detection --------------------

bool ClassEquals(HWND hwnd, const wchar_t* name)
{
    wchar_t cls[256]{};
    if (!GetClassNameW(hwnd, cls, 256))
        return false;
    return _wcsicmp(cls, name) == 0;
}

bool HasChildClass(HWND parent, const wchar_t* childClass)
{
    return FindWindowExW(parent, nullptr, childClass, nullptr) != nullptr;
}

bool LooksLikeOsdSize(HWND hwnd)
{
    RECT rc{};
    if (!GetWindowRect(hwnd, &rc))
        return false;

    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    // Compact volume/brightness bar OR larger media flyout
    if (w <= 0 || h <= 0)
        return false;

    if (g_aggressive)
        return (w >= 20 && w <= 1400 && h >= 15 && h <= 700);

    return (w >= 40 && w <= 1200 && h >= 20 && h <= 600);
}

bool IsModernOsd(HWND hwnd)
{
    if (!ClassEquals(hwnd, L"XamlExplorerHostIslandWindow"))
        return false;

    // Common Win11 OSD tree
    HWND bridge = FindWindowExW(hwnd, nullptr,
        L"Windows.UI.Composition.DesktopWindowContentBridge", nullptr);
    if (bridge)
    {
        if (HasChildClass(bridge, L"DesktopWindowXamlSource"))
            return LooksLikeOsdSize(hwnd);
    }

    // Fallback: any XAML source under this host with OSD-like size
    if (g_aggressive && HasChildClass(hwnd, L"DesktopWindowXamlSource"))
        return LooksLikeOsdSize(hwnd);

    // Another possible intermediate
    if (g_aggressive)
    {
        HWND child = nullptr;
        while ((child = FindWindowExW(hwnd, child, nullptr, nullptr)) != nullptr)
        {
            if (HasChildClass(child, L"DesktopWindowXamlSource") && LooksLikeOsdSize(hwnd))
                return true;
        }
    }

    return false;
}

bool IsClassicOsd(HWND hwnd)
{
    if (!ClassEquals(hwnd, L"NativeHWNDHost"))
        return false;

    if (!HasChildClass(hwnd, L"DirectUIHWND"))
        return false;

    return LooksLikeOsdSize(hwnd);
}

bool IsOsdWindow(HWND hwnd)
{
    if (!hwnd || !IsWindow(hwnd))
        return false;
    return IsModernOsd(hwnd) || IsClassicOsd(hwnd);
}

void ForceHide(HWND hwnd)
{
    if (!hwnd || !IsWindow(hwnd))
        return;

    // Hide + minimize + park off-screen
    ShowWindow(hwnd, SW_HIDE);
    ShowWindow(hwnd, SW_MINIMIZE);
    SetWindowPos(hwnd, nullptr, -32000, -32000, 0, 0,
        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_HIDEWINDOW);
}

void HideAllOsds()
{
    if (!g_enabled)
        return;

    HWND hwnd = nullptr;
    while ((hwnd = FindWindowExW(nullptr, hwnd, L"XamlExplorerHostIslandWindow", nullptr)) != nullptr)
    {
        if (IsModernOsd(hwnd))
        {
            ForceHide(hwnd);
            Wh_Log(L"Hide modern OSD %p", hwnd);
        }
    }

    hwnd = nullptr;
    while ((hwnd = FindWindowExW(nullptr, hwnd, L"NativeHWNDHost", nullptr)) != nullptr)
    {
        if (IsClassicOsd(hwnd))
        {
            ForceHide(hwnd);
            Wh_Log(L"Hide classic OSD %p", hwnd);
        }
    }
}

// -------------------- System-wide show event --------------------
// This is the important part for 25H2: we see the window the moment
// ANY process shows it, without needing to inject into that process.

void CALLBACK WinEventProc(
    HWINEVENTHOOK, DWORD event, HWND hwnd,
    LONG idObject, LONG idChild, DWORD, DWORD)
{
    if (!g_enabled)
        return;

    if (event != EVENT_OBJECT_SHOW)
        return;

    // Only top-level / interesting objects
    if (idObject != OBJID_WINDOW || idChild != CHILDID_SELF)
        return;

    if (!hwnd || !IsWindow(hwnd))
        return;

    // Quick class filter before heavier checks
    wchar_t cls[256]{};
    if (!GetClassNameW(hwnd, cls, 256))
        return;

    if (_wcsicmp(cls, L"XamlExplorerHostIslandWindow") != 0 &&
        _wcsicmp(cls, L"NativeHWNDHost") != 0)
        return;

    if (IsOsdWindow(hwnd))
    {
        Wh_Log(L"EVENT_OBJECT_SHOW OSD %p class=%s", hwnd, cls);
        ForceHide(hwnd);

        // Hide again a few times – Windows sometimes re-shows it
        for (int i = 0; i < 5; ++i)
        {
            Sleep(30);
            if (IsWindow(hwnd))
                ForceHide(hwnd);
        }
    }
}

void StartShowHook()
{
    if (g_showHook)
        return;

    g_showHook = SetWinEventHook(
        EVENT_OBJECT_SHOW, EVENT_OBJECT_SHOW,
        nullptr,
        WinEventProc,
        0, 0, // all processes / all threads
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);

    if (g_showHook)
        Wh_Log(L"Show-event hook installed");
    else
        Wh_Log(L"Show-event hook FAILED err=%u", GetLastError());
}

void StopShowHook()
{
    if (g_showHook)
    {
        UnhookWinEvent(g_showHook);
        g_showHook = nullptr;
    }
}

// -------------------- Backup timer --------------------

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
        g_timerId = SetTimer(nullptr, 0, 100, TimerProc);
}

void StopTimer()
{
    if (g_timerId)
    {
        KillTimer(nullptr, g_timerId);
        g_timerId = 0;
    }
}

// -------------------- Settings / entry --------------------

void LoadSettings()
{
    g_enabled = Wh_GetIntSetting(L"enabled") != 0;
    g_aggressive = Wh_GetIntSetting(L"aggressive") != 0;
}

BOOL Wh_ModInit()
{
    Wh_Log(L"Hide Volume & Brightness OSD v3.0 – Init (25H2 / no admin)");

    LoadSettings();
    StartShowHook();
    StartTimer();
    HideAllOsds();

    return TRUE;
}

void Wh_ModUninit()
{
    Wh_Log(L"Hide Volume & Brightness OSD – Uninit");
    StopTimer();
    StopShowHook();
}

void Wh_ModSettingsChanged()
{
    LoadSettings();

    if (g_enabled)
    {
        StartShowHook();
        StartTimer();
        HideAllOsds();
    }
    else
    {
        StopTimer();
        // keep hook but it no-ops when disabled
    }
}
