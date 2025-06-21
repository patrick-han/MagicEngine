#include "Log.h"
#include <iostream>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace Magic
{
std::vector<LogEntry> Logger::m_messages;

std::string CurrentDateTimeString() {
    std::time_t time = std::time(nullptr);
    tm localtime = *std::localtime(&time);
    std::ostringstream oss;
    oss << std::put_time(&localtime, "%m-%d-%Y %H:%M:%S");
    return oss.str();
}

void Logger::Info(const std::string& message) {
    std::string logMessage = std::format("\033[1;32m INFO: [{}] - {}\033[0m", CurrentDateTimeString(), message);
    std::cout  << logMessage << "\n";
    m_messages.emplace_back(LogType::INFO, logMessage);
}

void Logger::Warn(const std::string &message) {
    std::string logMessage = std::format("\033[1;33m WARN: [{}] - {}\033[0m", CurrentDateTimeString(), message);
    std::cout  << logMessage << "\n";
    m_messages.emplace_back(LogType::WARN, logMessage);
}


void Logger::Err(const std::string &message) {
    std::string logMessage = std::format("\033[1;31m ERROR: [{}] - {}\033[0m", CurrentDateTimeString(), message);
    std::cout  << logMessage << "\n";
    m_messages.emplace_back(LogType::ERROR, logMessage);
}
}