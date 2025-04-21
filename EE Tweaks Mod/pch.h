// pch.h
#ifndef PCH_H
#define PCH_H

// Add headers that are used frequently but changed infrequently
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <psapi.h> // For GetModuleInformation
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>
#include <algorithm> // For std::transform
#include <cctype>    // For std::tolower
#include <filesystem> // For path operations (requires C++17)
#include <stdexcept>  // For std::stoi exceptions
#include <ctime>      // For timestamp in log

#endif // PCH_H
