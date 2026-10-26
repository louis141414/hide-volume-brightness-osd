// ==WindhawkMod==
// @id              hide-volume-brightness-osd
// @name            Hide Volume & Brightness OSD
// @description     Selectively hides Windows volume, brightness, media and other OSDs. Fast, lightweight and fully configurable.
// @version         1.3.2
// @author          NietNormaleMeneer
// @github          https://github.com/louis141414
// @homepage        https://ko-fi.com/nietnormalemeneer
// @include         explorer.exe
// @compilerOptions -luser32
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Hide Volume & Brightness OSD

A fast and lightweight Windhawk mod that **selectively hides** the Windows on-screen displays (OSDs) for volume, brightness, media controls and more.

Perfect if you want a cleaner desktop without the floating bars that appear when you change volume or brightness.

### Features
- Hide **Volume OSD**
- Hide **Brightness OSD**
- Hide **Media / Now Playing** flyout
- Optionally hide other system OSDs
- Extremely low CPU usage
- Instant reaction (configurable poll interval)
- Works on Windows 10 and Windows 11

### Settings
Just enable the specific OSDs you want to hide. Everything else stays normal.

### Support the developer
If you find this mod useful, consider buying me a coffee:

**[https://ko-fi.com/nietnormalemeneer](https://ko-fi.com/nietnormalemeneer)**

Source code: [https://github.com/louis141414/hide-volume-brightness-osd](https://github.com/louis141414/hide-volume-brightness-osd)
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- hideVolume: true
  $name: Hide Volume OSD
  $description: Hides the volume bar / slider that appears when changing volume with keyboard or other methods.
- hideBrightness: true
  $name: Hide Brightness OSD
  $description: Hides the brightness indicator that appears when changing screen brightness.
- hideMedia: true
  $name: Hide Media / Now Playing flyout
  $description: Hides the larger media controls / now-playing OSD that sometimes appears together with volume changes.
- hideOther: false
  $name: Hide other system OSDs
  $description: Also tries to hide less common OSDs (e.g. airplane mode).
- pollIntervalMs: 120
  $name: Check interval (ms)
  $description: How often the mod looks for OSD windows. Lower = faster reaction, slightly higher CPU. Recommended: 80-150.
*/
// ==/WindhawkModSettings==

#include <windows.h>

// -------------------- Settings --------------------
bool g_hideVolume     = true;
bool g_hideBrightness = true;
bool g_hideMedia      = true;
bool g_hideOther      = false;
int  g_pollIntervalMs = 120;

// -------------------- State --------------------
UINT_PTR g_timerId = 0;

// -------------------- Helpers --------------------
inline bool IsValidOsdSize(int w, int h)
{
    // Typical OSD sizes – filters out unrelated windows
    return (w > 40 && w < 900 && h > 20 && h < 480);
}

// Classic NativeHWNDHost + DirectUIHWND (Windows 10 style + many Win11 builds)
HWND FindClassicOsd()
{
    HWND hwnd = nullptr;
    while ((hwnd = FindWindowExW(nullptr, hwnd, L"NativeHWNDHost", nullptr)) != nullptr)
    {
        if (FindWindowExW(hwnd, nullptr, L"DirectUIHWND", nullptr))
        {
            RECT rc{};
            if (GetWindowRect(hwnd, &rc))
            {
                int w = rc.right - rc.left;
                int h = rc.bottom - rc.top;
                if (IsValidOsdSize(w, h))
                    return hwnd;
            }
        }
    }
    return nullptr;
}

// Modern Windows 11 XAML-based OSD
HWND FindModernOsd()
{
    HWND hwnd = nullptr;
    while ((hwnd = FindWindowExW(nullptr, hwnd, L"XamlExplorerHostIslandWindow", nullptr)) != nullptr)
    {
        HWND bridge = FindWindowExW(hwnd, nullptr,
            L"Windows.UI.Composition.DesktopWindowContentBridge", nullptr);
        if (!bridge)
            continue;

        if (!FindWindowExW(bridge, nullptr, L"DesktopWindowXamlSource", nullptr))
            continue;

        RECT rc{};
        if (!GetWindowRect(hwnd, &rc))
            continue;

        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;

        if (IsValidOsdSize(w, h))
            return hwnd;
    }
    return nullptr;
}

// Decide whether this OSD should be hidden based on current settings
bool ShouldHide(HWND hwnd)
{
    if (!hwnd || !IsWindow(hwnd) || !IsWindowVisible(hwnd))
        return false;

    RECT rc{};
    if (!GetWindowRect(hwnd, &rc))
        return false;

    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    // Media / Now Playing flyouts are usually larger
    bool isMediaLike = (w > 260 || h > 110);

    if (isMediaLike)
        return g_hideMedia;

    // Compact bars = volume or brightness
    // (Windows does not expose a reliable way to distinguish them without heavy hooks)
    if (g_hideVolume || g_hideBrightness)
        return true;

    return g_hideOther;
}

void HideMatchingOsds()
{
    // Classic
    HWND classic = FindClassicOsd();
    if (classic && ShouldHide(classic))
        ShowWindow(classic, SW_HIDE);

    // Modern Win11
    HWND modern = FindModernOsd();
    if (modern && ShouldHide(modern))
        ShowWindow(modern, SW_HIDE);
}

// -------------------- Timer --------------------
void CALLBACK TimerProc(HWND, UINT, UINT_PTR, DWORD)
{
    HideMatchingOsds();
}

void StartTimer()
{
    if (g_timerId)
    {
        KillTimer(nullptr, g_timerId);
        g_timerId = 0;
    }

    // Only run when at least one type is enabled
    if (g_hideVolume || g_hideBrightness || g_hideMedia || g_hideOther)
    {
        g_timerId = SetTimer(nullptr, 0, (UINT)g_pollIntervalMs, TimerProc);
    }
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
    g_hideVolume     = Wh_GetIntSetting(L"hideVolume") != 0;
    g_hideBrightness = Wh_GetIntSetting(L"hideBrightness") != 0;
    g_hideMedia      = Wh_GetIntSetting(L"hideMedia") != 0;
    g_hideOther      = Wh_GetIntSetting(L"hideOther") != 0;
    g_pollIntervalMs = Wh_GetIntSetting(L"pollIntervalMs");

    if (g_pollIntervalMs < 40)  g_pollIntervalMs = 40;
    if (g_pollIntervalMs > 500) g_pollIntervalMs = 500;
}

// -------------------- Windhawk entry points --------------------
BOOL Wh_ModInit()
{
    Wh_Log(L"Hide Volume & Brightness OSD v1.3.2 – Init");

    LoadSettings();
    StartTimer();
    HideMatchingOsds();   // hide any currently visible OSDs immediately

    return TRUE;
}

void Wh_ModUninit()
{
    Wh_Log(L"Hide Volume & Brightness OSD – Uninit");
    StopTimer();
}

void Wh_ModSettingsChanged()
{
    Wh_Log(L"Settings changed");
    LoadSettings();
    StartTimer();
}
