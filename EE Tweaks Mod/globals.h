#ifndef GLOBALS_H
#define GLOBALS_H

#include "pch.h" // Include common headers

// --- Mod Info ---
extern const char* MOD_VERSION;
extern const char* MOD_AUTHOR;

// --- Configuration File Names ---
extern const char* CONFIG_FILE;
extern const char* LOG_FILE;

// --- Game/System Globals ---
extern std::string g_executableName; // Detected name of the game executable
extern std::string g_executablePath; // Full path of the game executable
extern std::string g_dllDir;         // Directory where this DLL resides
extern HMODULE g_hModule;            // Handle to this DLL instance

// --- Constants ---
extern const int NUM_FLAT_MAP_SIZES;
extern const std::string DEFAULT_FLAT_MAP_SIZES_STR;
extern const std::string ORIGINAL_FLAT_MAP_SIZES_HEX;

// --- Mod Definitions ---
struct MemoryPatch {
    std::string name;
    std::string moduleIdentifier; // "GAME_EXECUTABLE" or specific DLL name
    std::string originalHex;
    std::string targetHex;
    bool enabled;
    uintptr_t patchAddress; // To store the found address
};

// Declare g_patches as extern, it will be defined in memory.cpp
extern std::vector<MemoryPatch> g_patches;

#endif // GLOBALS_H