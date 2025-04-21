#ifndef MEMORY_H
#define MEMORY_H

#include "pch.h"
#include "globals.h" // Include for MemoryPatch struct

// Function declarations
bool HexToBytes(const std::string& hex, std::vector<unsigned char>& bytes);
void IntToBytesLE(int val, std::vector<unsigned char>& bytes);
void IntsToBytesLE(const std::vector<int>& ints,
    std::vector<unsigned char>& bytes);
uintptr_t FindPattern(uintptr_t startAddress, size_t searchSize,
    const std::vector<unsigned char>& pattern);
bool ApplyStandardPatch(MemoryPatch& patch);
bool ApplyDataPatch(const std::string& patchName, uintptr_t patchAddress,
    const std::vector<unsigned char>& originalBytes,
    const std::vector<unsigned char>& targetBytes,
    bool isExecutable); // More generic patch helper

#endif // MEMORY_H