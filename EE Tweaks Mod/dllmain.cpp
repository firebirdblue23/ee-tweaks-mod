#pragma comment( lib, "psapi.lib" ) // Keep this here or move to project settings
#include "pch.h"
#include "globals.h"
#include "logging.h"
#include "config.h"
#include "memory.h"
#include "patches.h"

// --- Helper Functions --- (Moved to respective files)

// --- Main Mod Logic ---
void ApplyTweaks() {
    // 0. Get DLL directory and Executable Name/Path FIRST
    char dllPath[MAX_PATH] = { 0 };
    char exePath[MAX_PATH] = { 0 };

    if (g_hModule == NULL ||
        GetModuleFileNameA(g_hModule, dllPath, MAX_PATH) == 0) {
        OutputDebugStringA(
            ("tweaks.dll: FATAL ERROR - Could not get DLL module path. Error: " +
                std::to_string(GetLastError()) +
                ". Using fallback directory '.'\n")
            .c_str());
        g_dllDir = ".";
    }
    else {
        try {
            std::filesystem::path fsDllPath(dllPath);
            g_dllDir = fsDllPath.has_parent_path() ?
                fsDllPath.parent_path().string() :
                ".";
        }
        catch (const std::exception& e) {
            OutputDebugStringA(
                ("tweaks.dll: Exception getting DLL directory: " +
                    std::string(e.what()) + ". Using fallback directory '.'\n")
                .c_str());
            g_dllDir = ".";
        }
    }

    if (GetModuleFileNameA(NULL, exePath, MAX_PATH) == 0) {
        OutputDebugStringA(
            ("tweaks.dll: ERROR - Could not get executable path. Error: " +
                std::to_string(GetLastError()) + "\n")
            .c_str());
        g_executablePath = "UNKNOWN_EXE_PATH";
        g_executableName = "UNKNOWN_EXE";
    }
    else {
        g_executablePath = exePath;
        try {
            std::filesystem::path fsExePath(g_executablePath);
            g_executableName = fsExePath.has_filename() ?
                fsExePath.filename().string() :
                "UNKNOWN_EXE";
        }
        catch (const std::exception& e) {
            OutputDebugStringA(
                ("tweaks.dll: Exception getting executable filename: " +
                    std::string(e.what()) + "\n")
                .c_str());
            g_executableName = "UNKNOWN_EXE";
        }
    }

    // 1. Load Configuration (determines if logging is enabled)
    LoadConfig(); // Reads EnableLogging flag into g_loggingEnabled

    // 2. Initialize Logging (opens file if enabled)
    InitializeLogging(); // Must be called after LoadConfig and path detection

    // 3. Initial Log Messages
    Log("--------------------");
    Log("Empire Earth Tweaks " + std::string(MOD_VERSION) + " loaded.");
    Log("Mod by: " + std::string(MOD_AUTHOR));
    Log("DLL Directory: " + g_dllDir);
    Log("Game Executable Path: " + g_executablePath);
    Log("Game Executable Name: " + g_executableName);
    // Logging status is logged within InitializeLogging()

    if (g_executableName != "EE-AOC.exe" &&
        g_executableName != "Empire Earth.exe" &&
        g_executableName != "UNKNOWN_EXE") {
        Log("Warning: Detected executable '" + g_executableName +
            "' is not recognized. Patches targeting the main executable might "
            "fail.");
    }
    else if (g_executableName == "UNKNOWN_EXE") {
        Log("Warning: Could not determine executable name. Patches targeting "
            "the main executable might fail.");
    }

    // 4. Update standard patch enabled status from config
    for (auto& patch : g_patches) {
        patch.enabled = GetConfigBool(patch.name + "Enabled", false);
        if (!patch.enabled && patch.name != "BypassMapSizeAssertion") {
            Log("Config: Patch '" + patch.name + "' disabled.");
        }
    }

    // Force enable the assertion bypass patch
    bool assertionPatchFound = false;
    for (auto& patch : g_patches) {
        if (patch.name == "BypassMapSizeAssertion") {
            if (!patch.enabled) {
                Log("Info: Forcing 'BypassMapSizeAssertion' patch to enabled state.");
                patch.enabled = true;
            }
            assertionPatchFound = true;
            break;
        }
    }
    if (!assertionPatchFound) {
        Log("Error: 'BypassMapSizeAssertion' patch definition not found internally.");
    }

    // 5. Find Standard Patch Locations
    Log("Scanning for standard patch locations...");
    for (auto& patch : g_patches) {
        if (!patch.enabled) {
            continue;
        }

        std::string moduleToScan = patch.moduleIdentifier;
        if (moduleToScan == "GAME_EXECUTABLE") {
            if (g_executableName == "UNKNOWN_EXE") {
                Log("Skipping scan for patch '" + patch.name +
                    "' because game executable name is unknown.");
                continue;
            }
            moduleToScan = g_executableName;
        }

        MODULEINFO moduleInfo = { 0 };
        uintptr_t baseAddress = 0;
        size_t moduleSize = 0;
        if (!GetModuleInfoByName(moduleToScan, moduleInfo, baseAddress,
            moduleSize)) {
            Log("Warning: Could not get module info for '" + moduleToScan +
                "' needed by patch '" + patch.name +
                "'. Patch will be skipped.");
            continue;
        }

        std::vector<unsigned char> originalBytes;
        if (!HexToBytes(patch.originalHex, originalBytes)) {
            Log("Error: Cannot scan for patch '" + patch.name +
                "' due to invalid original hex pattern. Skipping patch.");
            continue;
        }

        patch.patchAddress =
            FindPattern(baseAddress, moduleSize, originalBytes);

        if (patch.patchAddress != 0) {
            std::stringstream ss;
            ss << "0x" << std::hex << patch.patchAddress;
            Log("Found pattern for patch '" + patch.name + "' in module '" +
                moduleToScan + "' at address " + ss.str());
        }
        else {
            Log("Warning: Pattern for patch '" + patch.name + "' not found in '" +
                moduleToScan + "'. Patch will be skipped.");
        }
    }

    // 6. Apply Standard Patches
    Log("Applying enabled standard patches...");
    int patchesApplied = 0;
    for (auto& patch : g_patches) {
        if (ApplyStandardPatch(patch)) { // ApplyStandardPatch now uses ApplyDataPatch
            patchesApplied++;
        }
    }
    Log("Applied " + std::to_string(patchesApplied) + " standard patches.");

    // 7. Apply Custom Patches
    Log("Applying custom patches...");
    int customPatchesApplied = 0;

    if (ApplyAudioSampleRatePatch()) {
        customPatchesApplied++;
    }

    if (g_executableName != "UNKNOWN_EXE") {
        if (ApplyCustomFlatWorldSizesPatch()) {
            customPatchesApplied++;
        }
        if (ApplyGiganticMapSizePatch()) {
            customPatchesApplied++;
        }
        if (ApplyMapGridLimitPatch()) { // Depends on GiganticMapSize
            customPatchesApplied++;
        }
        // *** APPLY NEW PATCH HERE ***
        if (ApplyChunkDimensionPatch()) { // Depends on Gigantic/Custom sizes > 220
            customPatchesApplied++;
        }
    }
    else {
        Log("Skipping game executable specific patches (FlatWorldSizes, "
            "GiganticMapSize, MapGridLimit, ChunkDimension) because game "
            "executable name is unknown.");
    }

    Log("Applied " + std::to_string(customPatchesApplied) + " custom patches.");

    Log("Patching process finished.");
    Log("--------------------");

    // 8. Close Log File (handled by ShutdownLogging)
    // ShutdownLogging(); // Call this in DLL_PROCESS_DETACH instead
}

// --- DLL Entry Point ---
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call,
    LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        g_hModule = hModule; // Store module handle *early*
        DisableThreadLibraryCalls(hModule);
        ApplyTweaks(); // Run main logic
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break; // Do nothing
    case DLL_PROCESS_DETACH:
        OutputDebugStringA("tweaks.dll: Unloading.\n");
        ShutdownLogging(); // Close the log file properly on unload
        break;
    }
    return TRUE;
}
