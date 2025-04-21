#include "pch.h"
#include "globals.h" // Access constants and g_dllDir
#include "logging.h" // Access Log() and g_loggingEnabled
#include "config.h"

// Define config global
std::map<std::string, std::string> g_config;

// Trim leading/trailing whitespace from a string
std::string Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (std::string::npos == first) {
        return ""; // Return empty string if all whitespace or empty
    }
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

// Get boolean value from config string
bool GetConfigBool(const std::string& key, bool defaultValue) {
    auto it = g_config.find(key);
    if (it != g_config.end()) {
        std::string valueStr = it->second; // Already trimmed during load
        std::transform(valueStr.begin(), valueStr.end(), valueStr.begin(),
            ::tolower);
        return (valueStr == "true" || valueStr == "1" || valueStr == "yes" ||
            valueStr == "on");
    }
    return defaultValue;
}

// Get string value from config string
std::string GetConfigString(const std::string& key,
    const std::string& defaultValue) {
    auto it = g_config.find(key);
    if (it != g_config.end()) {
        return it->second; // Return the trimmed string directly
    }
    return defaultValue;
}

// Get integer value from config string
int GetConfigInt(const std::string& key, int defaultValue) {
    std::string valueStr = GetConfigString(key, "");
    if (valueStr.empty()) {
        return defaultValue;
    }
    try {
        long long val_ll = std::stoll(valueStr);
        if (val_ll < INT_MIN || val_ll > INT_MAX) {
            throw std::out_of_range("Value out of range for int type");
        }
        return static_cast<int>(val_ll);
    }
    catch (const std::invalid_argument& e) {
        Log("Config Error: Invalid integer format for key '" + key +
            "', value '" + valueStr + "'. Using default " +
            std::to_string(defaultValue) + ". Error: " + e.what());
        return defaultValue;
    }
    catch (const std::out_of_range& e) {
        Log("Config Error: Integer out of range for key '" + key +
            "', value '" + valueStr + "'. Using default " +
            std::to_string(defaultValue) + ". Error: " + e.what());
        return defaultValue;
    }
}

// Create the default configuration file
void CreateDefaultConfig(const std::string& configPath) {
    // Use OutputDebugStringA as logging isn't fully set up yet
    OutputDebugStringA(
        ("tweaks.dll: Creating default configuration file: " + configPath + "\n")
        .c_str());
    std::ofstream configFile(configPath);
    if (!configFile.is_open()) {
        OutputDebugStringA(
            ("tweaks.dll: Error: Could not create default config file!\n"));
        return;
    }

    configFile << "; Configuration file for Empire Earth Tweaks v" << MOD_VERSION
        << "\n"; // Use global constant
    configFile << "; Mod by: " << MOD_AUTHOR << "\n"; // Use global constant
    configFile << "; Use true/false, 1/0, yes/no, or on/off to enable/disable "
        "options.\n";
    configFile << "; Lines starting with ; or # are comments.\n\n";

    configFile << "; --- General Settings ---\n";
    configFile << "; Enable or disable the creation of the " << LOG_FILE
        << " file.\n";
    configFile << "; Debug messages are always sent to the debugger output.\n";
    configFile << "EnableLogging=true\n\n";

    configFile << "; --- Standard Patches ---\n";
    configFile << "SetSleepToZeroEnabled=true\n";
    configFile << "IncreaseMemoryBufferDX7Enabled=true\n";
    configFile << "IncreaseMemoryBufferDX7TnLEnabled=true\n";
    configFile << "VertexBufferSystemMemEnabled=true\n";

    configFile << "; --- Audio Settings ---\n";
    configFile << "; Set the desired audio sample rate for the Miles Sound "
        "System.\n";
    configFile << "; Common values: 22050, 44100, 48000. Default is 44100.\n";
    configFile << "; The game's original is 22050.\n";
    configFile << "AudioSampleRate=44100\n\n";

    configFile << "; --- Custom Flat World Sizes ---\n";
    configFile << "; Enables patching the list of available flat map sizes.\n";
    configFile << "CustomFlatWorldSizesEnabled=true\n\n";
    configFile << "; Comma-separated list of exactly " << NUM_FLAT_MAP_SIZES
        << " world sizes (integer values).\n";
    configFile << "; Default recommended list:\n";
    configFile << "CustomFlatWorldSizes=" << DEFAULT_FLAT_MAP_SIZES_STR
        << "\n\n";

    configFile << "; --- Custom Gigantic Map Size ---\n";
    configFile << "; Enables patching the Gigantic map size (default 220).\n";
    configFile << "GiganticMapSizeEnabled=true\n\n";
    configFile << "; The desired dimension for the Gigantic map (e.g., 512 for "
        "512x512).\n";
    configFile << "; Default game value is 220.\n";
    configFile << "; 4GB Patch is recommended for big maps.\n";
    configFile << "GiganticMapSize=512\n";

    configFile.close();
    OutputDebugStringA(
        ("tweaks.dll: Default configuration file created successfully.\n"));
}

// Load configuration from tweaks.config
void LoadConfig() {
    std::string configPath = g_dllDir + "\\" + CONFIG_FILE;
    std::ifstream configFile(configPath);

    std::error_code ec;
    if (!std::filesystem::exists(configPath, ec) || ec) {
        if (ec) {
            OutputDebugStringA(
                ("tweaks.dll: Warning: Filesystem error checking config "
                    "existence: " +
                    ec.message() + "\n")
                .c_str());
        }
        OutputDebugStringA(
            ("tweaks.dll: Warning: " + std::string(CONFIG_FILE) +
                " not found in DLL directory (" + g_dllDir + ").\n")
            .c_str());
        configFile.close();
        CreateDefaultConfig(configPath);
        configFile.open(configPath); // Try opening again
    }

    OutputDebugStringA(
        ("tweaks.dll: Loading configuration from " + configPath + "...\n")
        .c_str());

    if (!configFile.is_open()) {
        OutputDebugStringA(
            ("tweaks.dll: Error: Could not open " + configPath +
                ". Using default settings.\n")
            .c_str());
        g_config.clear();
        // Keep g_loggingEnabled = true (default) if config fails to load
        return;
    }

    g_config.clear(); // Clear previous config
    std::string line;
    int lineNumber = 0;
    while (getline(configFile, line)) {
        lineNumber++;
        line = Trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue; // Skip empty lines and comments
        }

        size_t equalsPos = line.find('=');
        if (equalsPos == std::string::npos) {
            OutputDebugStringA(
                ("tweaks.dll: Warning: Skipping invalid config line #" +
                    std::to_string(lineNumber) + ": " + line + "\n")
                .c_str());
            continue;
        }

        std::string key = Trim(line.substr(0, equalsPos));
        std::string value = Trim(line.substr(equalsPos + 1));

        if (key.empty()) {
            OutputDebugStringA(
                ("tweaks.dll: Warning: Skipping config line #" +
                    std::to_string(lineNumber) + " with empty key.\n")
                .c_str());
            continue;
        }

        g_config[key] = value;
    }

    configFile.close();

    // Read Logging Enable Flag *after* parsing the whole file
    g_loggingEnabled = GetConfigBool("EnableLogging", true);

    OutputDebugStringA(
        ("tweaks.dll: Configuration loaded. Logging " +
            std::string(g_loggingEnabled ? "enabled" : "disabled") + ".\n")
        .c_str());
}

// Parse comma-separated integer list
bool ParseIntList(const std::string& str, std::vector<int>& result) {
    result.clear();
    std::stringstream ss(str);
    std::string segment;

    while (std::getline(ss, segment, ',')) {
        std::string trimmedSegment = Trim(segment);
        if (trimmedSegment.empty()) continue; // Skip empty segments

        try {
            long long val_ll = std::stoll(trimmedSegment);
            if (val_ll < INT_MIN || val_ll > INT_MAX) {
                throw std::out_of_range(
                    "Integer value out of range for int type");
            }
            result.push_back(static_cast<int>(val_ll));
        }
        catch (const std::invalid_argument& e) {
            Log("Error parsing integer list: Invalid number format '" +
                trimmedSegment + "'. " + e.what());
            return false;
        }
        catch (const std::out_of_range& e) {
            Log("Error parsing integer list: Number out of range '" +
                trimmedSegment + "'. " + e.what());
            return false;
        }
    }
    return true;
}
