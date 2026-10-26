# Changelog

## [1.9] – 2026-10-26

### Stable release

- Production-ready version based on the proven **minimize host window** method (same idea as HideVolumeOSD)
- Works on **Windows 11 25H2** (build 26200) without administrator rights
- Also supports Windows 11 21H2–24H2 and Windows 10 (1809+)
- Loads into `explorer.exe`, `ShellExperienceHost.exe` and `ShellHost.exe`
- Settings:
  - **Enable mod** – turn hiding on/off
  - **Debug log** – optional diagnostics (off by default)
- Restores the OSD when the mod is disabled or unloaded
- Low CPU usage (~250 ms timer)
- Full README with compatibility table, install steps and troubleshooting

### Credits

Detection hierarchy inspired by [gpkgpk/HideVolumeOSD](https://github.com/gpkgpk/HideVolumeOSD).
