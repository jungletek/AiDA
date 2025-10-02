#include "settings_form_manager.hpp"
#include "settings.hpp"
#include "debug_logger.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>

// Static helper function implementations
std::string SettingsFormManager::generate_form_string() {
    // This would contain the form definition string for IDA SDK forms
    // For now, return a placeholder
    return "AiDA Settings Form";
}

qstrvec_t SettingsFormManager::create_provider_list() {
    qstrvec_t providers;
    providers.push_back("Gemini");
    providers.push_back("OpenAI");
    providers.push_back("OpenRouter");
    providers.push_back("Anthropic");
    providers.push_back("Copilot");
    providers.push_back("DeepSeek");
    return providers;
}

qstrvec_t SettingsFormManager::create_model_list(const std::vector<std::string>& models) {
    qstrvec_t qmodels;
    for (const auto& model : models) {
        qmodels.push_back(model.c_str());
    }
    return qmodels;
}

std::string SettingsFormManager::format_double_for_form(double value) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << value;
    return ss.str();
}

double SettingsFormManager::parse_double_from_form(const std::string& value, double default_value) {
    try {
        return std::stod(value);
    } catch (const std::exception&) {
        return default_value;
    }
}

int SettingsFormManager::provider_name_to_index(const std::string& provider_name) {
    std::string lower = provider_name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "gemini") return static_cast<int>(AiDAConstants::ProviderIndex::GEMINI);
    if (lower == "openai") return static_cast<int>(AiDAConstants::ProviderIndex::OPENAI);
    if (lower == "openrouter") return static_cast<int>(AiDAConstants::ProviderIndex::OPENROUTER);
    if (lower == "anthropic") return static_cast<int>(AiDAConstants::ProviderIndex::ANTHROPIC);
    if (lower == "copilot") return static_cast<int>(AiDAConstants::ProviderIndex::COPILOT);
    if (lower == "deepseek") return static_cast<int>(AiDAConstants::ProviderIndex::DEEPSEEK);

    return 0; // Default to Gemini
}

std::string SettingsFormManager::provider_index_to_name(int index) {
    switch (static_cast<AiDAConstants::ProviderIndex>(index)) {
        case AiDAConstants::ProviderIndex::GEMINI: return "gemini";
        case AiDAConstants::ProviderIndex::OPENAI: return "openai";
        case AiDAConstants::ProviderIndex::OPENROUTER: return "openrouter";
        case AiDAConstants::ProviderIndex::ANTHROPIC: return "anthropic";
        case AiDAConstants::ProviderIndex::COPILOT: return "copilot";
        case AiDAConstants::ProviderIndex::DEEPSEEK: return "deepseek";
        default: return "gemini";
    }
}

// Main form display function (placeholder implementation)
bool SettingsFormManager::show_settings_dialog(SettingsFormData& data) {
    // This would implement the actual IDA SDK form display
    // For now, return true as placeholder
    DebugLogger::log_api_call("SettingsForm", "show_settings_dialog");
    return true;
}

// Load form data from settings
bool SettingsFormManager::load_form_data_from_settings(const settings_t& settings, SettingsFormData& data) {
    try {
        data.provider = settings.api_provider;
        data.provider_index = provider_name_to_index(settings.api_provider);

        // General settings
        data.xref_context_count = settings.xref_context_count;
        data.xref_analysis_depth = settings.xref_analysis_depth;
        data.xref_code_snippet_lines = settings.xref_code_snippet_lines;
        data.bulk_processing_delay = settings.bulk_processing_delay;
        data.max_prompt_tokens = settings.max_prompt_tokens;
        data.temperature = settings.temperature;

        // Provider-specific settings
        data.gemini_api_key = settings.gemini_api_key;
        data.gemini_model_name = settings.gemini_model_name;

        data.openai_api_key = settings.openai_api_key;
        data.openai_model_name = settings.openai_model_name;

        data.openrouter_api_key = settings.openrouter_api_key;
        data.openrouter_model_name = settings.openrouter_model_name;

        data.anthropic_api_key = settings.anthropic_api_key;
        data.anthropic_model_name = settings.anthropic_model_name;

        data.copilot_proxy_address = settings.copilot_proxy_address;
        data.copilot_model_name = settings.copilot_model_name;

        data.deepseek_api_key = settings.deepseek_api_key;
        data.deepseek_model_name = settings.deepseek_model_name;

        return true;
    } catch (const std::exception& e) {
        DebugLogger::log_error("load_form_data_from_settings", e);
        return false;
    }
}

// Apply form data to settings
bool SettingsFormManager::apply_form_data_to_settings(const SettingsFormData& data, settings_t& settings) {
    try {
        settings.api_provider = data.provider;

        // General settings
        settings.xref_context_count = data.xref_context_count;
        settings.xref_analysis_depth = data.xref_analysis_depth;
        settings.xref_code_snippet_lines = data.xref_code_snippet_lines;
        settings.bulk_processing_delay = data.bulk_processing_delay;
        settings.max_prompt_tokens = data.max_prompt_tokens;
        settings.temperature = data.temperature;

        // Provider-specific settings
        settings.gemini_api_key = data.gemini_api_key;
        settings.gemini_model_name = data.gemini_model_name;

        settings.openai_api_key = data.openai_api_key;
        settings.openai_model_name = data.openai_model_name;

        settings.openrouter_api_key = data.openrouter_api_key;
        settings.openrouter_model_name = data.openrouter_model_name;

        settings.anthropic_api_key = data.anthropic_api_key;
        settings.anthropic_model_name = data.anthropic_model_name;

        settings.copilot_proxy_address = data.copilot_proxy_address;
        settings.copilot_model_name = data.copilot_model_name;

        settings.deepseek_api_key = data.deepseek_api_key;
        settings.deepseek_model_name = data.deepseek_model_name;

        return true;
    } catch (const std::exception& e) {
        DebugLogger::log_error("apply_form_data_to_settings", e);
        return false;
    }
}

// Validate form data
bool SettingsFormManager::validate_form_data(const SettingsFormData& data) {
    // Basic validation
    if (data.provider.empty()) {
        return false;
    }

    // Provider-specific validation
    if (data.provider == "gemini" && data.gemini_api_key.empty()) {
        return false;
    }
    if (data.provider == "openai" && data.openai_api_key.empty()) {
        return false;
    }
    if (data.provider == "openrouter" && data.openrouter_api_key.empty()) {
        return false;
    }
    if (data.provider == "anthropic" && data.anthropic_api_key.empty()) {
        return false;
    }
    if (data.provider == "copilot" && data.copilot_proxy_address.empty()) {
        return false;
    }
    if (data.provider == "deepseek" && data.deepseek_api_key.empty()) {
        return false;
    }

    // Range validation
    if (data.xref_context_count < 1 || data.xref_context_count > 20) {
        return false;
    }
    if (data.xref_analysis_depth < 1 || data.xref_analysis_depth > 10) {
        return false;
    }
    if (data.temperature < 0.0 || data.temperature > 2.0) {
        return false;
    }

    return true;
}

// Get available models for a provider
std::vector<std::string> SettingsFormManager::get_available_models(const std::string& provider, const std::string& api_key) {
    if (provider == "gemini") {
        return settings_t::gemini_models;
    }
    if (provider == "openai") {
        return settings_t::openai_models;
    }
    if (provider == "openrouter") {
        return settings_t::openrouter_models;
    }
    if (provider == "anthropic") {
        return settings_t::anthropic_models;
    }
    if (provider == "copilot") {
        return settings_t::copilot_models;
    }
    if (provider == "deepseek") {
        return settings_t::deepseek_models;
    }

    return {};
}

// Find model index in the models list
int SettingsFormManager::find_model_index(const std::vector<std::string>& models, const std::string& current_model) {
    for (size_t i = 0; i < models.size(); ++i) {
        if (models[i] == current_model) {
            return static_cast<int>(i);
        }
    }
    return 0; // Default to first model
}

// Test connection (placeholder implementation)
AiDAConstants::ConnectionTestResult SettingsFormManager::test_connection(const SettingsFormData& data) {
    AiDAConstants::ConnectionTestResult result;
    result.success = false;
    result.message = "Connection test not implemented";
    result.response_time_ms = 0;
    return result;
}

// Fetch OpenRouter models via API (placeholder implementation)
std::vector<std::string> SettingsFormManager::fetch_openrouter_models_via_api(const qstring& api_key) {
    // This would implement actual API call to fetch OpenRouter models
    // For now, return the static list
    return settings_t::openrouter_models;
}
