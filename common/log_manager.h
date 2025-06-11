// common/log_manager.h
#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <memory>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace Common {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    CRITICAL = 4
};

class LogManager {
public:
    static LogManager& Instance();

    // 로그 레벨 설정
    void SetLogLevel(LogLevel level);
    void SetConsoleOutput(bool enabled);
    void SetFileOutput(bool enabled, const std::string& filename = "");

    // 로그 출력
    void Log(LogLevel level, const std::string& category, const std::string& message);
    void Debug(const std::string& category, const std::string& message);
    void Info(const std::string& category, const std::string& message);
    void Warning(const std::string& category, const std::string& message);
    void Error(const std::string& category, const std::string& message);
    void Critical(const std::string& category, const std::string& message);

    // 편의 함수들 - 오버로드 버전 추가
    void LogFormat(LogLevel level, const std::string& category, const std::string& format);
    void DebugFormat(const std::string& category, const std::string& format);
    void InfoFormat(const std::string& category, const std::string& format);
    void WarningFormat(const std::string& category, const std::string& format);
    void ErrorFormat(const std::string& category, const std::string& format);
    void CriticalFormat(const std::string& category, const std::string& format);

    template<typename... Args>
    void LogFormat(LogLevel level, const std::string& category, const std::string& format, Args... args);

    template<typename... Args>
    void DebugFormat(const std::string& category, const std::string& format, Args... args);

    template<typename... Args>
    void InfoFormat(const std::string& category, const std::string& format, Args... args);

    template<typename... Args>
    void WarningFormat(const std::string& category, const std::string& format, Args... args);

    template<typename... Args>
    void ErrorFormat(const std::string& category, const std::string& format, Args... args);

    template<typename... Args>
    void CriticalFormat(const std::string& category, const std::string& format, Args... args);

private:
    LogManager();
    ~LogManager();

    LogManager(const LogManager&) = delete;
    LogManager& operator=(const LogManager&) = delete;

    std::string GetTimestamp() const;
    std::string LogLevelToString(LogLevel level) const;
    void WriteToFile(const std::string& message);
    void WriteToConsole(const std::string& message);

    template<typename... Args>
    std::string FormatString(const std::string& format, Args... args);

    LogLevel min_log_level_;
    bool console_output_;
    bool file_output_;
    std::string log_filename_;
    std::ofstream log_file_;
    mutable std::mutex log_mutex_;
};

// 매크로 정의 (편의성을 위해)
#define LOG_DEBUG(category, message) \
    Common::LogManager::Instance().Debug(category, message)

#define LOG_INFO(category, message) \
    Common::LogManager::Instance().Info(category, message)

#define LOG_WARNING(category, message) \
    Common::LogManager::Instance().Warning(category, message)

#define LOG_ERROR(category, message) \
    Common::LogManager::Instance().Error(category, message)

#define LOG_CRITICAL(category, message) \
    Common::LogManager::Instance().Critical(category, message)

// 포맷 버전 - C++11 호환 가변 매크로
#if defined(__GNUC__) || defined(__clang__)
    #define LOG_DEBUG_FORMAT(category, format, ...) \
        Common::LogManager::Instance().DebugFormat(category, format, ##__VA_ARGS__)

    #define LOG_INFO_FORMAT(category, format, ...) \
        Common::LogManager::Instance().InfoFormat(category, format, ##__VA_ARGS__)

    #define LOG_WARNING_FORMAT(category, format, ...) \
        Common::LogManager::Instance().WarningFormat(category, format, ##__VA_ARGS__)

    #define LOG_ERROR_FORMAT(category, format, ...) \
        Common::LogManager::Instance().ErrorFormat(category, format, ##__VA_ARGS__)

    #define LOG_CRITICAL_FORMAT(category, format, ...) \
        Common::LogManager::Instance().CriticalFormat(category, format, ##__VA_ARGS__)
#else
    // MSVC 호환 버전
    #define LOG_DEBUG_FORMAT(category, format, ...) \
        Common::LogManager::Instance().DebugFormat(category, format, __VA_ARGS__)

    #define LOG_INFO_FORMAT(category, format, ...) \
        Common::LogManager::Instance().InfoFormat(category, format, __VA_ARGS__)

    #define LOG_WARNING_FORMAT(category, format, ...) \
        Common::LogManager::Instance().WarningFormat(category, format, __VA_ARGS__)

    #define LOG_ERROR_FORMAT(category, format, ...) \
        Common::LogManager::Instance().ErrorFormat(category, format, __VA_ARGS__)

    #define LOG_CRITICAL_FORMAT(category, format, ...) \
        Common::LogManager::Instance().CriticalFormat(category, format, __VA_ARGS__)
#endif

// 템플릿 구현
template<typename... Args>
void LogManager::LogFormat(LogLevel level, const std::string& category, const std::string& format, Args... args) {
    if (static_cast<int>(level) >= static_cast<int>(min_log_level_)) {
        std::string formatted_message = FormatString(format, args...);
        Log(level, category, formatted_message);
    }
}

template<typename... Args>
void LogManager::DebugFormat(const std::string& category, const std::string& format, Args... args) {
    LogFormat(LogLevel::DEBUG, category, format, args...);
}

template<typename... Args>
void LogManager::InfoFormat(const std::string& category, const std::string& format, Args... args) {
    LogFormat(LogLevel::INFO, category, format, args...);
}

template<typename... Args>
void LogManager::WarningFormat(const std::string& category, const std::string& format, Args... args) {
    LogFormat(LogLevel::WARNING, category, format, args...);
}

template<typename... Args>
void LogManager::ErrorFormat(const std::string& category, const std::string& format, Args... args) {
    LogFormat(LogLevel::ERROR, category, format, args...);
}

template<typename... Args>
void LogManager::CriticalFormat(const std::string& category, const std::string& format, Args... args) {
    LogFormat(LogLevel::CRITICAL, category, format, args...);
}

template<typename... Args>
std::string LogManager::FormatString(const std::string& format, Args... args) {
    // 간단한 sprintf 스타일 포맷팅
    // 실제로는 더 안전한 방법을 사용해야 함 (fmt 라이브러리 등)
    size_t size = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(), buf.get() + size - 1);
}

} // namespace Common