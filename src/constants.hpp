#pragma once

namespace AiDAConstants {
// Network timeouts
static constexpr int CONNECTION_TIMEOUT_MS = 10000;
static constexpr int READ_TIMEOUT_MS = 600000;  // 10 minutes
static constexpr int TEST_TIMEOUT_MS = 30000;   // 30 seconds

// Retry configuration
static constexpr int MAX_RETRIES = 3;
static constexpr int INITIAL_RETRY_DELAY_MS = 1000;
static constexpr int MAX_RETRY_DELAY_MS = 30000; // 30 seconds

// Size limits
static constexpr size_t MAX_RESPONSE_SIZE = 10 * 1024 * 1024;  // 10MB
static constexpr size_t MAX_ERROR_RESPONSE_SIZE = 1024 * 1024;  // 1MB

// UI limits
static constexpr int MAX_FUNCTION_NAME_LENGTH = MAXNAMELEN - 10;
static constexpr int MAX_COMMENT_LENGTH = 82;

// Threading
static constexpr int TIMER_INTERVAL_MS = 1000;
static constexpr int MAX_CONCURRENT_REQUESTS = 3;

// Rate limiting
static constexpr int DEFAULT_REQUESTS_PER_MINUTE = 60;

// Settings form constants
static constexpr int DEFAULT_XREF_CONTEXT_COUNT = 5;
static constexpr int DEFAULT_XREF_ANALYSIS_DEPTH = 3;
static constexpr int DEFAULT_XREF_CODE_SNIPPET_LINES = 30;
static constexpr double DEFAULT_BULK_PROCESSING_DELAY = 1.5;
static constexpr int DEFAULT_MAX_PROMPT_TOKENS = 1048576;
static constexpr double DEFAULT_TEMPERATURE = 0.1;

// Provider indices for form handling
enum class ProviderIndex {
    GEMINI = 0,
    OPENAI = 1,
    OPENROUTER = 2,
    ANTHROPIC = 3,
    COPILOT = 4,
    DEEPSEEK = 5
};

// Form control IDs
enum class FormControlID {
    // General tab
    PROVIDER_COMBO = 1,
    XREF_COUNT = 2,
    XREF_DEPTH = 3,
    SNIPPET_LINES = 4,
    BULK_DELAY = 5,
    MAX_TOKENS = 6,
    TEMPERATURE = 7,

    // Gemini tab
    GEMINI_API_KEY = 11,
    GEMINI_MODEL_COMBO = 12,

    // OpenAI tab
    OPENAI_API_KEY = 21,
    OPENAI_MODEL_COMBO = 22,

    // OpenRouter tab
    OPENROUTER_API_KEY = 25,
    OPENROUTER_MODEL_COMBO = 26,

    // Anthropic tab
    ANTHROPIC_API_KEY = 31,
    ANTHROPIC_MODEL_COMBO = 32,

    // Copilot tab
    COPILOT_PROXY_ADDR = 41,
    COPILOT_MODEL_COMBO = 42,

    // DeepSeek tab
    DEEPSEEK_API_KEY = 51,
    DEEPSEEK_MODEL_COMBO = 52,

    // Tab control
    TAB_CONTROL = 100
};

// Button return codes
enum class FormResult {
    OK = 1,
    TEST_CONNECTION = 0,
    CANCEL = -1
};

} // namespace AiDAConstants
