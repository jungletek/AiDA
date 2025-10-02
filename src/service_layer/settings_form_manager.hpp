#pragma once

#include "constants.hpp"
// Forward declarations to avoid IDA SDK dependency
class aida_plugin_t;
#include <vector>
#include <string>

// Form data structure to hold all form values
struct SettingsFormData {
    // Provider selection
    std::string provider;
    int provider_index = 0;

    // General settings
    int xref_context_count = AiDAConstants::DEFAULT_XREF_CONTEXT_COUNT;
    int xref_analysis_depth = AiDAConstants::DEFAULT_XREF_ANALYSIS_DEPTH;
    int xref_code_snippet_lines = AiDAConstants::DEFAULT_XREF_CODE_SNIPPET_LINES;
    double bulk_processing_delay = AiDAConstants::DEFAULT_BULK_PROCESSING_DELAY;
    int max_prompt_tokens = AiDAConstants::DEFAULT_MAX_PROMPT_TOKENS;
    double temperature = AiDAConstants::DEFAULT_TEMPERATURE;

    // Provider-specific settings
    std::string gemini_api_key;
    std::string gemini_model_name;
    int gemini_model_index = 0;

    std::string openai_api_key;
    std::string openai_model_name;
    int openai_model_index = 0;

    std::string openrouter_api_key;
    std::string openrouter_model_name;
    int openrouter_model_index = 0;

    std::string anthropic_api_key;
    std::string anthropic_model_name;
    int anthropic_model_index = 0;

    std::string copilot_proxy_address;
    std::string copilot_model_name;
    int copilot_model_index = 0;

    std::string deepseek_api_key;
    std::string deepseek_model_name;
    int deepseek_model_index = 0;

    int selected_tab = 0;
};

// Settings form manager class to handle form operations
class SettingsFormManager {
public:
    // Main form display function
    static bool show_settings_dialog(SettingsFormData& data);

    // Data management functions
    static bool load_form_data_from_settings(const settings_t& settings, SettingsFormData& data);
    static bool apply_form_data_to_settings(const SettingsFormData& data, settings_t& settings);
    static bool validate_form_data(const SettingsFormData& data);

    // Provider-specific model handling
    static std::vector<std::string> get_available_models(const std::string& provider, const std::string& api_key = "");
    static int find_model_index(const std::vector<std::string>& models, const std::string& current_model);

    // Connection testing
    static AiDAConstants::ConnectionTestResult test_connection(const SettingsFormData& data);

    // OpenRouter model fetching (shared utility)
    static std::vector<std::string> fetch_openrouter_models_via_api(const qstring& api_key);

private:
    // Form string generation
    static std::string generate_form_string();

    // Provider model list creation
    static qstrvec_t create_provider_list();
    static qstrvec_t create_model_list(const std::vector<std::string>& models);

    // Data conversion helpers
    static std::string format_double_for_form(double value);
    static double parse_double_from_form(const std::string& value, double default_value);

    // Provider index conversion
    static int provider_name_to_index(const std::string& provider_name);
    static std::string provider_index_to_name(int index);
};
