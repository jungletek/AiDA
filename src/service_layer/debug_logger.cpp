#include "debug_logger.hpp"
#include "thirdparty_compat.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <regex>
#include <iostream>
#include <fstream>
#include <cstdarg>

// Standard C++ logging functions (IDA SDK independent)
inline void log_info(const char* format, ...) {
    va_list va;
    va_start(va, format);
    vprintf(format, va);
    va_end(va);
    printf("\n");  // Ensure newline
}

inline void log_to_file(const char* format, ...) {
    static std::ofstream log_file("AiDA_debug.log", std::ios::app);
    if (!log_file) return;

    va_list va;
    va_start(va, format);
    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), format, va);
    va_end(va);

    log_file << buffer << std::endl;
}

std::mutex DebugLogger::log_mutex;

std::string DebugLogger::get_timestamp()
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string DebugLogger::sanitize_api_key(const std::string& text)
{
    if (!g_settings.debug_mode) {
        return "[REDACTED]";
    }
    
    // Simple regex to find API keys (common patterns)
    std::regex api_key_pattern("(sk-[a-zA-Z0-9]{20,}|[a-zA-Z0-9]{32,})");
    return std::regex_replace(text, api_key_pattern, "[API_KEY_REDACTED]");
}

std::string DebugLogger::sanitize_json(const std::string& json_text)
{
    if (!g_settings.debug_mode) {
        return "[REDACTED]";
    }
    
    // For debug mode, we can show more details but still sanitize sensitive fields
    std::string sanitized = json_text;
    
    // Remove API keys from JSON
    std::regex api_key_field("\"(api_key|apiKey|key|token)\"\\s*:\\s*\"[^\"]*\"");
    sanitized = std::regex_replace(sanitized, api_key_field, "\"$1\":\"[REDACTED]\"");
    
    // Remove authorization headers
    std::regex auth_header("\"Authorization\"\\s*:\\s*\"[^\"]*\"");
    sanitized = std::regex_replace(sanitized, auth_header, "\"Authorization\":\"[REDACTED]\"");
    
    return sanitized;
}

void DebugLogger::log_api_call(const std::string& provider, const std::string& operation)
{
    if (!g_settings.debug_mode) return;

    std::lock_guard<std::mutex> lock(log_mutex);
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "[AiDA DEBUG][%s] API Call: Provider=%s, Operation=%s",
        get_timestamp().c_str(), provider.c_str(), operation.c_str());
    log_info(buffer);
    log_to_file(buffer);
}

void DebugLogger::log_error(const std::string& context, const std::exception& e)
{
    if (!g_settings.debug_mode) return;

    std::lock_guard<std::mutex> lock(log_mutex);
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "[AiDA DEBUG][%s] ERROR: Context=%s, Exception=%s",
        get_timestamp().c_str(), context.c_str(), e.what());
    log_info(buffer);
    log_to_file(buffer);
}

void DebugLogger::log_memory_usage()
{
    if (!g_settings.debug_mode) return;

    std::lock_guard<std::mutex> lock(log_mutex);

    // Basic memory usage tracking - in a real implementation, you might use platform-specific APIs
    // For now, we'll just log a placeholder message
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "[AiDA DEBUG][%s] Memory Usage: [Placeholder - implement platform-specific memory tracking]",
        get_timestamp().c_str());
    log_info(buffer);
    log_to_file(buffer);
}

void DebugLogger::log_request(const std::string& provider, const std::string& endpoint, const std::string& request_body)
{
    if (!g_settings.debug_mode) return;

    std::lock_guard<std::mutex> lock(log_mutex);
    std::string sanitized_body = sanitize_json(request_body);

    char buffer1[512];
    snprintf(buffer1, sizeof(buffer1), "[AiDA DEBUG][%s] REQUEST: Provider=%s, Endpoint=%s",
        get_timestamp().c_str(), provider.c_str(), endpoint.c_str());
    log_info(buffer1);
    log_to_file(buffer1);

    char buffer2[512];
    snprintf(buffer2, sizeof(buffer2), "[AiDA DEBUG][%s] Request Body: %s",
        get_timestamp().c_str(), sanitized_body.c_str());
    log_info(buffer2);
    log_to_file(buffer2);
}

void DebugLogger::log_response(const std::string& provider, const std::string& endpoint, const std::string& response_body, int status_code)
{
    if (!g_settings.debug_mode) return;

    std::lock_guard<std::mutex> lock(log_mutex);
    std::string sanitized_body = sanitize_json(response_body);

    char buffer1[512];
    snprintf(buffer1, sizeof(buffer1), "[AiDA DEBUG][%s] RESPONSE: Provider=%s, Endpoint=%s, Status=%d",
        get_timestamp().c_str(), provider.c_str(), endpoint.c_str(), status_code);
    log_info(buffer1);
    log_to_file(buffer1);

    char buffer2[512];
    snprintf(buffer2, sizeof(buffer2), "[AiDA DEBUG][%s] Response Body: %s",
        get_timestamp().c_str(), sanitized_body.c_str());
    log_info(buffer2);
    log_to_file(buffer2);
}

void DebugLogger::log_thread_state(const std::string& context)
{
    if (!g_settings.debug_mode) return;

    std::lock_guard<std::mutex> lock(log_mutex);
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "[AiDA DEBUG][%s] THREAD STATE: Context=%s",
        get_timestamp().c_str(), context.c_str());
    log_info(buffer);
    log_to_file(buffer);
}

void DebugLogger::log_performance(const std::string& operation, int64_t duration_ms)
{
    if (!g_settings.debug_mode) return;

    std::lock_guard<std::mutex> lock(log_mutex);
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "[AiDA DEBUG][%s] PERFORMANCE: Operation=%s, Duration=%lld ms",
        get_timestamp().c_str(), operation.c_str(), duration_ms);
    log_info(buffer);
    log_to_file(buffer);
}

void DebugLogger::log_memory_allocation(const std::string& context, size_t size)
{
    if (!g_settings.debug_mode) return;

    std::lock_guard<std::mutex> lock(log_mutex);
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "[AiDA DEBUG][%s] MEMORY ALLOCATION: Context=%s, Size=%zu bytes",
        get_timestamp().c_str(), context.c_str(), size);
    log_info(buffer);
    log_to_file(buffer);
}

void DebugLogger::log_memory_deallocation(const std::string& context, size_t size)
{
    if (!g_settings.debug_mode) return;

    std::lock_guard<std::mutex> lock(log_mutex);
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "[AiDA DEBUG][%s] MEMORY DEALLOCATION: Context=%s, Size=%zu bytes",
        get_timestamp().c_str(), context.c_str(), size);
    log_info(buffer);
    log_to_file(buffer);
}

void DebugLogger::log_json_parsing(const std::string& context, const std::string& json_data)
{
    if (!g_settings.debug_mode) return;

    std::lock_guard<std::mutex> lock(log_mutex);
    size_t json_size = json_data.size();
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "[AiDA DEBUG][%s] JSON PARSING: Context=%s, Size=%zu bytes",
        get_timestamp().c_str(), context.c_str(), json_size);
    log_info(buffer);
    log_to_file(buffer);

    // Log bounds check for large JSON responses
    const size_t MAX_JSON_SIZE = 10 * 1024 * 1024; // 10MB
    if (json_size > MAX_JSON_SIZE) {
        char warning_buffer[512];
        snprintf(warning_buffer, sizeof(warning_buffer), "[AiDA DEBUG][%s] JSON WARNING: Large JSON response detected (%zu bytes)",
            get_timestamp().c_str(), json_size);
        log_info(warning_buffer);
        log_to_file(warning_buffer);
    }
}

void DebugLogger::log_bounds_check(const std::string& context, size_t actual_size, size_t expected_max_size)
{
    if (!g_settings.debug_mode) return;

    std::lock_guard<std::mutex> lock(log_mutex);
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "[AiDA DEBUG][%s] BOUNDS CHECK: Context=%s, Actual=%zu, ExpectedMax=%zu",
        get_timestamp().c_str(), context.c_str(), actual_size, expected_max_size);
    log_info(buffer);
    log_to_file(buffer);

    if (actual_size > expected_max_size) {
        char violation_buffer[512];
        snprintf(violation_buffer, sizeof(violation_buffer), "[AiDA DEBUG][%s] BOUNDS VIOLATION: Size exceeds expected maximum!",
            get_timestamp().c_str());
        log_info(violation_buffer);
        log_to_file(violation_buffer);
    }
}
