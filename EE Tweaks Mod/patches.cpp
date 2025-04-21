#include "pch.h"
#include "globals.h" // Access constants and g_executableName
#include "logging.h" // Access Log()
#include "config.h"  // Access config functions
#include "memory.h"  // Access memory utilities
#include "patches.h"

// Helper to get module info
bool GetModuleInfoByName(const std::string& moduleName, MODULEINFO& moduleInfo,
    uintptr_t& baseAddress, size_t& moduleSize) {
    HMODULE hModule = GetModuleHandleA(moduleName.c_str());
    if (!hModule) {
        Log("Error: Could not get handle for module '" + moduleName + "'.");
        return false;
    }

    if (!GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo,
        sizeof(moduleInfo))) {
        Log("Error: Could not get module information for '" + moduleName +
            "'. Error code: " + std::to_string(GetLastError()));
        return false;
    }

    baseAddress = (uintptr_t)moduleInfo.lpBaseOfDll;
    moduleSize = moduleInfo.SizeOfImage;
    return true;
}

// Apply the custom flat world sizes patch
bool ApplyCustomFlatWorldSizesPatch() {
    Log("Checking Custom Flat World Sizes patch...");
    if (!GetConfigBool("CustomFlatWorldSizesEnabled", true)) {
        Log("Custom Flat World Sizes patch is disabled in config.");
        return false;
    }

    std::string sizesStr =
        GetConfigString("CustomFlatWorldSizes", DEFAULT_FLAT_MAP_SIZES_STR);
    Log("Custom sizes string from config: \"" + sizesStr + "\"");

    std::vector<int> newSizes;
    if (!ParseIntList(sizesStr, newSizes)) {
        Log("Error: Failed to parse CustomFlatWorldSizes list. Patch aborted.");
        return false;
    }

    if (newSizes.size() != NUM_FLAT_MAP_SIZES) {
        Log("Error: CustomFlatWorldSizes list must contain exactly " +
            std::to_string(NUM_FLAT_MAP_SIZES) + " values. Found " +
            std::to_string(newSizes.size()) + ". Patch aborted.");
        return false;
    }

    std::vector<unsigned char> originalBytes;
    std::vector<unsigned char> targetBytes;
    if (!HexToBytes(ORIGINAL_FLAT_MAP_SIZES_HEX, originalBytes)) {
        Log("Error: Failed to parse internal original flat map sizes hex "
            "pattern. Patch aborted.");
        return false;
    }
    IntsToBytesLE(newSizes, targetBytes);

    MODULEINFO moduleInfo = { 0 };
    uintptr_t baseAddress = 0;
    size_t moduleSize = 0;
    if (!GetModuleInfoByName(g_executableName, moduleInfo, baseAddress,
        moduleSize)) {
        Log("Aborting Custom Flat World Sizes patch.");
        return false;
    }

    uintptr_t patchAddress =
        FindPattern(baseAddress, moduleSize, originalBytes);
    if (patchAddress == 0) {
        Log("Warning: Original flat map size pattern not found in '" +
            g_executableName +
            "'. Custom Flat World Sizes patch cannot be applied.");
        return false;
    }

    // Apply as data patch (not executable code)
    if (ApplyDataPatch("CustomFlatWorldSizes", patchAddress, originalBytes,
        targetBytes, false)) {
        Log("Successfully applied Custom Flat World Sizes patch.");
        return true;
    }
    else {
        Log("Failed to apply Custom Flat World Sizes patch.");
        return false;
    }
}

// Apply the custom gigantic map size patch
bool ApplyGiganticMapSizePatch() {
    Log("Checking Gigantic Map Size patch...");
    if (!GetConfigBool("GiganticMapSizeEnabled", true)) {
        Log("Gigantic Map Size patch is disabled in config.");
        return false;
    }

    int newSize = GetConfigInt("GiganticMapSize", 220);
    if (newSize <= 0) {
        Log("Warning: Invalid GiganticMapSize (" + std::to_string(newSize) +
            "). Using default 220.");
        newSize = 220;
    }
    Log("Gigantic Map Size from config: " + std::to_string(newSize));

    // Original: EB 27 B8 DC 00 00 00 EB (DC 00 00 00 = 220 LE)
    std::vector<unsigned char> originalBytes = { 0xEB, 0x27, 0xB8, 0xDC,
                                                0x00, 0x00, 0x00, 0xEB };
    std::vector<unsigned char> newSizeBytes;
    IntToBytesLE(newSize, newSizeBytes); // Get the 4 bytes for the new size

    // Target: EB 27 B8 [New Size Bytes] EB
    std::vector<unsigned char> targetBytes = { 0xEB, 0x27, 0xB8, newSizeBytes[0],
                                              newSizeBytes[1], newSizeBytes[2],
                                              newSizeBytes[3], 0xEB };

    MODULEINFO moduleInfo = { 0 };
    uintptr_t baseAddress = 0;
    size_t moduleSize = 0;
    if (!GetModuleInfoByName(g_executableName, moduleInfo, baseAddress,
        moduleSize)) {
        Log("Aborting Gigantic Map Size patch.");
        return false;
    }

    uintptr_t patchAddress =
        FindPattern(baseAddress, moduleSize, originalBytes);
    if (patchAddress == 0) {
        Log("Warning: Original Gigantic map size pattern not found in '" +
            g_executableName +
            "'. Gigantic Map Size patch cannot be applied.");
        return false;
    }

    // Apply as code patch (is executable code)
    if (ApplyDataPatch("GiganticMapSize", patchAddress, originalBytes,
        targetBytes, true)) {
        Log("Successfully applied Gigantic Map Size patch (Size: " +
            std::to_string(newSize) + ").");
        return true;
    }
    else {
        Log("Failed to apply Gigantic Map Size patch.");
        return false;
    }
}

// Apply the patch to fix the internal map grid calculation limit
bool ApplyMapGridLimitPatch() {
    Log("Checking Map Grid Limit patch...");

    // This patch is implicitly tied to GiganticMapSizeEnabled
    if (!GetConfigBool("GiganticMapSizeEnabled", true)) {
        Log("Map Grid Limit patch skipped (GiganticMapSizeEnabled is false).");
        return false;
    }

    int giganticMapSize = GetConfigInt("GiganticMapSize", 220);
    if (giganticMapSize <= 0) giganticMapSize = 220; // Use default if invalid

    Log("Gigantic Map Size for Grid Limit check: " +
        std::to_string(giganticMapSize));

    // Original HEX pattern
    std::string originalHex = "72 09 b9 00 04 00 00 3b c1"; // Original 1024 limit
    std::string targetHex;
    std::string patchDescription;

    if (giganticMapSize <= 511) {
        Log("GiganticMapSize (" + std::to_string(giganticMapSize) +
            ") is within the original limit (<= 511). No Grid Limit patch "
            "needed.");
        return false; // Indicate no patch was applied, not an error
    }
    else if (giganticMapSize <= 1023) {
        targetHex = "72 09 b9 00 08 00 00 3b c1"; // 2048 limit
        patchDescription = "1023x1023 limit";
    }
    else if (giganticMapSize <= 2047) {
        targetHex = "72 09 b9 00 10 00 00 3b c1"; // 4096 limit
        patchDescription = "2047x2047 limit";
    }
    else if (giganticMapSize <= 4095) {
        targetHex = "72 09 b9 00 20 00 00 3b c1"; // 8192 limit
        patchDescription = "4095x4095 limit";
    }
    else {
        targetHex = "72 09 b9 00 20 00 00 3b c1"; // 8192 limit (max)
        patchDescription = "4095x4095 limit (max supported)";
        Log("Warning: GiganticMapSize (" + std::to_string(giganticMapSize) +
            ") exceeds the maximum tested limit (4095). Applying the "
            "4095x4095 patch, but stability is not guaranteed.");
    }
    Log("Applying Map Grid Limit patch for " + patchDescription + ".");

    std::vector<unsigned char> originalBytes;
    std::vector<unsigned char> targetBytes;
    if (!HexToBytes(originalHex, originalBytes) ||
        !HexToBytes(targetHex, targetBytes)) {
        Log("Error: Failed to parse hex strings for Map Grid Limit patch (" +
            patchDescription + "). Patch aborted.");
        return false;
    }

    MODULEINFO moduleInfo = { 0 };
    uintptr_t baseAddress = 0;
    size_t moduleSize = 0;
    if (!GetModuleInfoByName(g_executableName, moduleInfo, baseAddress,
        moduleSize)) {
        Log("Aborting Map Grid Limit patch.");
        return false;
    }

    uintptr_t patchAddress =
        FindPattern(baseAddress, moduleSize, originalBytes);
    if (patchAddress == 0) {
        Log("Warning: Original Map Grid Limit pattern not found in '" +
            g_executableName +
            "'. Map Grid Limit patch cannot be applied. Maybe the game version "
            "is different or already patched?");
        return false;
    }

    // Apply as code patch (is executable code)
    if (ApplyDataPatch("MapGridLimit", patchAddress, originalBytes, targetBytes,
        true)) {
        Log("Successfully applied Map Grid Limit patch (" + patchDescription +
            ").");
        return true;
    }
    else {
        Log("Failed to apply Map Grid Limit patch.");
        return false;
    }
}

// Apply the custom audio sample rate patch
bool ApplyAudioSampleRatePatch() {
    Log("Checking Audio Sample Rate patch...");

    int sampleRate = GetConfigInt("AudioSampleRate", 44100);
    if (sampleRate <= 0 || sampleRate > 65535) { // Max for 2 bytes
        Log("Warning: Invalid AudioSampleRate (" + std::to_string(sampleRate) +
            "). Using default 44100.");
        sampleRate = 44100;
    }
    Log("Audio Sample Rate from config: " + std::to_string(sampleRate));

    // Original: FF D6 6A 01 6A 02 6A 10 68 22 56 (22050 = 0x5622 LE)
    std::vector<unsigned char> originalBytes = {
        0xFF, 0xD6, 0x6A, 0x01, 0x6A, 0x02, 0x6A, 0x10, 0x68, 0x22, 0x56 };

    // Target: FF D6 6A 01 6A 02 6A 10 68 [RateLowByte] [RateHighByte]
    unsigned char rateLowByte = static_cast<unsigned char>(sampleRate & 0xFF);
    unsigned char rateHighByte = static_cast<unsigned char>((sampleRate >> 8) & 0xFF);
    std::vector<unsigned char> targetBytes = {
        0xFF, 0xD6, 0x6A, 0x01, 0x6A, 0x02, 0x6A, 0x10, 0x68,
        rateLowByte, rateHighByte };

    const char* moduleName = "Miles Sound System Mixer.dll";
    MODULEINFO moduleInfo = { 0 };
    uintptr_t baseAddress = 0;
    size_t moduleSize = 0;
    if (!GetModuleInfoByName(moduleName, moduleInfo, baseAddress, moduleSize)) {
        Log("Warning: Could not find module '" + std::string(moduleName) +
            "'. Audio Sample Rate patch cannot be applied. Is Miles audio "
            "being used?");
        return false;
    }

    uintptr_t patchAddress =
        FindPattern(baseAddress, moduleSize, originalBytes);
    if (patchAddress == 0) {
        Log("Warning: Original audio sample rate pattern not found in '" +
            std::string(moduleName) +
            "'. Audio Sample Rate patch cannot be applied.");
        return false;
    }

    // Apply as code patch (is executable code)
    if (ApplyDataPatch("AudioSampleRate", patchAddress, originalBytes,
        targetBytes, true)) {
        Log("Successfully applied Audio Sample Rate patch (Rate: " +
            std::to_string(sampleRate) + ").");
        return true;
    }
    else {
        Log("Failed to apply Audio Sample Rate patch.");
        return false;
    }
}

// *** NEW PATCH FUNCTION ***
// Apply the patch to increase chunk dimension categorization limit
bool ApplyChunkDimensionPatch() {
    Log("Checking Chunk Dimension patch...");

    bool needsPatch = false;
    int giganticMapSize = 0;
    std::vector<int> customSizes;

    // Check Gigantic Map Size
    if (GetConfigBool("GiganticMapSizeEnabled", true)) {
        giganticMapSize = GetConfigInt("GiganticMapSize", 220);
        if (giganticMapSize > 220) {
            Log("GiganticMapSize (" + std::to_string(giganticMapSize) +
                ") > 220. Chunk Dimension patch required.");
            needsPatch = true;
        }
    }

    // Check Custom Flat World Sizes (only if not already needed)
    if (!needsPatch && GetConfigBool("CustomFlatWorldSizesEnabled", true)) {
        std::string sizesStr =
            GetConfigString("CustomFlatWorldSizes", DEFAULT_FLAT_MAP_SIZES_STR);
        if (ParseIntList(sizesStr, customSizes)) {
            for (int size : customSizes) {
                if (size > 220) {
                    Log("Found CustomFlatWorldSize (" + std::to_string(size) +
                        ") > 220. Chunk Dimension patch required.");
                    needsPatch = true;
                    break; // Found one, no need to check further
                }
            }
        }
        else {
            Log("Warning: Could not parse CustomFlatWorldSizes for Chunk "
                "Dimension check.");
        }
    }

    if (!needsPatch) {
        Log("No map sizes > 220 found in config. Chunk Dimension patch not "
            "needed.");
        return false; // Not an error, just not needed
    }

    // Define the patch details
    std::string patchName = "ChunkDimensionLimit";
    // Original: F0 C7 45 FC 07 00 00 00 C7 (mov dword ptr [ebp-4], 7)
    std::string originalHex = "F0 C7 45 FC 07 00 00 00 C7";
    // Target:   F0 C7 45 FC 1F 00 00 00 C7 (mov dword ptr [ebp-4], 31)
    std::string targetHex = "F0 C7 45 FC 1F 00 00 00 C7";

    std::vector<unsigned char> originalBytes;
    std::vector<unsigned char> targetBytes;
    if (!HexToBytes(originalHex, originalBytes) ||
        !HexToBytes(targetHex, targetBytes)) {
        Log("Error: Failed to parse hex strings for " + patchName +
            " patch. Patch aborted.");
        return false;
    }

    MODULEINFO moduleInfo = { 0 };
    uintptr_t baseAddress = 0;
    size_t moduleSize = 0;
    if (!GetModuleInfoByName(g_executableName, moduleInfo, baseAddress,
        moduleSize)) {
        Log("Aborting " + patchName + " patch.");
        return false;
    }

    uintptr_t patchAddress =
        FindPattern(baseAddress, moduleSize, originalBytes);
    if (patchAddress == 0) {
        Log("Warning: Original " + patchName + " pattern not found in '" +
            g_executableName + "'. Patch cannot be applied.");
        return false;
    }

    // Apply as code patch (is executable code)
    if (ApplyDataPatch(patchName, patchAddress, originalBytes, targetBytes,
        true)) {
        Log("Successfully applied " + patchName + " patch.");
        return true;
    }
    else {
        Log("Failed to apply " + patchName + " patch.");
        return false;
    }
}
