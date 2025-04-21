#ifndef PATCHES_H
#define PATCHES_H

#include "pch.h"

bool GetModuleInfoByName(const std::string& moduleName, MODULEINFO& moduleInfo,
    uintptr_t& baseAddress, size_t& moduleSize);
// Function declarations for specific patches
bool ApplyCustomFlatWorldSizesPatch();
bool ApplyGiganticMapSizePatch();
bool ApplyMapGridLimitPatch();
bool ApplyAudioSampleRatePatch();
bool ApplyChunkDimensionPatch(); // New patch function

#endif // PATCHES_H