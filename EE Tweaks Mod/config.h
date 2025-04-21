#ifndef CONFIG_H
#define CONFIG_H

#include "pch.h"

// Declare config global as extern
extern std::map<std::string, std::string> g_config;

// Function declarations
std::string Trim(const std::string& str);
bool GetConfigBool(const std::string& key, bool defaultValue = false);
std::string GetConfigString(const std::string& key,
    const std::string& defaultValue = "");
int GetConfigInt(const std::string& key, int defaultValue = 0); // Helper for ints
void CreateDefaultConfig(const std::string& configPath);
void LoadConfig();
bool ParseIntList(const std::string& str, std::vector<int>& result);

#endif // CONFIG_H