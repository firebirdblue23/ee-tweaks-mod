#include "pch.h"
#include "globals.h" // Access g_dllDir, LOG_FILE
#include "logging.h"

// Define logging globals
std::ofstream g_logFile;
bool g_loggingEnabled = true; // Enable logging by default

// Get current timestamp for logging
std::string GetTimestamp() {
    std::time_t now = std::time(nullptr);
    std::tm timeinfo;
    errno_t err = localtime_s(&timeinfo, &now);
    if (err != 0) {
        return "YYYY-MM-DD HH:MM:SS";
    }
    char buf[20];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return std::string(buf);
}

// Log messages to file and debugger
void Log(const std::string& message) {
    // Always output to debugger first for critical setup messages
    OutputDebugStringA(("tweaks.dll: " + message + "\n").c_str());

    // Only write to file if logging is enabled and file is open
    if (!g_loggingEnabled || !g_logFile.is_open()) {
        return;
    }
    g_logFile << "[" << GetTimestamp() << "] " << message << std::endl;
}

// Initialize logging (open file if enabled)
void InitializeLogging() {
    if (g_loggingEnabled) {
        std::string logPath = g_dllDir + "\\" + LOG_FILE;
        g_logFile.clear(); // Clear any error flags
        if (g_logFile.is_open()) g_logFile.close(); // Close if somehow already open
        g_logFile.open(logPath, std::ios::app); // Append mode
        if (!g_logFile.is_open()) {
            // Log file failed to open, disable further file logging attempts
            OutputDebugStringA(
                ("tweaks.dll: FATAL ERROR - Could not open log file " + logPath +
                    ". File logging disabled.\n")
                .c_str());
            g_loggingEnabled = false; // Prevent Log() from trying to write
        }
    }
    // Log initial status regardless of file success
    Log(std::string("File Logging: ") + (g_loggingEnabled ? "Enabled" : "Disabled"));
}

// Shutdown logging (close file)
void ShutdownLogging() {
    if (g_logFile.is_open()) {
        Log("Closing log file.");
        g_logFile.close();
    }
}
