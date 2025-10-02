#include "debug_logger.hpp"
#include "settings.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

// Static member initialization
std::mutex DebugLogger::log_mutex;

// Get current timestamp for logging
std::string DebugLogger::get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

// Sanitize API key for logging (show only first/last 4 characters)
std::string DebugLogger::sanitize_api_key(const std::string& text) {
    if (text.length() <= 8) {
        return "****";
    }
    return text.substr(0, 4) + "****" + text.substr(text.length() - 4);
}

// Sanitize JSON for logging (truncate if too long)
std::string DebugLogger::sanitize_json(const std::string& json_text) {
    const size_t max_length = 500;
    if (json_text.length() <= max_length) {
        return json_text;
    }
    return json_text.substr(0, max_length) + "... [TRUNCATED]";
}

// Log API call
void DebugLogger::log_api_call(const std::string& provider, const std::string& operation) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << "[" << get_timestamp() << "] API_CALL: " << provider << " - " << operation << std::endl;
}

// Log error with exception details
void DebugLogger::log_error(const std::string& context, const std::exception& e) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << "[" << get_timestamp() << "] ERROR: " << context << " - " << e.what() << std::endl;
}

// Log memory usage (placeholder implementation)
void DebugLogger::log_memory_usage() {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << "[" << get_timestamp() << "] MEMORY: Usage check requested" << std::endl;
}

// Log HTTP request
void DebugLogger::log_request(const std::string& provider, const std::string& endpoint, const std::string& request_body) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << "[" << get_timestamp() << "] REQUEST: " << provider << " -> " << endpoint << std::endl;
    if (!request_body.empty()) {
        std::cout << "  Body: " << sanitize_json(request_body) << std::endl;
    }
}

// Log HTTP response
void DebugLogger::log_response(const std::string& provider, const std::string& endpoint, const std::string& response_body, int status_code) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << "[" << get_timestamp() << "] RESPONSE: " << provider << " <- " << endpoint << " (Status: " << status_code << ")" << std::endl;
    if (!response_body.empty()) {
        std::cout << "  Body: " << sanitize_json(response_body) << std::endl;
    }
}

// Log thread state
void DebugLogger::log_thread_state(const std::string& context) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << "[" << get_timestamp() << "] THREAD: " << context << std::endl;
}

// Log performance metrics
void DebugLogger::log_performance(const std::string& operation, int64_t duration_ms) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << "[" << get_timestamp() << "] PERFORMANCE: " << operation << " took " << duration_ms << "ms" << std::endl;
}

// Log memory allocation
void DebugLogger::log_memory_allocation(const std::string& context, size_t size) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << "[" << get_timestamp() << "] MEM_ALLOC: " << context << " allocated " << size << " bytes" << std::endl;
}

// Log memory deallocation
void DebugLogger::log_memory_deallocation(const std::string& context, size_t size) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << "[" << get_timestamp() << "] MEM_DEALLOC: " << context << " freed " << size << " bytes" << std::endl;
}

// Log JSON parsing
void DebugLogger::log_json_parsing(const std::string& context, const std::string& json_data) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << "[" << get_timestamp() << "] JSON_PARSE: " << context << std::endl;
    std::cout << "  Data: " << sanitize_json(json_data) << std::endl;
}

// Log bounds checking
void DebugLogger::log_bounds_check(const std::string& context, size_t actual_size, size_t expected_max_size) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << "[" << get_timestamp() << "] BOUNDS_CHECK: " << context
              << " (size: " << actual_size << ", max: " << expected_max_size << ")" << std::endl;
}
