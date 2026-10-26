// ==WindhawkMod==
// @id              hide-volume-brightness-osd
// @name            Hide Volume & Brightness OSD
// @description     Hides Windows volume/brightness/media OSDs using the proven Minimize method. No admin needed.
// @version         3.2
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
find the system OSD host window, then keep it minimized.

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
- debugLog: true
  $name: Debug log
  $description: Log every scan and candidate window (turn off after it works).
*/
// ==/WindhawkModSettings==

#include <windows.h>

#ifndef SW_MINIMIZE
#define SW_MINIMIZE 6
#endif
#ifndef SW_RESTORE
#define SW_RESTORE 9
#endif

bool g_enabled = true;
bool g_debugLog = true;
HWND g_osdHwnd = nullptr;
UINT_PTR g_timerId = 0;
int g_scanCount = 0;

// -------------------- Find (HideVolumeOSD hierarchy) --------------------

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

        if (g_debugLog)
        {
            Wh_Log(L"[FIND] %s hwnd=%p size=%dx%d visible=%d",
                outerClass, found, w, h, IsWindowVisible(found) ? 1 : 0);
        }

        if (pairCount == 0)
            result = found;

        // Prefer non-zero size (gpkgpk v1.3.3)
        if (w > 0 && h > 0)
            result = found;

        pairCount++;
    }

    return result;
}

HWND FindOsdWindow()
{
    // Windows 11 path – also used on 25H2 / 26200
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

    // Windows 10 / classic
    hwnd = FindOsdByHierarchy(
        L"NativeHWNDHost",
        L"DirectUIHWND",
        nullptr);

    return hwnd;
}

// Dump every top-level Xaml / Native host so we can see what 25H2 uses
void DiagnosticDump()
{
    if (!g_debugLog)
        return;

    Wh_Log(L"--- DIAG dump start ---");

    HWND hwnd = nullptr;
    while ((hwnd = FindWindowExW(nullptr, hwnd, nullptr, nullptr)) != nullptr)
    {
        wchar_t cls[256]{};
        if (!GetClassNameW(hwnd, cls, 256))
            continue;

        if (wcsstr(cls, L"Xaml") == nullptr &&
            wcsstr(cls, L"NativeHWND") == nullptr &&
            wcsstr(cls, L"Island") == nullptr &&
            wcsstr(cls, L"Flyout") == nullptr &&
            wcsstr(cls, L"OSD") == nullptr)
            continue;

        RECT rc{};
        GetWindowRect(hwnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;

        // Log even zero-size so we know the host exists
        Wh_Log(L"[DIAG] class=%s hwnd=%p size=%dx%d vis=%d",
            cls, hwnd, w, h, IsWindowVisible(hwnd) ? 1 : 0);

        // Log first-level children for Xaml hosts
        if (wcsstr(cls, L"Xaml") || wcsstr(cls, L"NativeHWND"))
        {
            HWND child = nullptr;
            while ((child = FindWindowExW(hwnd, child, nullptr, nullptr)) != nullptr)
            {
                wchar_t ccls[256]{};
                GetClassNameW(child, ccls, 256);
                RECT crc{};
                GetWindowRect(child, &crc);
                Wh_Log(L"  child class=%s size=%dx%d",
                    ccls, crc.right - crc.left, crc.bottom - crc.top);
            }
        }
    }

    Wh_Log(L"--- DIAG dump end ---");
}

// Force the OSD host to exist (HideVolumeOSD Init idea)
void TriggerOsdCreation()
{
    Wh_Log(L"Triggering OSD creation via volume mute toggle");
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
        Wh_Log(L"OSD host not found – triggering creation");
        TriggerOsdCreation();
        Sleep(250);
        g_osdHwnd = FindOsdWindow();
    }

    if (g_osdHwnd)
        Wh_Log(L"OSD host found: %p", g_osdHwnd);
    else
    {
        Wh_Log(L"OSD host still not found – running diagnostic dump");
        DiagnosticDump();
    }
}

void KeepMinimized()
{
    if (!g_enabled)
        return;

    EnsureOsdHandle();

    if (g_osdHwnd && IsWindow(g_osdHwnd))
    {
        // Proven HideVolumeOSD method
        ShowWindow(g_osdHwnd, SW_MINIMIZE);

        if (g_debugLog && (g_scanCount % 20 == 0)) // every ~5s at 250ms
            Wh_Log(L"KeepMinimized on %p", g_osdHwnd);
    }

    g_scanCount++;
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

// -------------------- Settings / entry --------------------

void LoadSettings()
{
    g_enabled = Wh_GetIntSetting(L"enabled") != 0;
    g_debugLog = Wh_GetIntSetting(L"debugLog") != 0;
}

BOOL Wh_ModInit()
{
    Wh_Log(L"Hide Volume & Brightness OSD v3.2 – Init (HideVolumeOSD method + diag)");

    LoadSettings();

    // Full dump first so we see what exists before we touch anything
    DiagnosticDump();

    EnsureOsdHandle();
    KeepMinimized();
    StartTimer();

    return TRUE;
}

void Wh_ModUninit()
{
    Wh_Log(L"Hide Volume & Brightness OSD – Uninit");
    StopTimer();

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
