#pragma once

#include "constants.hpp"
#include <string>
#include <optional>
#include <unordered_map>
#include <mutex>
#include <chrono>

// Request cache to avoid duplicate API calls and improve performance
class RequestCache {
private:
    struct CacheEntry {
        std::string response;
        std::chrono::steady_clock::time_point timestamp;
        bool is_error; // Track if this is an error response to cache errors too

        CacheEntry(std::string resp, bool error = false)
            : response(std::move(resp)), timestamp(std::chrono::steady_clock::now()), is_error(error) {}
    };

    std::unordered_map<std::string, CacheEntry> _cache;
    std::mutex _cache_mutex;
    std::chrono::minutes _default_ttl;
    size_t _max_cache_size;

    // Cache key generation
    std::string generate_cache_key(
        const std::string& host,
        const std::string& path,
        const std::string& request_body,
        const std::string& model_name) const;

public:
    explicit RequestCache(
        std::chrono::minutes default_ttl = std::chrono::minutes(30),
        size_t max_cache_size = 1000)
        : _default_ttl(default_ttl), _max_cache_size(max_cache_size) {}

    // Try to get a cached response
    std::optional<std::string> get(
        const std::string& host,
        const std::string& path,
        const std::string& request_body,
        const std::string& model_name);

    // Store a response in the cache
    void put(
        const std::string& host,
        const std::string& path,
        const std::string& request_body,
        const std::string& model_name,
        const std::string& response,
        bool is_error = false);

    // Cache management
    void clear();
    size_t size() const;
    void set_ttl(std::chrono::minutes ttl) { _default_ttl = ttl; }
    void set_max_size(size_t max_size) { _max_cache_size = max_size; }

    // Cleanup expired entries
    void cleanup_expired();

    // Cache statistics
    struct CacheStats {
        size_t total_entries;
        size_t error_entries;
        size_t hit_count;
        size_t miss_count;
        double hit_rate() const {
            size_t total = hit_count + miss_count;
            return total > 0 ? static_cast<double>(hit_count) / total : 0.0;
        }
    };

    CacheStats get_stats() const;

private:
    // Check if cache entry is expired
    bool is_expired(const CacheEntry& entry) const;

    // Remove expired entries and enforce size limit
    void enforce_limits();
};

// Global request cache instance
extern RequestCache g_request_cache;
