# Hide Volume & Brightness OSD

**Windhawk mod** that completely hides the Windows volume, brightness and media on-screen displays (OSDs).

No replacement UI — the system popups simply never appear.

[![Version](https://img.shields.io/badge/version-1.9-blue)](https://github.com/louis141414/hide-volume-brightness-osd/releases)
[![Platform](https://img.shields.io/badge/platform-Windows%2010%20%7C%2011-0078D6)](https://github.com/louis141414/hide-volume-brightness-osd)
[![No Admin](https://img.shields.io/badge/admin-not%20required-success)](https://github.com/louis141414/hide-volume-brightness-osd)
[![Ko-fi](https://img.shields.io/badge/support-Ko--fi-ff5e5b)](https://ko-fi.com/nietnormalemeneer)

---

## Features

| Feature | Description |
|---------|-------------|
| **Volume OSD** | Hides the volume bar when you change volume |
| **Brightness OSD** | Hides the brightness indicator |
| **Media flyout** | Hides the larger media / now-playing panel |
| **Low overhead** | ~250 ms timer, minimal CPU |
| **No admin** | Works without elevated rights |
| **Clean restore** | OSD comes back when you disable or unload the mod |

---

## Compatibility

| OS | Status |
|----|--------|
| **Windows 11 25H2** (build 26200+) | Supported |
| Windows 11 24H2 / 23H2 / 22H2 | Supported |
| Windows 11 21H2 | Supported |
| Windows 10 (1809 and newer) | Supported |

Tested on **Windows 11 25H2 (26200.9711)** with Windhawk 1.7.x.

---

## How it works

Windows keeps a dedicated host window for the volume / brightness / media OSD:

- **Windows 11:** `XamlExplorerHostIslandWindow` → composition bridge → XAML source  
- **Windows 10:** `NativeHWNDHost` → `DirectUIHWND`

The mod finds that host and keeps it **minimized**, so the OSD never becomes visible. This is the same reliable approach used by the popular HideVolumeOSD tool, implemented as a pure Windhawk mod.

```
┌─────────────────────────────┐
│  Volume / Brightness key    │
└──────────────┬──────────────┘
               │
               ▼
┌─────────────────────────────┐
│  System creates / shows     │
│  OSD host window            │
└──────────────┬──────────────┘
               │
               ▼
┌─────────────────────────────┐
│  Mod keeps host minimized   │  ← you never see the popup
└─────────────────────────────┘
```

---

## Installation

1. Install **[Windhawk](https://windhawk.net/)** if you don’t have it yet.
2. Open Windhawk → **Create a new mod** (or open your existing local mod).
3. Paste the contents of [`hide-volume-brightness-osd.wh.cpp`](hide-volume-brightness-osd.wh.cpp).
4. Click **Compile mod**.
5. Enable the mod.
6. (Recommended) Restart Explorer once: Task Manager → Windows Explorer → Restart.

---

## Settings

| Setting | Default | Description |
|---------|---------|-------------|
| **Enable mod** | On | Keep the OSDs hidden while enabled |
| **Debug log** | Off | Extra diagnostic messages in the Windhawk log |

---

## Before / After

| Before | After |
|--------|-------|
| Volume bar appears on every volume change | No volume bar |
| Brightness indicator overlays the screen | No brightness indicator |
| Media flyout can cover content | No media flyout |

*Tip: press volume up/down after enabling — the popup should no longer appear.*

---

## Troubleshooting

- **OSD still shows**  
  Restart Explorer, then re-enable the mod. Make sure the mod is compiled as **v1.9**.

- **Need diagnostics**  
  Turn on **Debug log** in the mod settings and check the Windhawk / DebugView log for `OSD host ready`.

- **Want the OSD back**  
  Disable the mod or unload it — the host window is restored automatically.

---

## Support the developer

If this mod is useful to you, you can support development here:

**[https://ko-fi.com/nietnormalemeneer](https://ko-fi.com/nietnormalemeneer)**

---

## License

Free to use, modify and share. Source available in this repository.

---

## Links

- **Source:** [hide-volume-brightness-osd.wh.cpp](hide-volume-brightness-osd.wh.cpp)
- **Releases:** [GitHub Releases](https://github.com/louis141414/hide-volume-brightness-osd/releases)
- **Ko-fi:** [ko-fi.com/nietnormalemeneer](https://ko-fi.com/nietnormalemeneer)
- **Windhawk:** [windhawk.net](https://windhawk.net/)
