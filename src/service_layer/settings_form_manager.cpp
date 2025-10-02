#include "settings_form_manager.hpp"
#include "string_utils.hpp"
#include "debug_logger.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <sstream>

// OpenRouter model fetching utility
std::vector<std::string> SettingsFormManager::fetch_openrouter_models_via_api(const qstring& api_key) {
    std::vector<std::string> models;
    if (api_key.empty()) {
        return models;
    }

    try {
        httplib::Client cli("https://openrouter.ai");
        std::string auth = string_utils::to_std(api_key);
        if (auth.find("Bearer ") != 0) {
            auth = "Bearer " + auth;
        }
        cli.set_default_headers({
            {"Authorization", auth},
        });
        cli.set_read_timeout(AiDAConstants::TEST_TIMEOUT_MS / 1000); // Convert to seconds
        cli.set_connection_timeout(AiDAConstants::CONNECTION_TIMEOUT_MS / 1000);

        auto res = cli.Get("/api/v1/models");
        if (!res || res->status != 200) {
            if (res) {
                DebugLogger::log_error("OpenRouter API fetch failed",
                    std::runtime_error("HTTP " + std::to_string(res->status)));
            } else {
                DebugLogger::log_error("OpenRouter API fetch failed",
                    std::runtime_error("HTTP request error"));
            }
            return models;
        }

        auto j = nlohmann::json::parse(res->body);
        if (!j.contains("data") || !j["data"].is_array()) {
            DebugLogger::log_error("OpenRouter API response invalid",
                std::runtime_error("Missing or invalid 'data' array"));
            return models;
        }

        auto is_probably_chat_model = [](const std::string& id) -> bool {
            // Conservative exclude list for non-chat models
            static const char* const excludes[] = {
                "embedding", "embeddings", "whisper", "audio", "tts", "dall-e", "image", "vision-preview", "stable-diffusion", "sd-"
            };
            for (const auto& ex : excludes) {
                if (id.find(ex) != std::string::npos) {
                    return false;
                }
            }
            return true; // default include
        };

        for (const auto& m : j["data"]) {
            if (!m.contains("id")) continue;
            std::string id = m["id"].get<std::string>();
            if (is_probably_chat_model(id)) {
                models.push_back(std::move(id));
            }
        }

        std::sort(models.begin(), models.end());
        models.erase(std::unique(models.begin(), models.end()), models.end());

        DebugLogger::log_api_call("OpenRouter", "models fetch");
        DebugLogger::log_info("Fetched " + std::to_string(models.size()) + " OpenRouter models");
    } catch (const std::exception& e) {
        DebugLogger::log_error("Exception while fetching OpenRouter models", e);
    }
    return models;
}

// Form string generation with constants
std::string SettingsFormManager::generate_form_string() {
    return
        "STARTITEM 0\n"
        "BUTTON YES Ok\n"
        "BUTTON NO \"Test Connection\"\n"
        "BUTTON CANCEL Cancel\n"
        "AI Assistant Settings\n\n"
        // --- general tab ---
        "<#API Provider Configuration#Provider:b1:0:20::>\n\n"
        "<#Analysis Parameters#XRef Context Count:D2:10:10::>\n"
        "<XRef Analysis Depth:D3:10:10::>\n"
        "<Code Snippet Lines:D4:10:10::>\n"
        "<Bulk Processing Delay (sec):q5:10:10::>\n"
        "<Max Prompt Tokens:D6:10:10::>\n"
        "<Model Temperature:q7:10:10::>\n"
        "<=:General>100>\n" // tab ctrl is 100

        // --- gemini ---
        "<API Key:q11:64:64::>\n"
        "<Model Name:b12:0:40::>\n"
        "<=:Gemini>100>\n"

        // --- openai ---
        "<API Key:q21:64:64::>\n"
        "<Model Name:b22:0:40::>\n"
        "<=:OpenAI>100>\n"

        // --- OpenRouter ---
        "<API Key:q25:80:80::>\n"
        "<Model Name:b26:0:40::>\n"
        "<=:OpenRouter>100>\n"

        // --- Anthropic Tab ---
        "<API Key:q31:64:64::>\n"
        "<Model Name:b32:0:40::>\n"
        "<=:Anthropic>100>\n"

        // --- copilot ---
        "<Proxy Address:q41:64:64::>\n"
        "<Model Name:b42:0:40::>\n"
        "<=:Copilot>100>\n"

        // --- deepseek ---
        "<API Key:q51:64:64::>\n"
        "<Model Name:b52:0:40::>\n"
        "<=:DeepSeek>100>\n";
}

// Provider list creation
qstrvec_t SettingsFormManager::create_provider_list() {
    static const char* const providers[] = { "Gemini", "OpenAI", "OpenRouter", "Anthropic", "Copilot", "DeepSeek" };
    qstrvec_t result;
    for (const auto& provider : providers) {
        result.push_back(provider);
    }
    return result;
}

// Model list creation
qstrvec_t SettingsFormManager::create_model_list(const std::vector<std::string>& models) {
    qstrvec_t result;
    for (const auto& model : models) {
        result.push_back(model.c_str());
    }
    return result;
}

// Provider index conversion
int SettingsFormManager::provider_name_to_index(const std::string& provider_name) {
    qstring provider_lower = string_utils::to_qstring(provider_name);
    qstrlwr(provider_lower.begin());

    if (provider_lower == "openai") return static_cast<int>(AiDAConstants::ProviderIndex::OPENAI);
    if (provider_lower == "openrouter") return static_cast<int>(AiDAConstants::ProviderIndex::OPENROUTER);
    if (provider_lower == "anthropic") return static_cast<int>(AiDAConstants::ProviderIndex::ANTHROPIC);
    if (provider_lower == "copilot") return static_cast<int>(AiDAConstants::ProviderIndex::COPILOT);
    if (provider_lower == "deepseek") return static_cast<int>(AiDAConstants::ProviderIndex::DEEPSEEK);

    return static_cast<int>(AiDAConstants::ProviderIndex::GEMINI); // Default to Gemini
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

// Data conversion helpers
std::string SettingsFormManager::format_double_for_form(double value) {
    qstring formatted;
    formatted.sprnt("%.2f", value);
    return formatted.c_str();
}

double SettingsFormManager::parse_double_from_form(const std::string& value, double default_value) {
    try {
        return std::stod(value);
    } catch (const std::exception&) {
        DebugLogger::log_error("Invalid double value in form",
            std::runtime_error("Value: " + value + ", using default: " + std::to_string(default_value)));
        return default_value;
    }
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
        data.gemini_model_index = find_model_index(settings_t::gemini_models, settings.gemini_model_name);

        data.openai_api_key = settings.openai_api_key;
        data.openai_model_name = settings.openai_model_name;
        data.openai_model_index = find_model_index(settings_t::openai_models, settings.openai_model_name);

        data.openrouter_api_key = settings.openrouter_api_key;
        data.openrouter_model_name = settings.openrouter_model_name;

        data.anthropic_api_key = settings.anthropic_api_key;
        data.anthropic_model_name = settings.anthropic_model_name;
        data.anthropic_model_index = find_model_index(settings_t::anthropic_models, settings.anthropic_model_name);

        data.copilot_proxy_address = settings.copilot_proxy_address;
        data.copilot_model_name = settings.copilot_model_name;
        data.copilot_model_index = find_model_index(settings_t::copilot_models, settings.copilot_model_name);

        data.deepseek_api_key = settings.deepseek_api_key;
        data.deepseek_model_name = settings.deepseek_model_name;
        data.deepseek_model_index = find_model_index(settings_t::deepseek_models, settings.deepseek_model_name);

        return true;
    } catch (const std::exception& e) {
        DebugLogger::log_error("Failed to load form data from settings", e);
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
        if (data.gemini_model_index < static_cast<int>(settings_t::gemini_models.size())) {
            settings.gemini_model_name = settings_t::gemini_models[data.gemini_model_index];
        }

        settings.openai_api_key = data.openai_api_key;
        if (data.openai_model_index < static_cast<int>(settings_t::openai_models.size())) {
            settings.openai_model_name = settings_t::openai_models[data.openai_model_index];
        }

        settings.openrouter_api_key = data.openrouter_api_key;
        auto openrouter_models = get_available_models("openrouter", data.openrouter_api_key);
        if (data.openrouter_model_index < static_cast<int>(openrouter_models.size())) {
            settings.openrouter_model_name = openrouter_models[data.openrouter_model_index];
        }

        settings.anthropic_api_key = data.anthropic_api_key;
        if (data.anthropic_model_index < static_cast<int>(settings_t::anthropic_models.size())) {
            settings.anthropic_model_name = settings_t::anthropic_models[data.anthropic_model_index];
        }

        settings.copilot_proxy_address = data.copilot_proxy_address;
        if (data.copilot_model_index < static_cast<int>(settings_t::copilot_models.size())) {
            settings.copilot_model_name = settings_t::copilot_models[data.copilot_model_index];
        }

        settings.deepseek_api_key = data.deepseek_api_key;
        if (data.deepseek_model_index < static_cast<int>(settings_t::deepseek_models.size())) {
            settings.deepseek_model_name = settings_t::deepseek_models[data.deepseek_model_index];
        }

        return true;
    } catch (const std::exception& e) {
        DebugLogger::log_error("Failed to apply form data to settings", e);
        return false;
    }
}

// Validate form data
bool SettingsFormManager::validate_form_data(const SettingsFormData& data) {
    if (data.provider.empty()) {
        warning("AI Assistant: Provider selection cannot be empty.");
        return false;
    }

    // Validate general settings
    if (data.xref_context_count <= 0 || data.xref_context_count > 100) {
        warning("AI Assistant: XRef context count must be between 1 and 100.");
        return false;
    }

    if (data.xref_analysis_depth <= 0 || data.xref_analysis_depth > 10) {
        warning("AI Assistant: XRef analysis depth must be between 1 and 10.");
        return false;
    }

    if (data.temperature < 0.0 || data.temperature > 2.0) {
        warning("AI Assistant: Temperature must be between 0.0 and 2.0.");
        return false;
    }

    // Provider-specific validation
    if (data.provider == "gemini" && data.gemini_api_key.empty()) {
        warning("AI Assistant: Gemini API key is required.");
        return false;
    }

    if (data.provider == "openai" && data.openai_api_key.empty()) {
        warning("AI Assistant: OpenAI API key is required.");
        return false;
    }

    if (data.provider == "openrouter" && data.openrouter_api_key.empty()) {
        warning("AI Assistant: OpenRouter API key is required.");
        return false;
    }

    if (data.provider == "anthropic" && data.anthropic_api_key.empty()) {
        warning("AI Assistant: Anthropic API key is required.");
        return false;
    }

    if (data.provider == "copilot" && data.copilot_proxy_address.empty()) {
        warning("AI Assistant: Copilot proxy address is required.");
        return false;
    }

    if (data.provider == "deepseek" && data.deepseek_api_key.empty()) {
        warning("AI Assistant: DeepSeek API key is required.");
        return false;
    }

    return true;
}

// Get available models for a provider
std::vector<std::string> SettingsFormManager::get_available_models(const std::string& provider, const std::string& api_key) {
    if (provider == "gemini") {
        return settings_t::gemini_models;
    } else if (provider == "openai") {
        return settings_t::openai_models;
    } else if (provider == "openrouter") {
        // Try to fetch dynamically, fallback to static list
        std::vector<std::string> models = fetch_openrouter_models_via_api(api_key.c_str());
        return models.empty() ? settings_t::openrouter_models : models;
    } else if (provider == "anthropic") {
        return settings_t::anthropic_models;
    } else if (provider == "copilot") {
        return settings_t::copilot_models;
    } else if (provider == "deepseek") {
        return settings_t::deepseek_models;
    }

    return {};
}

// Find model index in model list
int SettingsFormManager::find_model_index(const std::vector<std::string>& models, const std::string& current_model) {
    auto it = std::find(models.begin(), models.end(), current_model);
    return (it == models.end()) ? 0 : static_cast<int>(std::distance(models.begin(), it));
}

// Test connection with form data
AiDAConstants::ConnectionTestResult SettingsFormManager::test_connection(const SettingsFormData& data) {
    AiDAConstants::ConnectionTestResult result;
    result.success = false;
    result.response_time_ms = 0;

    try {
        // Create temporary settings from form data
        settings_t temp_settings;
        if (!apply_form_data_to_settings(data, temp_settings)) {
            result.message = "Error: Failed to create test settings";
            result.details = "Could not apply form data to settings structure";
            return result;
        }

        // Create AI client for testing
        auto client = get_ai_client(temp_settings);
        if (!client) {
            result.message = "Error: Failed to create AI client for testing";
            result.details = "Provider: " + temp_settings.api_provider;
            return result;
        }

        // Perform connection test
        auto start_time = std::chrono::steady_clock::now();
        result = client->test_connection();
        auto end_time = std::chrono::steady_clock::now();
        result.response_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

        return result;
    } catch (const std::exception& e) {
        result.message = "Error: Exception during connection test";
        result.details = std::string("Exception: ") + e.what();
        return result;
    }
}

// Main form display function
bool SettingsFormManager::show_settings_dialog(SettingsFormData& data) {
    const std::string form_str = generate_form_string();
    const qstrvec_t providers_list = create_provider_list();

    // Get available models for each provider
    auto gemini_models = get_available_models("gemini");
    auto openai_models = get_available_models("openai");
    auto openrouter_models = get_available_models("openrouter", data.openrouter_api_key);
    auto anthropic_models = get_available_models("anthropic");
    auto copilot_models = get_available_models("copilot");
    auto deepseek_models = get_available_models("deepseek");

    // Create model lists
    qstrvec_t gemini_models_qsv = create_model_list(gemini_models);
    qstrvec_t openai_models_qsv = create_model_list(openai_models);
    qstrvec_t openrouter_models_qsv = create_model_list(openrouter_models);
    qstrvec_t anthropic_models_qsv = create_model_list(anthropic_models);
    qstrvec_t copilot_models_qsv = create_model_list(copilot_models);
    qstrvec_t deepseek_models_qsv = create_model_list(deepseek_models);

    // Update model indices based on available models
    data.gemini_model_index = find_model_index(gemini_models, data.gemini_model_name);
    data.openai_model_index = find_model_index(openai_models, data.openai_model_name);
    data.openrouter_model_index = find_model_index(openrouter_models, data.openrouter_model_name);
    data.anthropic_model_index = find_model_index(anthropic_models, data.anthropic_model_name);
    data.copilot_model_index = find_model_index(copilot_models, data.copilot_model_name);
    data.deepseek_model_index = find_model_index(deepseek_models, data.deepseek_model_name);

    // Format double values for form
    qstring bulk_delay_str = format_double_for_form(data.bulk_processing_delay).c_str();
    qstring temp_str = format_double_for_form(data.temperature).c_str();

    // Prepare form values
    sval_t xref_count = data.xref_context_count;
    sval_t xref_depth = data.xref_analysis_depth;
    sval_t snippet_lines = data.xref_code_snippet_lines;
    sval_t max_tokens = data.max_prompt_tokens;

    int result = ask_form(form_str.c_str(),
        // general tab (8 args)
        &providers_list, &data.provider_index,
        &xref_count, &xref_depth, &snippet_lines,
        &bulk_delay_str, &max_tokens, &temp_str,
        // gemini tab (3 args)
        &string_utils::to_qstring(data.gemini_api_key), &gemini_models_qsv, &data.gemini_model_index,
        // openai tab (3 args)
        &string_utils::to_qstring(data.openai_api_key), &openai_models_qsv, &data.openai_model_index,
        // openrouter tab (3 args)
        &string_utils::to_qstring(data.openrouter_api_key), &openrouter_models_qsv, &data.openrouter_model_index,
        // anthropic tab (3 args)
        &string_utils::to_qstring(data.anthropic_api_key), &anthropic_models_qsv, &data.anthropic_model_index,
        // copilot tab (3 args)
        &string_utils::to_qstring(data.copilot_proxy_address), &copilot_models_qsv, &data.copilot_model_index,
        // deepseek tab (3 args)
        &string_utils::to_qstring(data.deepseek_api_key), &deepseek_models_qsv, &data.deepseek_model_index,
        // tab control (1 arg)
        &data.selected_tab
    );

    if (result == static_cast<int>(AiDAConstants::FormResult::OK)) {
        // Update data from form results
        data.provider = provider_index_to_name(data.provider_index);
        data.xref_context_count = static_cast<int>(xref_count);
        data.xref_analysis_depth = static_cast<int>(xref_depth);
        data.xref_code_snippet_lines = static_cast<int>(snippet_lines);
        data.max_prompt_tokens = static_cast<int>(max_tokens);
        data.bulk_processing_delay = parse_double_from_form(bulk_delay_str.c_str(), AiDAConstants::DEFAULT_BULK_PROCESSING_DELAY);
        data.temperature = parse_double_from_form(temp_str.c_str(), AiDAConstants::DEFAULT_TEMPERATURE);

        // Update model selections
        if (data.gemini_model_index < static_cast<int>(gemini_models.size())) {
            data.gemini_model_name = gemini_models[data.gemini_model_index];
        }
        if (data.openai_model_index < static_cast<int>(openai_models.size())) {
            data.openai_model_name = openai_models[data.openai_model_index];
        }
        if (data.openrouter_model_index < static_cast<int>(openrouter_models.size())) {
            data.openrouter_model_name = openrouter_models[data.openrouter_model_index];
        }
        if (data.anthropic_model_index < static_cast<int>(anthropic_models.size())) {
            data.anthropic_model_name = anthropic_models[data.anthropic_model_index];
        }
        if (data.copilot_model_index < static_cast<int>(copilot_models.size())) {
            data.copilot_model_name = copilot_models[data.copilot_model_index];
        }
        if (data.deepseek_model_index < static_cast<int>(deepseek_models.size())) {
            data.deepseek_model_name = deepseek_models[data.deepseek_model_index];
        }

        return true;
    }

    return false;
}
