// common/log_manager.cpp
#include "log_manager.h"
#include <iostream>
#include <filesystem>

namespace Common {

LogManager& LogManager::Instance() {
    static LogManager instance;
    return instance;
}

LogManager::LogManager()
    : min_log_level_(LogLevel::INFO)
    , console_output_(true)
    , file_output_(false) {
}

LogManager::~LogManager() {
    if (log_file_.is_open()) {
        log_file_.close();
    }
}

void LogManager::SetLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    min_log_level_ = level;
}

void LogManager::SetConsoleOutput(bool enabled) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    console_output_ = enabled;
}

void LogManager::SetFileOutput(bool enabled, const std::string& filename) {
    std::lock_guard<std::mutex> lock(log_mutex_);

    file_output_ = enabled;

    if (enabled) {
        if (log_file_.is_open()) {
            log_file_.close();
        }

        // 파일명이 제공되지 않은 경우 기본 파일명 생성
        if (filename.empty()) {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto tm = *std::localtime(&time_t);

            std::stringstream ss;
            ss << "mmorpg_server_" << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".log";
            log_filename_ = ss.str();
        } else {
            log_filename_ = filename;
        }

        // 로그 디렉토리 생성
        std::filesystem::path log_path(log_filename_);
        auto parent_path = log_path.parent_path();
        if (!parent_path.empty() && !std::filesystem::exists(parent_path)) {
            std::filesystem::create_directories(parent_path);
        }

        log_file_.open(log_filename_, std::ios::out | std::ios::app);
        if (!log_file_.is_open()) {
            std::cerr << "Failed to open log file: " << log_filename_ << std::endl;
            file_output_ = false;
        }
    } else {
        if (log_file_.is_open()) {
            log_file_.close();
        }
    }
}

void LogManager::Log(LogLevel level, const std::string& category, const std::string& message) {
    if (static_cast<int>(level) < static_cast<int>(min_log_level_)) {
        return;
    }

    std::lock_guard<std::mutex> lock(log_mutex_);

    std::string timestamp = GetTimestamp();
    std::string level_str = LogLevelToString(level);

    std::stringstream ss;
    ss << "[" << timestamp << "] [" << level_str << "] [" << category << "] " << message;
    std::string formatted_message = ss.str();

    if (console_output_) {
        WriteToConsole(formatted_message);
    }

    if (file_output_ && log_file_.is_open()) {
        WriteToFile(formatted_message);
    }
}

void LogManager::Debug(const std::string& category, const std::string& message) {
    Log(LogLevel::DEBUG, category, message);
}

void LogManager::Info(const std::string& category, const std::string& message) {
    Log(LogLevel::INFO, category, message);
}

void LogManager::Warning(const std::string& category, const std::string& message) {
    Log(LogLevel::WARNING, category, message);
}

void LogManager::Error(const std::string& category, const std::string& message) {
    Log(LogLevel::ERROR, category, message);
}

void LogManager::Critical(const std::string& category, const std::string& message) {
    Log(LogLevel::CRITICAL, category, message);
}

// 포맷 함수 오버로드 (인자 없는 버전)
void LogManager::LogFormat(LogLevel level, const std::string& category, const std::string& format) {
    Log(level, category, format);
}

void LogManager::DebugFormat(const std::string& category, const std::string& format) {
    Log(LogLevel::DEBUG, category, format);
}

void LogManager::InfoFormat(const std::string& category, const std::string& format) {
    Log(LogLevel::INFO, category, format);
}

void LogManager::WarningFormat(const std::string& category, const std::string& format) {
    Log(LogLevel::WARNING, category, format);
}

void LogManager::ErrorFormat(const std::string& category, const std::string& format) {
    Log(LogLevel::ERROR, category, format);
}

void LogManager::CriticalFormat(const std::string& category, const std::string& format) {
    Log(LogLevel::CRITICAL, category, format);
}

std::string LogManager::GetTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);

    // 밀리초 계산
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();

    return ss.str();
}

std::string LogManager::LogLevelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG:    return "DEBUG";
        case LogLevel::INFO:     return "INFO ";
        case LogLevel::WARNING:  return "WARN ";
        case LogLevel::ERROR:    return "ERROR";
        case LogLevel::CRITICAL: return "CRIT ";
        default:                 return "UNKN ";
    }
}

void LogManager::WriteToFile(const std::string& message) {
    log_file_ << message << std::endl;
    log_file_.flush(); // 즉시 파일에 쓰기
}

void LogManager::WriteToConsole(const std::string& message) {
    std::cout << message << std::endl;
}

} // namespace Common