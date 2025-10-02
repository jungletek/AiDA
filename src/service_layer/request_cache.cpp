#include "request_cache.hpp"
#include "thirdparty_compat.hpp"
#include "debug_logger.hpp"

// Global request cache instance
RequestCache g_request_cache;

// Generate a cache key from request parameters
std::string RequestCache::generate_cache_key(
    const std::string& host,
    const std::string& path,
    const std::string& request_body,
    const std::string& model_name) const {

    // Simple hash-based key generation to avoid storing large request bodies
    size_t hash = 0x5381; // FNV-1a initial hash

    // Hash host
    for (char c : host) {
        hash = hash * 33 + static_cast<unsigned char>(c);
    }

    // Hash path
    for (char c : path) {
        hash = hash * 33 + static_cast<unsigned char>(c);
    }

    // Hash model name
    for (char c : model_name) {
        hash = hash * 33 + static_cast<unsigned char>(c);
    }

    // Hash request body (first 1000 chars to avoid excessive memory usage)
    size_t body_len = std::min(request_body.length(), size_t(1000));
    for (size_t i = 0; i < body_len; ++i) {
        hash = hash * 33 + static_cast<unsigned char>(request_body[i]);
    }

    return std::to_string(hash);
}

// Try to get a cached response
std::optional<std::string> RequestCache::get(
    const std::string& host,
    const std::string& path,
    const std::string& request_body,
    const std::string& model_name) {

    std::lock_guard<safe_mutex> lock(_cache_mutex);

    std::string key = generate_cache_key(host, path, request_body, model_name);
    auto it = _cache.find(key);

    if (it != _cache.end()) {
        const CacheEntry& entry = it->second;

        // Check if entry is expired
        if (is_expired(entry)) {
            _cache.erase(it);
            return std::nullopt;
        }

        return entry.response;
    }

    return std::nullopt;
}

// Store a response in the cache
void RequestCache::put(
    const std::string& host,
    const std::string& path,
    const std::string& request_body,
    const std::string& model_name,
    const std::string& response,
    bool is_error) {

    std::lock_guard<safe_mutex> lock(_cache_mutex);

    std::string key = generate_cache_key(host, path, request_body, model_name);

    // Remove existing entry if present
    _cache.erase(key);

    // Add new entry
    _cache[key] = CacheEntry(response, is_error);

    // Enforce size limits
    enforce_limits();
}

// Clear all cache entries
void RequestCache::clear() {
    std::lock_guard<safe_mutex> lock(_cache_mutex);
    _cache.clear();
}

// Get current cache size
size_t RequestCache::size() const {
    std::lock_guard<safe_mutex> lock(_cache_mutex);
    return _cache.size();
}

// Cleanup expired entries
void RequestCache::cleanup_expired() {
    std::lock_guard<safe_mutex> lock(_cache_mutex);

    auto it = _cache.begin();
    while (it != _cache.end()) {
        if (is_expired(it->second)) {
            it = _cache.erase(it);
        } else {
            ++it;
        }
    }
}

// Get cache statistics
RequestCache::CacheStats RequestCache::get_stats() const {
    std::lock_guard<safe_mutex> lock(_cache_mutex);

    CacheStats stats;
    stats.total_entries = _cache.size();

    for (const auto& pair : _cache) {
        if (pair.second.is_error) {
            stats.error_entries++;
        }
    }

    return stats;
}

// Check if cache entry is expired
bool RequestCache::is_expired(const CacheEntry& entry) const {
    auto now = std::chrono::steady_clock::now();
    auto age = now - entry.timestamp;
    return age >= _default_ttl;
}

// Remove expired entries and enforce size limit
void RequestCache::enforce_limits() {
    // Remove expired entries
    auto it = _cache.begin();
    while (it != _cache.end()) {
        if (is_expired(it->second)) {
            it = _cache.erase(it);
        } else {
            ++it;
        }
    }

    // If still over limit, remove oldest entries
    while (_cache.size() > _max_cache_size) {
        auto oldest_it = _cache.begin();
        auto oldest_time = oldest_it->second.timestamp;

        // Find the truly oldest entry
        for (auto it = _cache.begin(); it != _cache.end(); ++it) {
            if (it->second.timestamp < oldest_time) {
                oldest_it = it;
                oldest_time = it->second.timestamp;
            }
        }

        _cache.erase(oldest_it);
    }
}
