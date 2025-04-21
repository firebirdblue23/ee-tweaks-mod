#include "pch.h"
#include "globals.h" // Access g_executableName
#include "logging.h" // Access Log()
#include "config.h"  // Access GetConfigBool
#include "memory.h"

// --- Define Global Constants and Variables from globals.h ---
const char* MOD_VERSION = "1.1";
const char* MOD_AUTHOR = "firebirdblue";
const char* CONFIG_FILE = "tweaks.config";
const char* LOG_FILE = "tweaks_log.txt";
std::string g_executableName = "UNKNOWN_EXE";
std::string g_executablePath = "UNKNOWN_EXE_PATH";
std::string g_dllDir = ".";
HMODULE g_hModule = NULL;

const int NUM_FLAT_MAP_SIZES = 25;
const std::string DEFAULT_FLAT_MAP_SIZES_STR =
"15,25,50,75,90,100,125,140,150,160,175,200,225,250,300,350,400,450,500,"
"550,600,650,700,750,800";
const std::string ORIGINAL_FLAT_MAP_SIZES_HEX =
"0F 00 00 00 19 00 00 00 32 00 00 00 3C 00 00 00 46 00 00 00 4B 00 00 00 "
"50 00 00 00 5A 00 00 00 64 00 00 00 7D 00 00 00 82 00 00 00 87 00 00 00 "
"8C 00 00 00 91 00 00 00 96 00 00 00 9B 00 00 00 A0 00 00 00 A5 00 00 00 "
"AF 00 00 00 C8 00 00 00 E1 00 00 00 FA 00 00 00 2C 01 00 00 5E 01 00 00 "
"90 01 00 00";

// Define g_patches here
std::vector<MemoryPatch> g_patches = {
    {"SetSleepToZero",
        "GAME_EXECUTABLE", // Use placeholder for game exe
        "6A 01 6A 01 8B CE FF 50 10 6A 01",
        "6A 01 6A 01 8B CE FF 50 10 6A 00",
        false,
        0},
    {"IncreaseMemoryBufferDX7",
        "DX7HRDisplay.dll",
        "C7 45 FC 60 09 00 00",
        "C7 45 FC FF FF 00 00",
        false,
        0},
    {"IncreaseMemoryBufferDX7TnL",
        "DX7HRTnLDisplay.dll",
        "C7 45 FC 60 09 00 00",
        "C7 45 FC FF FF 00 00",
        false,
        0},
    {"VertexBufferSystemMem",
        "DX7HRTnLDisplay.dll",
        "C7 45 F4 00 00 01 00",
        "C7 45 F4 00 08 01 00",
        false,
        0},
    {"BypassMapSizeAssertion",
        "GAME_EXECUTABLE",
        "EB 2B 3D DC 00 00 00",
        "EB 2B 3D FF FF FF FF",
        false, // Will be forced true
        0}
        // Note: Audio patch is handled by ApplyAudioSampleRatePatch
        // Note: Map size patches are handled by ApplyCustomFlatWorldSizesPatch, ApplyGiganticMapSizePatch, ApplyMapGridLimitPatch, ApplyChunkDimensionPatch
};
// --- End Global Definitions ---

// Convert hex string (e.g., "6A 01") to byte vector
bool HexToBytes(const std::string& hex, std::vector<unsigned char>& bytes) {
    bytes.clear();
    std::stringstream ss(hex);
    std::string byteStr;
    unsigned int byteVal;

    while (ss >> byteStr) {
        std::stringstream converter;
        converter << std::hex << byteStr;
        if (!(converter >> byteVal) || byteVal > 255) {
            Log("Error: Invalid hex value '" + byteStr + "' in pattern.");
            return false; // Invalid hex character
        }
        bytes.push_back(static_cast<unsigned char>(byteVal));
    }
    return true;
}

// Convert a single 32-bit integer to a 4-byte little-endian vector
void IntToBytesLE(int val, std::vector<unsigned char>& bytes) {
    bytes.resize(4);
    bytes[0] = static_cast<unsigned char>(val & 0xFF);
    bytes[1] = static_cast<unsigned char>((val >> 8) & 0xFF);
    bytes[2] = static_cast<unsigned char>((val >> 16) & 0xFF);
    bytes[3] = static_cast<unsigned char>((val >> 24) & 0xFF);
}

// Convert vector of integers to little-endian byte vector (32-bit integers)
void IntsToBytesLE(const std::vector<int>& ints,
    std::vector<unsigned char>& bytes) {
    bytes.clear();
    bytes.reserve(ints.size() * sizeof(int)); // Reserve space (4 bytes per int)
    for (int val : ints) {
        bytes.push_back(static_cast<unsigned char>(val & 0xFF));
        bytes.push_back(static_cast<unsigned char>((val >> 8) & 0xFF));
        bytes.push_back(static_cast<unsigned char>((val >> 16) & 0xFF));
        bytes.push_back(static_cast<unsigned char>((val >> 24) & 0xFF));
    }
}

// Find a byte pattern within a memory range
uintptr_t FindPattern(uintptr_t startAddress, size_t searchSize,
    const std::vector<unsigned char>& pattern) {
    if (pattern.empty() || searchSize < pattern.size()) {
        return 0; // Invalid search parameters
    }

    const unsigned char* scanBytes =
        reinterpret_cast<const unsigned char*>(startAddress);
    size_t patternSize = pattern.size();
    if (searchSize < patternSize) return 0; // Prevent overflow
    size_t maxScanPos = searchSize - patternSize;

    for (size_t i = 0; i <= maxScanPos; ++i) {
        if (memcmp(scanBytes + i, pattern.data(), patternSize) == 0) {
            return startAddress + i; // Pattern found
        }
    }

    return 0; // Pattern not found
}

// Generic function to apply a patch (data or code)
bool ApplyDataPatch(const std::string& patchName, uintptr_t patchAddress,
    const std::vector<unsigned char>& originalBytes,
    const std::vector<unsigned char>& targetBytes, bool isExecutable) {
    if (patchAddress == 0) {
        Log("Error: Cannot apply patch '" + patchName +
            "', patch address is null.");
        return false;
    }
    if (originalBytes.size() != targetBytes.size()) {
        Log("Error: Original and target byte patterns have different sizes for "
            "patch '" +
            patchName + "'.");
        return false;
    }
    if (targetBytes.empty()) {
        Log("Error: Target byte pattern is empty for patch '" + patchName + "'.");
        return false;
    }

    size_t patchSize = targetBytes.size();
    unsigned char* patchAddr = reinterpret_cast<unsigned char*>(patchAddress);
    std::stringstream addrHex;
    addrHex << "0x" << std::hex << patchAddress;

    // --- Safety Check: Verify original bytes before patching ---
    std::vector<unsigned char> currentBytes(patchSize);
    SIZE_T bytesRead = 0;
    if (!ReadProcessMemory(GetCurrentProcess(), patchAddr, currentBytes.data(),
        patchSize, &bytesRead) ||
        bytesRead != patchSize) {
        Log("Error: Failed to read memory at address " + addrHex.str() +
            " for verification before patch '" + patchName +
            "'. Error code: " + std::to_string(GetLastError()) +
            ". Patch aborted.");
        return false;
    }

    if (memcmp(currentBytes.data(), originalBytes.data(), patchSize) != 0) {
        std::stringstream ss_expected, ss_actual;
        ss_expected << std::hex << std::uppercase << std::setfill('0');
        ss_actual << std::hex << std::uppercase << std::setfill('0');
        for (unsigned char b : originalBytes)
            ss_expected << std::setw(2) << static_cast<int>(b) << " ";
        for (unsigned char b : currentBytes)
            ss_actual << std::setw(2) << static_cast<int>(b) << " ";

        Log("Error: Original bytes mismatch for patch '" + patchName +
            "' at address " + addrHex.str() + ".");
        Log("  Expected: " + ss_expected.str());
        Log("  Actual:   " + ss_actual.str());
        Log("  Patch aborted. Was the game already modified?");
        return false;
    }

    // --- Change memory protection ---
    DWORD oldProtect;
    DWORD newProtect =
        isExecutable ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
    if (!VirtualProtect(patchAddr, patchSize, newProtect, &oldProtect)) {
        Log("Error: Failed to change memory protection (" +
            std::string(isExecutable ? "RWX" : "RW") + ") for patch '" +
            patchName + "'. Error code: " + std::to_string(GetLastError()));
        return false;
    }

    // --- Apply the patch ---
    SIZE_T bytesWritten = 0;
    if (!WriteProcessMemory(GetCurrentProcess(), patchAddr, targetBytes.data(),
        patchSize, &bytesWritten) ||
        bytesWritten != patchSize) {
        Log("Error: Failed to write memory for patch '" + patchName +
            "'. Error code: " + std::to_string(GetLastError()) +
            ". Attempting to restore protection.");
        VirtualProtect(patchAddr, patchSize, oldProtect,
            &oldProtect); // Use oldProtect again
        return false;
    }

    // Flush instruction cache only if it was executable code
    if (isExecutable) {
        FlushInstructionCache(GetCurrentProcess(), patchAddr, patchSize);
    }

    // --- Restore original memory protection ---
    DWORD tempProtect; // Needs a variable, even if not used after
    if (!VirtualProtect(patchAddr, patchSize, oldProtect, &tempProtect)) {
        Log("Warning: Failed to restore original memory protection for patch '" +
            patchName + "'. Error code: " + std::to_string(GetLastError()));
        // Continue anyway, patch was applied
    }

    Log("Successfully applied patch '" + patchName + "' at address " +
        addrHex.str());
    return true;
}

// Apply a standard memory patch using the generic helper
bool ApplyStandardPatch(MemoryPatch& patch) {
    if (!patch.enabled || patch.patchAddress == 0) {
        // Log("Skipping patch '" + patch.name + "' (disabled or not found).");
        return false; // Not an error, just skipped
    }

    std::vector<unsigned char> originalBytes, targetBytes;
    if (!HexToBytes(patch.originalHex, originalBytes) ||
        !HexToBytes(patch.targetHex, targetBytes)) {
        Log("Error: Failed to parse hex strings for patch '" + patch.name +
            "'.");
        return false;
    }

    // Standard patches are assumed to be executable code
    return ApplyDataPatch(patch.name, patch.patchAddress, originalBytes,
        targetBytes, true);
}
