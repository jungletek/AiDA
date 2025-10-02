#pragma once

#include <string>
#include <chrono>
#include <mutex>
#include "settings.hpp"

class DebugLogger
{
public:
    static void log_api_call(const std::string& provider, const std::string& operation);
    static void log_error(const std::string& context, const std::exception& e);
    static void log_memory_usage();
    static void log_request(const std::string& provider, const std::string& endpoint, const std::string& request_body);
    static void log_response(const std::string& provider, const std::string& endpoint, const std::string& response_body, int status_code);
    static void log_thread_state(const std::string& context);
    static void log_performance(const std::string& operation, int64_t duration_ms);
    static void log_memory_allocation(const std::string& context, size_t size);
    static void log_memory_deallocation(const std::string& context, size_t size);
    static void log_json_parsing(const std::string& context, const std::string& json_data);
    static void log_bounds_check(const std::string& context, size_t actual_size, size_t expected_max_size);

private:
    static std::string get_timestamp();
    static std::string sanitize_api_key(const std::string& text);
    static std::string sanitize_json(const std::string& json_text);
    static std::mutex log_mutex;
};
