#pragma once

#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

// Configuration-driven AI provider definitions
// This replaces the need for 6 different AI client classes

struct AIProviderConfig {
    // Basic provider identification
    std::string name;                    // "openai", "gemini", "anthropic", etc.
    std::string display_name;            // "OpenAI GPT-4", "Google Gemini Pro", etc.
    std::string host;                    // Base URL for API requests
    std::string path_template;           // URL path template (e.g., "/v1/chat/completions")

    // Authentication configuration
    std::string auth_header_name;        // "Authorization", "X-API-Key", etc.
    std::string auth_prefix;             // "Bearer ", "sk-", etc.

    // Request/Response format configuration
    std::string request_format;          // "openai", "anthropic", "gemini"
    std::string response_parser;         // "openai", "anthropic", "gemini"

    // Model configuration
    std::vector<std::string> available_models;
    std::string default_model;

    // Request customization
    std::map<std::string, std::string> default_headers;
    std::map<std::string, std::string> request_transforms;

    // Rate limiting (requests per minute)
    int rate_limit_rpm = 60;

    // Provider-specific settings
    bool supports_function_calling = false;
    bool supports_vision = false;
    bool supports_system_messages = true;
    bool supports_streaming = false;

    // Timeout settings (milliseconds)
    int connection_timeout_ms = 10000;
    int read_timeout_ms = 60000;

    // Cost/pricing information (optional)
    double input_token_cost_usd = 0.0;   // Cost per 1K input tokens
    double output_token_cost_usd = 0.0;  // Cost per 1K output tokens

    // Create configuration from JSON
    static AIProviderConfig from_json(const nlohmann::json& j);

    // Convert configuration to JSON for serialization
    nlohmann::json to_json() const;

    // Get authentication header value for API key
    std::string get_auth_header(const std::string& api_key) const;

    // Get complete URL for a request
    std::string get_request_url(const std::string& model_name = "") const;

    // Validate configuration
    bool is_valid() const;
};

// Built-in provider configurations
class ProviderConfigs {
public:
    // Get all built-in provider configurations
    static std::vector<AIProviderConfig> get_all_providers();

    // Get configuration for specific provider
    static AIProviderConfig get_provider_config(const std::string& provider_name);

    // Register custom provider configuration
    static void register_custom_provider(const AIProviderConfig& config);

private:
    static std::vector<AIProviderConfig> _builtin_providers;
    static std::map<std::string, AIProviderConfig> _custom_providers;

    // Initialize built-in provider configurations
    static void initialize_builtin_providers();
};

// Provider configuration templates for different API formats
namespace ProviderTemplates {

    // OpenAI-compatible format (OpenAI, OpenRouter, DeepSeek, Copilot)
    AIProviderConfig openai_format(
        const std::string& name,
        const std::string& host,
        const std::string& path_template = "/v1/chat/completions"
    );

    // Anthropic format
    AIProviderConfig anthropic_format(
        const std::string& name,
        const std::string& host = "https://api.anthropic.com"
    );

    // Google Gemini format
    AIProviderConfig gemini_format(
        const std::string& name,
        const std::string& host = "https://generativelanguage.googleapis.com"
    );

    // Generic REST API format
    AIProviderConfig generic_format(
        const std::string& name,
        const std::string& host,
        const std::string& path_template
    );
}
