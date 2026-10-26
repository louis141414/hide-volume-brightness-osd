// ==WindhawkMod==
// @id              hide-volume-brightness-osd
// @name            Hide Volume & Brightness OSD
// @description     Hides the Windows volume, brightness and media OSDs. Lightweight, no admin required. Works on Windows 10 and Windows 11 (including 25H2).
// @version         1.9
// @author          NietNormaleMeneer
// @github          https://github.com/louis141414
// @homepage        https://ko-fi.com/nietnormalemeneer
// @include         explorer.exe
// @include         ShellHost.exe
// @include         ShellExperienceHost.exe
// @compilerOptions -luser32
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Hide Volume & Brightness OSD

A lightweight Windhawk mod that **completely hides** the Windows on-screen
displays (OSDs) for volume, brightness and media controls.

No replacement UI is shown — the system popups simply never appear.

---

### Features

- Hides **volume** OSD
- Hides **brightness** OSD
- Hides **media / now-playing** flyout
- Uses the proven **minimize host window** method (same approach as HideVolumeOSD)
- Extremely low CPU usage (~250 ms timer)
- **No administrator rights** required
- Instant enable / disable from Windhawk settings
- Restores the OSD when the mod is unloaded or disabled

---

### Compatibility

| OS | Status |
|----|--------|
| Windows 11 25H2 (build 26200+) | Supported |
| Windows 11 24H2 / 23H2 / 22H2 | Supported |
| Windows 11 21H2 | Supported |
| Windows 10 (1809 and newer) | Supported |

Tested on **Windows 11 25H2 (26200.9711)**.

---

### How it works

Windows keeps a special host window for the volume/brightness/media OSD:

- **Windows 11:** `XamlExplorerHostIslandWindow` → composition bridge → XAML source
- **Windows 10:** `NativeHWNDHost` → `DirectUIHWND`

This mod finds that host once and keeps it minimized so the OSD never becomes visible. When you disable or unload the mod, the host is restored.

---

### Installation

1. Install [Windhawk](https://windhawk.net/)
2. Create a new mod and paste the source of this mod
3. Click **Compile mod**
4. Enable the mod
5. (Recommended) Restart Explorer once

---

### Settings

| Setting | Default | Description |
|---------|---------|-------------|
| **Enable mod** | On | Keep the OSD hidden while enabled |
| **Debug log** | Off | Extra logging for troubleshooting |

---

### Support

If you find this useful, you can support development here:

**[https://ko-fi.com/nietnormalemeneer](https://ko-fi.com/nietnormalemeneer)**

Source code: [https://github.com/louis141414/hide-volume-brightness-osd](https://github.com/louis141414/hide-volume-brightness-osd)
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- enabled: true
  $name: Enable mod
  $description: Keep the volume, brightness and media OSDs hidden.
- debugLog: false
  $name: Debug log
  $description: Write extra diagnostic messages to the Windhawk log. Turn off for normal use.
*/
// ==/WindhawkModSettings==

#include <windows.h>

#ifndef SW_MINIMIZE
#define SW_MINIMIZE 6
#endif
#ifndef SW_RESTORE
#define SW_RESTORE 9
#endif

// -------------------- State --------------------

bool g_enabled = true;
bool g_debugLog = false;
HWND g_osdHwnd = nullptr;
UINT_PTR g_timerId = 0;

// -------------------- Logging --------------------

void Log(const wchar_t* fmt, ...)
{
    if (!g_debugLog)
        return;

    wchar_t buf[512];
    va_list args;
    va_start(args, fmt);
    vswprintf_s(buf, fmt, args);
    va_end(args);
    Wh_Log(L"%s", buf);
}

// -------------------- Detection --------------------

HWND FindOsdByHierarchy(const wchar_t* outerClass,
                        const wchar_t* innerClass,
                        const wchar_t* innerInnerClass)
{
    HWND found = nullptr;
    HWND result = nullptr;
    int pairCount = 0;

    while ((found = FindWindowExW(nullptr, found, outerClass, nullptr)) != nullptr)
    {
        HWND inner = FindWindowExW(found, nullptr, innerClass, nullptr);
        if (!inner)
            continue;

        if (innerInnerClass && innerInnerClass[0])
        {
            if (!FindWindowExW(inner, nullptr, innerInnerClass, nullptr))
                continue;
        }

        RECT rc{};
        GetWindowRect(found, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;

        Log(L"[FIND] %s hwnd=%p size=%dx%d visible=%d",
            outerClass, found, w, h, IsWindowVisible(found) ? 1 : 0);

        if (pairCount == 0)
            result = found;

        // Prefer a host that currently has non-zero size
        if (w > 0 && h > 0)
            result = found;

        pairCount++;
    }

    return result;
}

HWND FindOsdWindow()
{
    // Windows 11 (including 25H2)
    HWND hwnd = FindOsdByHierarchy(
        L"XamlExplorerHostIslandWindow",
        L"Windows.UI.Composition.DesktopWindowContentBridge",
        L"DesktopWindowXamlSource");
    if (hwnd)
        return hwnd;

    // Fallback without requiring DesktopWindowXamlSource
    hwnd = FindOsdByHierarchy(
        L"XamlExplorerHostIslandWindow",
        L"Windows.UI.Composition.DesktopWindowContentBridge",
        nullptr);
    if (hwnd)
        return hwnd;

    // Windows 10 classic OSD
    hwnd = FindOsdByHierarchy(
        L"NativeHWNDHost",
        L"DirectUIHWND",
        nullptr);

    return hwnd;
}

// -------------------- Host lifetime --------------------

void TriggerOsdCreation()
{
    Log(L"Triggering OSD host creation");
    keybd_event(VK_VOLUME_MUTE, 0, 0, 0);
    keybd_event(VK_VOLUME_MUTE, 0, KEYEVENTF_KEYUP, 0);
    Sleep(80);
    keybd_event(VK_VOLUME_MUTE, 0, 0, 0);
    keybd_event(VK_VOLUME_MUTE, 0, KEYEVENTF_KEYUP, 0);
}

void EnsureOsdHandle()
{
    if (g_osdHwnd && IsWindow(g_osdHwnd))
        return;

    g_osdHwnd = FindOsdWindow();

    if (!g_osdHwnd)
    {
        Log(L"OSD host not found – creating it");
        TriggerOsdCreation();
        Sleep(250);
        g_osdHwnd = FindOsdWindow();
    }

    if (g_osdHwnd)
        Wh_Log(L"OSD host ready: %p", g_osdHwnd);
    else
        Wh_Log(L"OSD host not found yet (will retry)");
}

void KeepMinimized()
{
    if (!g_enabled)
        return;

    EnsureOsdHandle();

    if (g_osdHwnd && IsWindow(g_osdHwnd))
        ShowWindow(g_osdHwnd, SW_MINIMIZE);
}

void RestoreOsd()
{
    if (g_osdHwnd && IsWindow(g_osdHwnd))
    {
        ShowWindow(g_osdHwnd, SW_RESTORE);
        Wh_Log(L"OSD host restored: %p", g_osdHwnd);
    }
}

// -------------------- Timer --------------------

void CALLBACK TimerProc(HWND, UINT, UINT_PTR, DWORD)
{
    KeepMinimized();
}

void StartTimer()
{
    if (g_timerId)
    {
        KillTimer(nullptr, g_timerId);
        g_timerId = 0;
    }
    if (g_enabled)
        g_timerId = SetTimer(nullptr, 0, 250, TimerProc);
}

void StopTimer()
{
    if (g_timerId)
    {
        KillTimer(nullptr, g_timerId);
        g_timerId = 0;
    }
}

// -------------------- Settings --------------------

void LoadSettings()
{
    g_enabled = Wh_GetIntSetting(L"enabled") != 0;
    g_debugLog = Wh_GetIntSetting(L"debugLog") != 0;
}

// -------------------- Entry points --------------------

BOOL Wh_ModInit()
{
    Wh_Log(L"Hide Volume & Brightness OSD v1.9 – Init");

    LoadSettings();
    EnsureOsdHandle();
    KeepMinimized();
    StartTimer();

    return TRUE;
}

void Wh_ModUninit()
{
    Wh_Log(L"Hide Volume & Brightness OSD v1.9 – Uninit");
    StopTimer();
    RestoreOsd();
}

void Wh_ModSettingsChanged()
{
    LoadSettings();

    if (g_enabled)
    {
        EnsureOsdHandle();
        KeepMinimized();
        StartTimer();
    }
    else
    {
        StopTimer();
        RestoreOsd();
    }
}
