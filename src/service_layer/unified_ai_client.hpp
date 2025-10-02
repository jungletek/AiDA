#pragma once

#include "ai_provider_config.hpp"
#include "connection_pool.hpp"
#include "request_cache.hpp"
#include "settings.hpp"
#include "string_utils.hpp"
#include "constants.hpp"

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <nlohmann/json.hpp>

// Unified AI Client - replaces 6 different AI client classes
// Configuration-driven behavior for all AI providers

class UnifiedAIClient
{
public:
    using callback_t = std::function<void(const std::string&)>;
    using addr_callback_t = std::function<void(uint64_t)>;

    // Constructor with provider configuration
    explicit UnifiedAIClient(const AIProviderConfig& config,
                           ConnectionPool& connection_pool,
                           RequestCache& request_cache);

    // Constructor with provider name (uses built-in config)
    explicit UnifiedAIClient(const std::string& provider_name,
                           ConnectionPool& connection_pool,
                           RequestCache& request_cache);

    ~UnifiedAIClient();

    // Connection test result structure
    struct ConnectionTestResult {
        bool success;
        std::string message;
        std::string details;
        int response_time_ms;
    };

    // Core AI client interface (using standard C++ types)
    bool is_available() const;
    ConnectionTestResult test_connection();
    void analyze_function(uint64_t address, callback_t callback);
    void suggest_name(uint64_t address, callback_t callback);
    void generate_struct(uint64_t address, callback_t callback);
    void generate_comment(uint64_t address, callback_t callback);
    void generate_hook(uint64_t address, callback_t callback);
    void custom_query(uint64_t address, const std::string& question, callback_t callback);
    void locate_global_pointer(uint64_t address, const std::string& target_name, addr_callback_t callback);
    void rename_all(uint64_t address, callback_t callback);

    // Cancel current request
    void cancel_current_request();

    // Get current provider configuration
    const AIProviderConfig& get_config() const { return _config; }

    // Update provider configuration
    void update_config(const AIProviderConfig& new_config);

    // Get provider statistics
    struct ProviderStats {
        int total_requests = 0;
        int cached_requests = 0;
        int failed_requests = 0;
        double average_response_time_ms = 0.0;
        std::chrono::steady_clock::time_point last_request_time;
    };

    ProviderStats get_stats() const;

private:
    AIProviderConfig _config;
    ConnectionPool& _connection_pool;
    RequestCache& _request_cache;

    // Threading and state management
    std::thread _worker_thread;
    std::mutex _worker_mutex;
    std::condition_variable _worker_cv;
    std::atomic<bool> _should_stop{false};
    std::atomic<bool> _is_processing{false};

    // Request state
    mutable std::mutex _stats_mutex;
    mutable ProviderStats _stats;

    // Request payload builders for different formats
    nlohmann::json build_openai_payload(const std::string& prompt, double temperature) const;
    nlohmann::json build_anthropic_payload(const std::string& prompt, double temperature) const;
    nlohmann::json build_gemini_payload(const std::string& prompt, double temperature) const;
    nlohmann::json build_generic_payload(const std::string& prompt, double temperature) const;

    // Response parsers for different formats
    std::string parse_openai_response(const nlohmann::json& response) const;
    std::string parse_anthropic_response(const nlohmann::json& response) const;
    std::string parse_gemini_response(const nlohmann::json& response) const;
    std::string parse_generic_response(const nlohmann::json& response) const;

    // Main request processing method
    void _process_request(const std::string& prompt,
                         callback_t callback,
                         double temperature,
                         const std::string& request_type);

    // HTTP request execution with retry logic
    std::string _execute_http_request(const std::string& url,
                                    const thirdparty_compat::Headers& headers,
                                    const std::string& body);

    // Request caching key generation
    std::string _generate_cache_key(const std::string& prompt,
                                   const std::string& model_name,
                                   double temperature) const;

    // Update statistics
    void _update_stats(bool success, double response_time_ms, bool was_cached) const;

    // Validate API key format for current provider
    bool _validate_api_key(const std::string& api_key) const;

    // Get model name from settings or config default
    std::string _get_model_name() const;

    // Worker thread main loop
    void _worker_thread_main();

    // Process a single request (called by worker thread)
    void _process_single_request(const std::string& prompt,
                                callback_t callback,
                                double temperature,
                                const std::string& request_type);
};

// Factory function to create unified client
std::unique_ptr<UnifiedAIClient> create_unified_client(
    const std::string& provider_name,
    ConnectionPool& connection_pool,
    RequestCache& request_cache
);

// Global instances for easy access
extern ConnectionPool g_connection_pool;
extern RequestCache g_request_cache;

// Exception types for different error conditions
class AIClientException : public std::runtime_error {
public:
    explicit AIClientException(const std::string& message) : std::runtime_error(message) {}
};

class NetworkError : public AIClientException {
public:
    explicit NetworkError(const std::string& message) : AIClientException(message) {}
};

class APIError : public AIClientException {
public:
    explicit APIError(const std::string& message) : AIClientException(message) {}
};

class ConfigurationError : public AIClientException {
public:
    explicit ConfigurationError(const std::string& message) : AIClientException(message) {}
};

class RateLimitError : public AIClientException {
public:
    explicit RateLimitError(const std::string& message) : AIClientException(message) {}
};
