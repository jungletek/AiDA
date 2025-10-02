#include "ai_provider_config.hpp"
#include "debug_logger.hpp"

// Static member initialization
std::vector<AIProviderConfig> ProviderConfigs::_builtin_providers;
std::map<std::string, AIProviderConfig> ProviderConfigs::_custom_providers;

// Initialize built-in provider configurations
void ProviderConfigs::initialize_builtin_providers() {
    if (!_builtin_providers.empty()) {
        return; // Already initialized
    }

    // OpenAI Configuration
    _builtin_providers.push_back({
        "openai", "OpenAI GPT-4",
        "https://api.openai.com",
        "/v1/chat/completions",
        "Authorization", "Bearer ",
        "openai", "openai",
        {"gpt-5", "gpt-5-mini", "gpt-5-nano", "o3-pro", "o3", "o3-mini", "o1-pro", "o1", "o4-mini", "gpt-4.5-preview", "gpt-4.1", "gpt-4.1-mini", "gpt-4.1-nano", "gpt-4o", "gpt-4-turbo", "gpt-4", "gpt-4o-mini", "gpt-3.5-turbo", "gpt-3.5-turbo-16k"},
        "gpt-5",
        {{"Content-Type", "application/json"}},
        {},
        60,  // 60 RPM
        true, true, true, false,
        10000, 60000,
        0.03, 0.06  // $0.03/1K input, $0.06/1K output
    });

    // Google Gemini Configuration
    _builtin_providers.push_back({
        "gemini", "Google Gemini",
        "https://generativelanguage.googleapis.com",
        "/v1beta/models/{model}:generateContent",
        "x-goog-api-key", "",
        "gemini", "gemini",
        {"gemini-2.5-pro", "gemini-2.5-flash", "gemini-2.5-flash-lite", "gemini-2.0-flash", "gemini-2.0-flash-lite", "gemini-1.5-pro-latest", "gemini-1.5-pro", "gemini-1.5-pro-002", "gemini-1.5-flash-latest", "gemini-1.5-flash", "gemini-1.5-flash-8b", "gemini-1.5-flash-8b-latest"},
        "gemini-2.0-flash",
        {{"Content-Type", "application/json"}},
        {},
        60,  // 60 RPM
        true, true, true, false,
        10000, 60000,
        0.00125, 0.005  // $0.00125/1K input, $0.005/1K output
    });

    // OpenRouter Configuration (uses OpenAI format)
    _builtin_providers.push_back({
        "openrouter", "OpenRouter",
        "https://openrouter.ai",
        "/api/v1/chat/completions",
        "Authorization", "Bearer ",
        "openai", "openai",
        {"moonshotai/kimi-k2:free", "openai/gpt-oss-20b:free", "z-ai/glm-4.5-air:free", "tngtech/deepseek-r1t2-chimera:free"},
        "moonshotai/kimi-k2:free",
        {{"Content-Type", "application/json"}, {"HTTP-Referer", "https://github.com/jungletek/AiDA"}},
        {},
        60,  // 60 RPM
        true, false, true, false,
        10000, 60000,
        0.0, 0.0  // Free tier
    });

    // Anthropic Configuration
    _builtin_providers.push_back({
        "anthropic", "Anthropic Claude",
        "https://api.anthropic.com",
        "/v1/messages",
        "x-api-key", "",
        "anthropic", "anthropic",
        {"claude-opus-4-0", "claude-sonnet-4-0", "claude-3.5-sonnet-latest", "claude-3.5-haiku-latest", "claude-3-opus-latest", "claude-3-sonnet-latest", "claude-3-haiku-latest", "claude-2.1", "claude-2", "claude-instant-v1.2"},
        "claude-3.5-sonnet-latest",
        {{"Content-Type", "application/json"}, {"anthropic-version", "2023-06-01"}},
        {},
        50,  // 50 RPM (Anthropic's limit)
        true, true, true, false,
        10000, 60000,
        0.015, 0.075  // $0.015/1K input, $0.075/1K output
    });

    // Copilot Configuration (uses OpenAI format)
    _builtin_providers.push_back({
        "copilot", "GitHub Copilot",
        "http://127.0.0.1:4141",  // Default local proxy
        "/v1/chat/completions",
        "Authorization", "Bearer ",
        "openai", "openai",
        {"claude-sonnet-4", "claude-3.7-sonnet-thought", "gemini-2.5-pro", "claude-3.7-sonnet", "gpt-4.1-2025-04-14", "gpt-4.1", "o4-mini-2025-04-16", "o4-mini", "o3-mini-2025-01-31", "o3-mini", "o3-mini-paygo", "claude-3.5-sonnet", "gemini-2.0-flash-001", "gpt-4o-2024-11-20", "gpt-4o-2024-08-06", "gpt-4o-2024-05-13", "gpt-4o", "gpt-4o-copilot", "gpt-4-o-preview", "gpt-4-0125-preview", "gpt-4", "gpt-4-0613", "gpt-4o-mini-2024-07-18", "gpt-4o-mini", "gpt-3.5-turbo", "gpt-3.5-turbo-0613"},
        "gpt-4.1",
        {{"Content-Type", "application/json"}},
        {},
        1000,  // High limit for local proxy
        true, false, true, false,
        5000, 30000,
        0.0, 0.0  // Free for Copilot users
    });

    // DeepSeek Configuration
    _builtin_providers.push_back({
        "deepseek", "DeepSeek",
        "https://api.deepseek.com",
        "/v1/chat/completions",
        "Authorization", "Bearer ",
        "openai", "openai",
        {"deepseek-chat", "deepseek-reasoner", "deepseek-coder", "deepseek-v2", "deepseek-v2-lite", "deepseek-llm-67b-chat", "deepseek-llm-7b-chat"},
        "deepseek-chat",
        {{"Content-Type", "application/json"}},
        {},
        60,  // 60 RPM
        true, false, true, false,
        10000, 60000,
        0.00014, 0.00028  // $0.00014/1K input, $0.00028/1K output
    });
}

// Get all built-in provider configurations
std::vector<AIProviderConfig> ProviderConfigs::get_all_providers() {
    initialize_builtin_providers();
    std::vector<AIProviderConfig> all_providers = _builtin_providers;

    // Add custom providers
    for (const auto& pair : _custom_providers) {
        all_providers.push_back(pair.second);
    }

    return all_providers;
}

// Get configuration for specific provider
AIProviderConfig ProviderConfigs::get_provider_config(const std::string& provider_name) {
    initialize_builtin_providers();

    // Check custom providers first
    auto custom_it = _custom_providers.find(provider_name);
    if (custom_it != _custom_providers.end()) {
        return custom_it->second;
    }

    // Check built-in providers
    for (const auto& config : _builtin_providers) {
        if (config.name == provider_name) {
            return config;
        }
    }

    // Return empty config if not found
    return AIProviderConfig{};
}

// Register custom provider configuration
void ProviderConfigs::register_custom_provider(const AIProviderConfig& config) {
    _custom_providers[config.name] = config;
    DebugLogger::log_api_call("ProviderConfigs", "register_custom_provider: " + config.name);
}

// Create configuration from JSON
AIProviderConfig AIProviderConfig::from_json(const nlohmann::json& j) {
    AIProviderConfig config;

    config.name = j.value("name", "");
    config.display_name = j.value("display_name", "");
    config.host = j.value("host", "");
    config.path_template = j.value("path_template", "");
    config.auth_header_name = j.value("auth_header_name", "");
    config.auth_prefix = j.value("auth_prefix", "");
    config.request_format = j.value("request_format", "");
    config.response_parser = j.value("response_parser", "");
    config.default_model = j.value("default_model", "");

    // Parse arrays
    if (j.contains("available_models") && j["available_models"].is_array()) {
        for (const auto& model : j["available_models"]) {
            if (model.is_string()) {
                config.available_models.push_back(model.get<std::string>());
            }
        }
    }

    // Parse maps
    if (j.contains("default_headers") && j["default_headers"].is_object()) {
        for (const auto& [key, value] : j["default_headers"].items()) {
            if (value.is_string()) {
                config.default_headers[key] = value.get<std::string>();
            }
        }
    }

    if (j.contains("request_transforms") && j["request_transforms"].is_object()) {
        for (const auto& [key, value] : j["request_transforms"].items()) {
            if (value.is_string()) {
                config.request_transforms[key] = value.get<std::string>();
            }
        }
    }

    // Parse numeric values
    config.rate_limit_rpm = j.value("rate_limit_rpm", 60);
    config.connection_timeout_ms = j.value("connection_timeout_ms", 10000);
    config.read_timeout_ms = j.value("read_timeout_ms", 60000);
    config.input_token_cost_usd = j.value("input_token_cost_usd", 0.0);
    config.output_token_cost_usd = j.value("output_token_cost_usd", 0.0);

    // Parse boolean values
    config.supports_function_calling = j.value("supports_function_calling", false);
    config.supports_vision = j.value("supports_vision", false);
    config.supports_system_messages = j.value("supports_system_messages", true);
    config.supports_streaming = j.value("supports_streaming", false);

    return config;
}

// Convert configuration to JSON for serialization
nlohmann::json AIProviderConfig::to_json() const {
    nlohmann::json j;

    j["name"] = name;
    j["display_name"] = display_name;
    j["host"] = host;
    j["path_template"] = path_template;
    j["auth_header_name"] = auth_header_name;
    j["auth_prefix"] = auth_prefix;
    j["request_format"] = request_format;
    j["response_parser"] = response_parser;
    j["default_model"] = default_model;

    // Convert arrays
    j["available_models"] = available_models;

    // Convert maps
    j["default_headers"] = default_headers;
    j["request_transforms"] = request_transforms;

    // Numeric values
    j["rate_limit_rpm"] = rate_limit_rpm;
    j["connection_timeout_ms"] = connection_timeout_ms;
    j["read_timeout_ms"] = read_timeout_ms;
    j["input_token_cost_usd"] = input_token_cost_usd;
    j["output_token_cost_usd"] = output_token_cost_usd;

    // Boolean values
    j["supports_function_calling"] = supports_function_calling;
    j["supports_vision"] = supports_vision;
    j["supports_system_messages"] = supports_system_messages;
    j["supports_streaming"] = supports_streaming;

    return j;
}

// Get authentication header value for API key
std::string AIProviderConfig::get_auth_header(const std::string& api_key) const {
    return auth_header_name + ": " + auth_prefix + api_key;
}

// Get complete URL for a request
std::string AIProviderConfig::get_request_url(const std::string& model_name) const {
    std::string url = host;

    // Replace {model} placeholder in path template
    std::string path = path_template;
    size_t pos = path.find("{model}");
    if (pos != std::string::npos) {
        std::string model = model_name.empty() ? default_model : model_name;
        path.replace(pos, 7, model);
    }

    // Ensure we have a proper URL
    if (!url.empty() && url.back() != '/' && !path.empty() && path.front() != '/') {
        url += "/";
    }
    url += path;

    return url;
}

// Validate configuration
bool AIProviderConfig::is_valid() const {
    return !name.empty() &&
           !host.empty() &&
           !auth_header_name.empty() &&
           !request_format.empty() &&
           !response_parser.empty() &&
           !available_models.empty() &&
           !default_model.empty();
}

// Provider configuration templates
namespace ProviderTemplates {

    AIProviderConfig openai_format(
        const std::string& name,
        const std::string& host,
        const std::string& path_template) {

        return {
            name, name, host, path_template,
            "Authorization", "Bearer ",
            "openai", "openai",
            {}, // Models to be filled in
            "",
            {{"Content-Type", "application/json"}},
            {},
            60, true, false, true, false,
            10000, 60000,
            0.0, 0.0
        };
    }

    AIProviderConfig anthropic_format(
        const std::string& name,
        const std::string& host) {

        return {
            name, name, host, "/v1/messages",
            "x-api-key", "",
            "anthropic", "anthropic",
            {}, // Models to be filled in
            "",
            {{"Content-Type", "application/json"}, {"anthropic-version", "2023-06-01"}},
            {},
            50, true, true, true, false,
            10000, 60000,
            0.0, 0.0
        };
    }

    AIProviderConfig gemini_format(
        const std::string& name,
        const std::string& host) {

        return {
            name, name, host, "/v1beta/models/{model}:generateContent",
            "x-goog-api-key", "",
            "gemini", "gemini",
            {}, // Models to be filled in
            "",
            {{"Content-Type", "application/json"}},
            {},
            60, true, true, true, false,
            10000, 60000,
            0.0, 0.0
        };
    }

    AIProviderConfig generic_format(
        const std::string& name,
        const std::string& host,
        const std::string& path_template) {

        return {
            name, name, host, path_template,
            "Authorization", "Bearer ",
            "generic", "generic",
            {}, // Models to be filled in
            "",
            {{"Content-Type", "application/json"}},
            {},
            60, false, false, true, false,
            10000, 60000,
            0.0, 0.0
        };
    }
}
