#include "thirdparty_compat.hpp"
#include "unified_ai_client.hpp"
#include "debug_logger.hpp"
#include "ida_interface.hpp"
#include "ida_utils.hpp"
#include "settings.hpp"
#include <regex>
#include <sstream>
#include <iomanip>

// Initialize global instances
ConnectionPool g_connection_pool;
RequestCache g_request_cache(thirdparty_compat::minutes(30), 1000);  // 30 minute cache TTL

// UnifiedAIClient implementation
UnifiedAIClient::UnifiedAIClient(const AIProviderConfig& config,
                               ConnectionPool& connection_pool,
                               RequestCache& request_cache)
    : _config(config), _connection_pool(connection_pool), _request_cache(request_cache) {

    if (!_config.is_valid()) {
        throw ConfigurationError("Invalid provider configuration");
    }

    // Start worker thread for async processing
    _worker_thread = std::thread(&UnifiedAIClient::_worker_thread_main, this);
}

UnifiedAIClient::UnifiedAIClient(const std::string& provider_name,
                               ConnectionPool& connection_pool,
                               RequestCache& request_cache)
    : UnifiedAIClient(ProviderConfigs::get_provider_config(provider_name),
                     connection_pool, request_cache) {}

UnifiedAIClient::~UnifiedAIClient() {
    cancel_current_request();

    {
        std::lock_guard<thirdparty_compat::mutex> lock(_worker_mutex);
        _should_stop = true;
        _worker_cv.notify_one();
    }

    if (_worker_thread.joinable()) {
        _worker_thread.join();
    }
}

bool UnifiedAIClient::is_available() const {
    // Check if provider has valid configuration and API key
    std::string api_key = _config.get_auth_header(_config.auth_prefix);
    return _config.is_valid() && !_config.get_auth_header(_config.auth_prefix).empty();
}

UnifiedAIClient::ConnectionTestResult UnifiedAIClient::test_connection() {
    ConnectionTestResult result;
    result.success = false;

    auto start_time = std::chrono::steady_clock::now();

    try {
        // Build a simple test request
        std::string test_prompt = "Hello";
        std::string model_name = _get_model_name();

        // Check cache first
        std::string cache_key = _generate_cache_key(test_prompt, model_name, 0.1);
        auto cached_response = _request_cache.get(cache_key);

        if (cached_response) {
            result.success = true;
            result.message = "Connection test successful (cached)";
            result.response_time_ms = 0;
            _update_stats(true, 0.0, true);
            return result;
        }

        // Build test payload
        nlohmann::json payload;
        if (_config.request_format == "openai") {
            payload = build_openai_payload(test_prompt, 0.1);
        } else if (_config.request_format == "anthropic") {
            payload = build_anthropic_payload(test_prompt, 0.1);
        } else if (_config.request_format == "gemini") {
            payload = build_gemini_payload(test_prompt, 0.1);
        } else {
            payload = build_generic_payload(test_prompt, 0.1);
        }

        // Build headers
        thirdparty_compat::Headers headers = _config.default_headers;
        std::string auth_header = _config.get_auth_header(_config.auth_prefix);
        if (!auth_header.empty()) {
            headers.insert({ _config.auth_header_name, auth_header });
        }

        // Execute request
        std::string url = _config.get_request_url(model_name);
        std::string body = payload.dump();

        auto http_start = thirdparty_compat::steady_clock::now();
        std::string response = _execute_http_request(url, headers, body);
        auto http_end = thirdparty_compat::steady_clock::now();

        double response_time = std::chrono::duration_cast<std::chrono::milliseconds>(http_end - http_start).count();

        // Parse response
        nlohmann::json json_response = nlohmann::json::parse(response);
        std::string parsed_response;

        if (_config.response_parser == "openai") {
            parsed_response = parse_openai_response(json_response);
        } else if (_config.response_parser == "anthropic") {
            parsed_response = parse_anthropic_response(json_response);
        } else if (_config.response_parser == "gemini") {
            parsed_response = parse_gemini_response(json_response);
        } else {
            parsed_response = parse_generic_response(json_response);
        }

        result.success = true;
        result.message = "Connection test successful";
        result.details = "Response: " + parsed_response.substr(0, 100) + "...";
        result.response_time_ms = static_cast<int>(response_time);

        _update_stats(true, response_time, false);

    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        double response_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

        result.success = false;
        result.message = "Connection test failed";
        result.details = e.what();
        result.response_time_ms = static_cast<int>(response_time);

        _update_stats(false, response_time, false);

        DebugLogger::log_error("Connection test failed", e);
    }

    return result;
}

void UnifiedAIClient::analyze_function(ea_t ea, callback_t callback) {
    std::string prompt = IDAUtils::build_analysis_prompt(ea);
    _process_request(prompt, callback, 0.1, "analyze_function");
}

void UnifiedAIClient::suggest_name(ea_t ea, callback_t callback) {
    std::string prompt = IDAUtils::build_naming_prompt(ea);
    _process_request(prompt, callback, 0.1, "suggest_name");
}

void UnifiedAIClient::generate_struct(ea_t ea, callback_t callback) {
    std::string prompt = IDAUtils::build_struct_prompt(ea);
    _process_request(prompt, callback, 0.1, "generate_struct");
}

void UnifiedAIClient::generate_comment(ea_t ea, callback_t callback) {
    std::string prompt = IDAUtils::build_comment_prompt(ea);
    _process_request(prompt, callback, 0.1, "generate_comment");
}

void UnifiedAIClient::generate_hook(ea_t ea, callback_t callback) {
    std::string prompt = IDAUtils::build_hook_prompt(ea);
    _process_request(prompt, callback, 0.1, "generate_hook");
}

void UnifiedAIClient::custom_query(ea_t ea, const std::string& question, callback_t callback) {
    std::string prompt = IDAUtils::build_custom_prompt(ea, question);
    _process_request(prompt, callback, 0.1, "custom_query");
}

void UnifiedAIClient::locate_global_pointer(ea_t ea, const std::string& target_name, addr_callback_t callback) {
    // This is a more complex operation that may need special handling
    std::string prompt = IDAUtils::build_pointer_prompt(ea, target_name);
    _process_request(prompt, [callback](const std::string& response) {
        // Parse response for address
        ea_t address = IDAUtils::parse_address_from_response(response);
        if (address != BADADDR) {
            callback(address);
        }
    }, 0.1, "locate_global_pointer");
}

void UnifiedAIClient::rename_all(ea_t ea, callback_t callback) {
    std::string prompt = IDAUtils::build_rename_all_prompt(ea);
    _process_request(prompt, callback, 0.1, "rename_all");
}

void UnifiedAIClient::cancel_current_request() {
    _should_stop = true;
}

void UnifiedAIClient::update_config(const AIProviderConfig& new_config) {
    if (!new_config.is_valid()) {
        throw ConfigurationError("Invalid provider configuration");
    }

    std::lock_guard<thirdparty_compat::mutex> lock(_worker_mutex);
    _config = new_config;
}

UnifiedAIClient::ProviderStats UnifiedAIClient::get_stats() const {
    std::lock_guard<thirdparty_compat::mutex> lock(_stats_mutex);
    return _stats;
}

// Private implementation methods
void UnifiedAIClient::_process_request(const std::string& prompt,
                                     callback_t callback,
                                     double temperature,
                                     const std::string& request_type) {

    if (_is_processing) {
        DebugLogger::log_error("Request already in progress", std::runtime_error("Busy"));
        if (callback) callback("Error: Request already in progress");
        return;
    }

    // Submit request to worker thread
    {
        std::lock_guard<thirdparty_compat::mutex> lock(_worker_mutex);
        _is_processing = true;
        _should_stop = false;

        // Note: In a full implementation, we'd queue the request here
        // For now, we'll process it directly in the calling thread
    }

    try {
        _process_single_request(prompt, callback, temperature, request_type);
    } catch (const std::exception& e) {
        DebugLogger::log_error("Request processing failed", e);
        if (callback) callback("Error: " + std::string(e.what()));
    }

    _is_processing = false;
}

void UnifiedAIClient::_process_single_request(const std::string& prompt,
                                            callback_t callback,
                                            double temperature,
                                            const std::string& request_type) {

    auto start_time = std::chrono::steady_clock::now();

    try {
        std::string model_name = _get_model_name();

        // Check cache first
        std::string cache_key = _generate_cache_key(prompt, model_name, temperature);
        auto cached_response = _request_cache.get(cache_key);

        if (cached_response) {
            auto end_time = std::chrono::steady_clock::now();
            double response_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

            _update_stats(true, response_time, true);

            DebugLogger::log_performance("Cached request", response_time);
            if (callback) callback(*cached_response);
            return;
        }

        // Build request payload
        nlohmann::json payload;
        if (_config.request_format == "openai") {
            payload = build_openai_payload(prompt, temperature);
        } else if (_config.request_format == "anthropic") {
            payload = build_anthropic_payload(prompt, temperature);
        } else if (_config.request_format == "gemini") {
            payload = build_gemini_payload(prompt, temperature);
        } else {
            payload = build_generic_payload(prompt, temperature);
        }

        // Build headers
        thirdparty_compat::Headers headers = _config.default_headers;
        std::string auth_header = _config.get_auth_header(_config.auth_prefix);
        if (!auth_header.empty()) {
            headers.insert({ _config.auth_header_name, auth_header });
        }

        // Execute HTTP request
        std::string url = _config.get_request_url(model_name);
        std::string body = payload.dump();

        DebugLogger::log_request(_config.name, url, body);

        auto http_start = thirdparty_compat::steady_clock::now();
        std::string response_body = _execute_http_request(url, headers, body);
        auto http_end = thirdparty_compat::steady_clock::now();

        double response_time = std::chrono::duration_cast<std::chrono::milliseconds>(http_end - http_start).count();

        DebugLogger::log_response(_config.name, url, response_body, 200);

        // Parse response
        nlohmann::json json_response = nlohmann::json::parse(response_body);
        std::string parsed_response;

        if (_config.response_parser == "openai") {
            parsed_response = parse_openai_response(json_response);
        } else if (_config.response_parser == "anthropic") {
            parsed_response = parse_anthropic_response(json_response);
        } else if (_config.response_parser == "gemini") {
            parsed_response = parse_gemini_response(json_response);
        } else {
            parsed_response = parse_generic_response(json_response);
        }

        // Cache successful response
        _request_cache.put(cache_key, parsed_response);

        _update_stats(true, response_time, false);

        DebugLogger::log_performance("API request", response_time);

        if (callback) callback(parsed_response);

    } catch (const std::exception& e) {
        auto end_time = std::chrono::steady_clock::now();
        double response_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

        _update_stats(false, response_time, false);

        DebugLogger::log_error("Request failed", e);

        if (callback) callback("Error: " + std::string(e.what()));
    }
}

std::string UnifiedAIClient::_execute_http_request(const std::string& url,
                                                 const thirdparty_compat::Headers& headers,
                                                 const std::string& body) {

    if (url.empty()) {
        throw NetworkError("Empty URL");
    }

    // For now, use a simple HTTP client
    // In full implementation, this would use the connection pool
    thirdparty_compat::Client client(url.c_str());
    client.set_connection_timeout(_config.connection_timeout_ms / 1000);
    client.set_read_timeout(_config.read_timeout_ms / 1000);

    auto response = client.Post(url.c_str(), headers, body, "application/json");

    if (!response) {
        throw NetworkError("HTTP request failed - no response");
    }

    if (response->status >= 400) {
        std::string error_msg = "HTTP error " + std::to_string(response->status);
        if (!response->body.empty()) {
            error_msg += ": " + response->body;
        }

        if (response->status == 429) {
            throw RateLimitError(error_msg);
        } else {
            throw APIError(error_msg);
        }
    }

    return response->body;
}

std::string UnifiedAIClient::_generate_cache_key(const std::string& prompt,
                                               const std::string& model_name,
                                               double temperature) const {
    // Create a simple hash of the request parameters
    std::string key = _config.name + ":" + model_name + ":" + prompt + ":" + std::to_string(temperature);

    // Simple hash function (in production, use proper MD5)
    size_t hash = 0;
    for (char c : key) {
        hash = hash * 31 + static_cast<size_t>(c);
    }

    return std::to_string(hash);
}

void UnifiedAIClient::_update_stats(bool success, double response_time_ms, bool was_cached) const {
    std::lock_guard<thirdparty_compat::mutex> lock(_stats_mutex);

    _stats.total_requests++;
    if (was_cached) _stats.cached_requests++;
    if (!success) _stats.failed_requests++;

    // Update average response time
    if (!was_cached) {
        double total_time = _stats.average_response_time_ms * (_stats.total_requests - _stats.cached_requests - 1);
        total_time += response_time_ms;
        _stats.average_response_time_ms = total_time / (_stats.total_requests - _stats.cached_requests);
    }

    _stats.last_request_time = std::chrono::steady_clock::now();
}

bool UnifiedAIClient::_validate_api_key(const std::string& api_key) const {
    if (api_key.empty()) return false;

    // Provider-specific validation
    if (_config.name == "openai" && !api_key.starts_with("sk-")) return false;
    if (_config.name == "anthropic" && !api_key.starts_with("sk-ant-api03-")) return false;

    return api_key.length() >= 20;  // Minimum reasonable length
}

std::string UnifiedAIClient::_get_model_name() const {
    // This would integrate with settings in full implementation
    return _config.default_model;
}

void UnifiedAIClient::_worker_thread_main() {
    // Worker thread implementation for async processing
    // For now, this is a placeholder - requests are processed synchronously
    while (!_should_stop) {
        std::unique_lock<thirdparty_compat::mutex> lock(_worker_mutex);
        _worker_cv.wait_for(lock, thirdparty_compat::seconds(1));

        if (_should_stop) break;
    }
}

// Payload builders for different formats
nlohmann::json UnifiedAIClient::build_openai_payload(const std::string& prompt, double temperature) const {
    return {
        {"model", _get_model_name()},
        {"messages", {{
            {{"role", "user"}, {"content", prompt}}
        }}},
        {"temperature", temperature},
        {"max_tokens", 1000}
    };
}

nlohmann::json UnifiedAIClient::build_anthropic_payload(const std::string& prompt, double temperature) const {
    return {
        {"model", _get_model_name()},
        {"messages", {{
            {{"role", "user"}, {"content", prompt}}
        }}},
        {"temperature", temperature},
        {"max_tokens", 1000}
    };
}

nlohmann::json UnifiedAIClient::build_gemini_payload(const std::string& prompt, double temperature) const {
    return {
        {"contents", {{
            {{"parts", {{
                {{"text", prompt}}
            }}}}
        }}},
        {"generationConfig", {
            {"temperature", temperature},
            {"maxOutputTokens", 1000}
        }}
    };
}

nlohmann::json UnifiedAIClient::build_generic_payload(const std::string& prompt, double temperature) const {
    return {
        {"prompt", prompt},
        {"temperature", temperature},
        {"max_tokens", 1000}
    };
}

// Response parsers for different formats
std::string UnifiedAIClient::parse_openai_response(const nlohmann::json& response) const {
    if (response.contains("choices") && !response["choices"].empty()) {
        auto& choice = response["choices"][0];
        if (choice.contains("message") && choice["message"].contains("content")) {
            return choice["message"]["content"];
        }
    }
    return "Error: Unexpected response format";
}

std::string UnifiedAIClient::parse_anthropic_response(const nlohmann::json& response) const {
    if (response.contains("content") && !response["content"].empty()) {
        return response["content"][0]["text"];
    }
    return "Error: Unexpected response format";
}

std::string UnifiedAIClient::parse_gemini_response(const nlohmann::json& response) const {
    if (response.contains("candidates") && !response["candidates"].empty()) {
        auto& candidate = response["candidates"][0];
        if (candidate.contains("content") && candidate["content"].contains("parts")) {
            return candidate["content"]["parts"][0]["text"];
        }
    }
    return "Error: Unexpected response format";
}

std::string UnifiedAIClient::parse_generic_response(const nlohmann::json& response) const {
    if (response.contains("text")) {
        return response["text"];
    }
    if (response.contains("response")) {
        return response["response"];
    }
    return response.dump();  // Fallback: return raw JSON
}

// Factory function
std::unique_ptr<UnifiedAIClient> create_unified_client(
    const std::string& provider_name,
    ConnectionPool& connection_pool,
    RequestCache& request_cache) {

    return std::make_unique<UnifiedAIClient>(provider_name, connection_pool, request_cache);
}
