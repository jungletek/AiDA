#pragma once

// ============================================================================
// AI Client Interface - Service Layer
// This file contains the interface for AI client functionality
// Implementation is in ai_client.cpp in ida_layer (IDA SDK dependent)
// ============================================================================

#include "unified_ai_client.hpp"
#include "settings.hpp"
#include <memory>

// Factory function to create appropriate AI client based on settings
std::unique_ptr<UnifiedAIClient> get_ai_client(const settings_t& settings);
