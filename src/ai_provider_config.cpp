#include "ai_provider_config.hpp"
#include "string_utils.hpp"
#include <algorithm>

// Initialize static members
std::vector<AIProviderConfig> ProviderConfigs::_builtin_providers;
std::map<std::string, AIProviderConfig> ProviderConfigs::_custom_providers;

// AIProviderConfig implementation
AIProviderConfig AIProviderConfig::from_json(const nlohmann::json& j) {
    AIProviderConfig config;

    config.name = j.value("name", "");
    config.display_name = j.value("display_name", config.name);
    config.host = j.value("host", "");
    config.path_template = j.value("path_template", "");

    config.auth_header_name = j.value("auth_header_name", "Authorization");
    config.auth_prefix = j.value("auth_prefix", "");

    config.request_format = j.value("request_format", "openai");
    config.response_parser = j.value("response_parser", "openai");

    if (j.contains("available_models")) {
        config.available_models = j["available_models"].get<std::vector<std::string>>();
    }
    config.default_model = j.value("default_model", "");

    if (j.contains("default_headers")) {
        config.default_headers = j["default_headers"].get<std::map<std::string, std::string>>();
    }

    config.rate_limit_rpm = j.value("rate_limit_rpm", 60);

    config.supports_function_calling = j.value("supports_function_calling", false);
    config.supports_vision = j.value("supports_vision", false);
    config.supports_system_messages = j.value("supports_system_messages", true);
    config.supports_streaming = j.value("supports_streaming", false);

    config.connection_timeout_ms = j.value("connection_timeout_ms", 10000);
    config.read_timeout_ms = j.value("read_timeout_ms", 60000);

    config.input_token_cost_usd = j.value("input_token_cost_usd", 0.0);
    config.output_token_cost_usd = j.value("output_token_cost_usd", 0.0);

    return config;
}

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

    j["available_models"] = available_models;
    j["default_model"] = default_model;

    j["default_headers"] = default_headers;

    j["rate_limit_rpm"] = rate_limit_rpm;

    j["supports_function_calling"] = supports_function_calling;
    j["supports_vision"] = supports_vision;
    j["supports_system_messages"] = supports_system_messages;
    j["supports_streaming"] = supports_streaming;

    j["connection_timeout_ms"] = connection_timeout_ms;
    j["read_timeout_ms"] = read_timeout_ms;

    j["input_token_cost_usd"] = input_token_cost_usd;
    j["output_token_cost_usd"] = output_token_cost_usd;

    return j;
}

std::string AIProviderConfig::get_auth_header(const std::string& api_key) const {
    if (api_key.empty()) {
        return "";
    }

    if (auth_prefix.empty()) {
        return api_key;
    }

    return auth_prefix + api_key;
}

std::string AIProviderConfig::get_request_url(const std::string& model_name) const {
    std::string url = host;

    // Replace model placeholder in path if present
    std::string path = path_template;
    if (path.find("{model}") != std::string::npos && !model_name.empty()) {
        size_t pos = path.find("{model}");
        path.replace(pos, 7, model_name);
    }

    // Ensure proper URL construction
    if (!path.empty()) {
        if (url.back() != '/' && path.front() != '/') {
            url += "/";
        }
        url += path;
    }

    return url;
}

bool AIProviderConfig::is_valid() const {
    return !name.empty() &&
           !host.empty() &&
           !auth_header_name.empty() &&
           !request_format.empty() &&
           !response_parser.empty();
}

// ProviderConfigs implementation
std::vector<AIProviderConfig> ProviderConfigs::get_all_providers() {
    if (_builtin_providers.empty()) {
        initialize_builtin_providers();
    }

    std::vector<AIProviderConfig> all_providers = _builtin_providers;

    // Add custom providers
    for (const auto& pair : _custom_providers) {
        all_providers.push_back(pair.second);
    }

    return all_providers;
}

AIProviderConfig ProviderConfigs::get_provider_config(const std::string& provider_name) {
    if (_builtin_providers.empty()) {
        initialize_builtin_providers();
    }

    // Check custom providers first
    auto custom_it = _custom_providers.find(provider_name);
    if (custom_it != _custom_providers.end()) {
        return custom_it->second;
    }

    // Check built-in providers
    auto it = std::find_if(_builtin_providers.begin(), _builtin_providers.end(),
        [&provider_name](const AIProviderConfig& config) {
            return config.name == provider_name;
        });

    if (it != _builtin_providers.end()) {
        return *it;
    }

    // Return empty config if not found
    return AIProviderConfig{};
}

void ProviderConfigs::register_custom_provider(const AIProviderConfig& config) {
    if (config.is_valid()) {
        _custom_providers[config.name] = config;
    }
}

void ProviderConfigs::initialize_builtin_providers() {
    _builtin_providers = {

        // OpenAI Configuration
        {
            "openai", "OpenAI GPT-4",
            "https://api.openai.com",
            "/v1/chat/completions",
            "Authorization", "Bearer ",
            "openai", "openai",
            {"gpt-4", "gpt-4-turbo", "gpt-3.5-turbo"},
            "gpt-4",
            {{"Content-Type", "application/json"}},
            {},
            60,  // 60 RPM
            true, true, true, false,
            10000, 60000,
            0.03, 0.06  // $0.03/1K input, $0.06/1K output
        },

        // Google Gemini Configuration
        {
            "gemini", "Google Gemini Pro",
            "https://generativelanguage.googleapis.com",
            "/v1beta/models/{model}:generateContent",
            "X-API-Key", "",
            "gemini", "gemini",
            {"gemini-pro", "gemini-pro-vision"},
            "gemini-pro",
            {{"Content-Type", "application/json"}},
            {},
            60,
            true, true, true, false,
            10000, 60000,
            0.0005, 0.0015  // $0.0005/1K input, $0.0015/1K output
        },

        // OpenRouter Configuration (OpenAI-compatible)
        {
            "openrouter", "OpenRouter",
            "https://openrouter.ai",
            "/api/v1/chat/completions",
            "Authorization", "Bearer ",
            "openai", "openai",
            {"gpt-4", "gpt-3.5-turbo", "claude-3-opus", "claude-3-sonnet"},
            "gpt-4",
            {{"Content-Type", "application/json"}, {"HTTP-Referer", "https://github.com/jungletek/AiDA"}},
            {},
            100,  // Higher limit for OpenRouter
            true, true, true, false,
            15000, 90000
        },

        // Anthropic Claude Configuration
        {
            "anthropic", "Anthropic Claude",
            "https://api.anthropic.com",
            "/v1/messages",
            "X-API-Key", "",
            "anthropic", "anthropic",
            {"claude-3-opus", "claude-3-sonnet", "claude-3-haiku"},
            "claude-3-sonnet",
            {{"Content-Type", "application/json"}, {"anthropic-version", "2023-06-01"}},
            {},
            50,  // Lower limit for Anthropic
            false, false, true, false,
            10000, 60000,
            0.015, 0.075  // $0.015/1K input, $0.075/1K output
        },

        // GitHub Copilot Configuration (via proxy)
        {
            "copilot", "GitHub Copilot",
            "",  // Empty host - uses proxy
            "/v1/chat/completions",
            "Authorization", "Bearer ",
            "openai", "openai",
            {"gpt-4", "gpt-3.5-turbo"},
            "gpt-4",
            {{"Content-Type", "application/json"}},
            {},
            100,
            true, false, true, false,
            10000, 60000
        },

        // DeepSeek Configuration
        {
            "deepseek", "DeepSeek Chat",
            "https://api.deepseek.com",
            "/v1/chat/completions",
            "Authorization", "Bearer ",
            "openai", "openai",
            {"deepseek-chat", "deepseek-coder"},
            "deepseek-chat",
            {{"Content-Type", "application/json"}},
            {},
            120,  // Higher limit for DeepSeek
            true, false, true, false,
            15000, 90000,
            0.00014, 0.00028  // Very affordable pricing
        }
    };
}

// ProviderTemplates implementation
namespace ProviderTemplates {

AIProviderConfig openai_format(
    const std::string& name,
    const std::string& host,
    const std::string& path_template
) {
    return {
        name, name,
        host, path_template,
        "Authorization", "Bearer ",
        "openai", "openai",
        {}, "",  // Models configured separately
        {{"Content-Type", "application/json"}},
        {},
        60,
        true, true, true, false,
        10000, 60000
    };
}

AIProviderConfig anthropic_format(
    const std::string& name,
    const std::string& host
) {
    return {
        name, name,
        host, "/v1/messages",
        "X-API-Key", "",
        "anthropic", "anthropic",
        {}, "",
        {{"Content-Type", "application/json"}, {"anthropic-version", "2023-06-01"}},
        {},
        50,
        false, false, true, false,
        10000, 60000
    };
}

AIProviderConfig gemini_format(
    const std::string& name,
    const std::string& host
) {
    return {
        name, name,
        host, "/v1beta/models/{model}:generateContent",
        "X-API-Key", "",
        "gemini", "gemini",
        {}, "",
        {{"Content-Type", "application/json"}},
        {},
        60,
        true, true, true, false,
        10000, 60000
    };
}

AIProviderConfig generic_format(
    const std::string& name,
    const std::string& host,
    const std::string& path_template
) {
    return {
        name, name,
        host, path_template,
        "Authorization", "Bearer ",
        "generic", "generic",
        {}, "",
        {{"Content-Type", "application/json"}},
        {},
        60,
        false, false, true, false,
        10000, 60000
    };
}

}
