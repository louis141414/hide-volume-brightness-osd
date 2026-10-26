// ==WindhawkMod==
// @id              hide-volume-brightness-osd
// @name            Hide Volume & Brightness OSD
// @description     Hides Windows volume/brightness/media OSDs using the proven Minimize method. No admin needed.
// @version         3.1
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

Uses the same method as HideVolumeOSD:
find the system OSD host window once, then keep it minimized.

Works on Windows 11 25H2 (build 26200) without admin rights.

### Support
**[https://ko-fi.com/nietnormalemeneer](https://ko-fi.com/nietnormalemeneer)**

Source: [https://github.com/louis141414/hide-volume-brightness-osd](https://github.com/louis141414/hide-volume-brightness-osd)
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- enabled: true
  $name: Enable mod
  $description: Keep the OSD hidden while this is on.
*/
// ==/WindhawkModSettings==

#include <windows.h>

// SW_MINIMIZE = 6, SW_RESTORE = 9
#ifndef SW_MINIMIZE
#define SW_MINIMIZE 6
#endif

bool g_enabled = true;
HWND g_osdHwnd = nullptr;
UINT_PTR g_timerId = 0;

// -------------------- Find (exact HideVolumeOSD logic) --------------------

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

        // Prefer a window that currently has non-zero size (gpkgpk improvement),
        // but still accept zero-size hosts so we can minimize them early.
        RECT rc{};
        GetWindowRect(found, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;

        if (pairCount == 0)
            result = found;

        // Prefer non-zero size if we see one
        if (w > 0 && h > 0)
            result = found;

        pairCount++;
    }

    return result;
}

HWND FindOsdWindow()
{
    // Windows 11 path (build >= 22000) – also used on 25H2 / 26200
    HWND hwnd = FindOsdByHierarchy(
        L"XamlExplorerHostIslandWindow",
        L"Windows.UI.Composition.DesktopWindowContentBridge",
        L"DesktopWindowXamlSource");

    if (hwnd)
        return hwnd;

    // Windows 10 / classic fallback
    hwnd = FindOsdByHierarchy(
        L"NativeHWNDHost",
        L"DirectUIHWND",
        nullptr);

    return hwnd;
}

// Force the OSD host to exist (same idea as HideVolumeOSD Init)
void TriggerOsdCreation()
{
    // Volume mute toggle tends to create the host without changing volume much
    keybd_event(VK_VOLUME_MUTE, 0, 0, 0);
    keybd_event(VK_VOLUME_MUTE, 0, KEYEVENTF_KEYUP, 0);
    Sleep(50);
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
        Wh_Log(L"OSD host not found – triggering creation");
        TriggerOsdCreation();
        Sleep(200);
        g_osdHwnd = FindOsdWindow();
    }

    if (g_osdHwnd)
        Wh_Log(L"OSD host found: %p", g_osdHwnd);
    else
        Wh_Log(L"OSD host still not found");
}

void KeepMinimized()
{
    if (!g_enabled)
        return;

    EnsureOsdHandle();

    if (g_osdHwnd && IsWindow(g_osdHwnd))
    {
        // This is the proven HideVolumeOSD method
        ShowWindow(g_osdHwnd, SW_MINIMIZE);
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
        g_timerId = SetTimer(nullptr, 0, 250, TimerProc); // low CPU, enough to re-minimize
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
}

BOOL Wh_ModInit()
{
    Wh_Log(L"Hide Volume & Brightness OSD v3.1 – Init (HideVolumeOSD method)");

    LoadSettings();

    EnsureOsdHandle();
    KeepMinimized();
    StartTimer();

    return TRUE;
}

void Wh_ModUninit()
{
    Wh_Log(L"Hide Volume & Brightness OSD – Uninit");
    StopTimer();

    // Restore so the OSD works again after unloading
    if (g_osdHwnd && IsWindow(g_osdHwnd))
    {
        ShowWindow(g_osdHwnd, SW_RESTORE);
        Wh_Log(L"Restored OSD host %p", g_osdHwnd);
    }
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
        if (g_osdHwnd && IsWindow(g_osdHwnd))
            ShowWindow(g_osdHwnd, SW_RESTORE);
    }
}
