#pragma once
#include <string>
#include <vector>
#include <format>

namespace Magic
{
enum class LogType {
    Info,
    Warn,
    Error
};

struct LogEntry {
    LogType type;
    std::string message;
};

class Logger {
public:
    static std::vector<LogEntry> m_messages;
    static void Info(const std::string& message);
    static void Warn(const std::string& message);
    static void Err(const std::string& message);
};


}
