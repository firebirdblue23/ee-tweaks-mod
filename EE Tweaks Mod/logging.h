#ifndef LOGGING_H
#define LOGGING_H

#include "pch.h"

// Declare logging globals as extern
extern std::ofstream g_logFile;
extern bool g_loggingEnabled;

// Function declarations
std::string GetTimestamp();
void Log(const std::string& message);
void InitializeLogging(); // New function to handle log file opening
void ShutdownLogging();   // New function to handle log file closing

#endif // LOGGING_H