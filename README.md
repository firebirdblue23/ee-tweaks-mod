# Empire Earth Tweaks Mod

This mod provides several game improvements and customization options for Empire Earth and Empire Earth: The Art of Conquest.

**Important:** It is highly recommended to patch your game executable (`Empire Earth.exe` or `EE-AOC.exe`) with the [4GB Patch](https://ntcore.com/4gb-patch/) before using this mod, especially if you plan to use features like very large maps (e.g., above 1000x1000).

## Features

### Core Enhancements

*   **Extended Gigantic Map Size:**
    *   Set a custom dimension for the "Gigantic" map size via the `GiganticMapSize` setting in `tweaks.config` (e.g., `GiganticMapSize=512` for a 512x512 map).
    *   Enable/disable with `GiganticMapSizeEnabled`.

*   **Custom Flat Map Sizes:**
    *   Define your own list of up to 25 available map sizes for the "Flat" random map type.
    *   Configure via the `CustomFlatWorldSizes` setting in `tweaks.config` (e.g., `CustomFlatWorldSizes=100,150,200,250,300`).

*   **Custom Audio Sample Rate:**
    *   Change the audio sample rate used by the game's Miles Sound System for higher quality audio playback.
    *   Set the desired rate (e.g., `44100` Hz or `48000` Hz) using the `AudioSampleRate` setting in `tweaks.config`. The game default is `22050` Hz.

*   **Logging:**
    *   Generates a `tweaks_log.txt` file in the game directory detailing which patches were applied or skipped. Useful for troubleshooting.
    *   Enable/disable with `EnableLogging`.

### Performance & Stability Fixes

These features are based on feedback and investigation within the [DDrawCompat GitHub repository (Issue #251)](https://github.com/narzoul/DDrawCompat/issues/251).

*   **SetSleepToZero:**
    *   Sets one `Sleep()` call to `0` ms within the game's main loop. This can potentially reduce CPU usage and improve performance/reduce stuttering on some systems.
    *   Enable/disable with `SetSleepToZeroEnabled`.

*   **Increased DirectX 7 Memory Buffers:**
    *   Increases internal memory buffer allocations for both the standard DX7 and the DX7 TnL renderers.
    *   This may improve stability or performance, especially at higher resolutions or detail levels.
    *   Enable/disable separately for standard DX7 (`IncreaseMemoryBufferDX7Enabled`) and DX7 TnL (`IncreaseMemoryBufferDX7TnLEnabled`).

*   **DirectX 7 TnL Vertex Buffer Fix:**
    *   Forces the DX7 Hardware TnL renderer to use system memory (`SYSTEMMEM`) for vertex buffers instead of video memory (`VIDEOMEMORY`). This can resolve issues or improve compatibility on certain hardware/driver combinations.
    *   Enable/disable with `VertexBufferSystemMemEnabled`.

## Installation

1.  Download the latest `tweaks.dll` from the [Releases page](https://github.com/firebirdblue23/ee-tweaks-mod/releases) of this repository.
2.  Place the `tweaks.dll` file into the main directory of your Empire Earth installation (the same folder that contains `EE-AOC.exe` or `Empire Earth.exe`).
3.  Place the `tweaks.config` file into the same directory.
    *   If you don't have a `tweaks.config` file, running the game once with `tweaks.dll` present should generate a default configuration file for you.
4.  Launch the game as usual. The tweaks will be applied automatically based on the configuration.

## Configuration

*   Open `tweaks.config` with any standard text editor (like Notepad).
*   Enable or disable features by setting the corresponding `...Enabled` option to `true` or `false`.
*   Adjust values for features like `GiganticMapSize`, `CustomFlatWorldSizes`, and `AudioSampleRate` as desired, following the examples in the file.
*   Lines starting with `;` or `#` are treated as comments and are ignored by the mod.

## Compatibility

*   Tested primarily with **Empire Earth: The Art of Conquest** (`EE-AOC.exe`) on Windows.
*   Tested with [Empire Earth: Vanilla](https://empireearth.eu/download/) and **dreXmod**.
*   It *should* work with the base **Empire Earth** (`Empire Earth.exe`) as many memory patterns are similar, but this is not guaranteed or extensively tested.
*   Compatibility with other game versions, releases, or mods is not guaranteed.

## Disclaimer

Use this mod at your own risk. While developed with care (with assistance from AI tools for pattern finding and code generation), unexpected issues or conflicts could potentially arise. The author(s) are not responsible for any damage or problems caused by using this mod.
