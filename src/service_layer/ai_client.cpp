#include "thirdparty_compat.hpp"
#include "ai_client.hpp"
#include "unified_ai_client.hpp"
#include "connection_pool.hpp"
#include "request_cache.hpp"
#include "debug_logger.hpp"
#include "string_utils.hpp"
#include "constants.hpp"
#include <nlohmann/json.hpp>
#include <httplib.h>
#include <algorithm>
#include <thread>

// Global instances
ConnectionPool g_connection_pool;
RequestCache g_request_cache(thirdparty_compat::minutes(30), 1000);

// Factory function to create unified client
std::unique_ptr<UnifiedAIClient> get_ai_client(const settings_t& settings) {
    // Convert provider to lowercase using our string conversion utilities
    std::string provider = string_utils::to_lower(settings.api_provider);

    try {
        // Create the unified client using the provider name
        auto client = std::make_unique<UnifiedAIClient>(provider, g_connection_pool, g_request_cache);

        if (client->is_available()) {
            return std::move(client);
        } else {
            return {};
        }
    } catch (const std::exception& e) {
        return {};
    }
}
