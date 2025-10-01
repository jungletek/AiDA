#include "request_cache.hpp"
#include "debug_logger.hpp"
#include <sstream>
#include <iomanip>
#include <openssl/md5.h> // For cache key hashing

// Global request cache instance
RequestCache g_request_cache(std::chrono::minutes(30), 1000);

// Generate a cache key using MD5 hash of request parameters
std::string RequestCache::generate_cache_key(
    const std::string& host,
    const std::string& path,
    const std::string& request_body,
    const std::string& model_name) const {

    std::stringstream ss;
    ss << host << "|" << path << "|" << request_body << "|" << model_name;

    std::string combined = ss.str();

    // Generate MD5 hash for compact cache key
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5(reinterpret_cast<const unsigned char*>(combined.c_str()), combined.length(), digest);

    std::stringstream hash_ss;
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
        hash_ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }

    return hash_ss.str();
}

// Try to get a cached response
std::optional<std::string> RequestCache::get(
    const std::string& host,
    const std::string& path,
    const std::string& request_body,
    const std::string& model_name) {

    std::lock_guard<std::mutex> lock(_cache_mutex);

    std::string key = generate_cache_key(host, path, request_body, model_name);
    auto it = _cache.find(key);

    if (it != _cache.end()) {
        const auto& entry = it->second;

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

    std::lock_guard<std::mutex> lock(_cache_mutex);

    std::string key = generate_cache_key(host, path, request_body, model_name);

    // Check if we should cache this response
    if (is_error && response.find("Error: API returned status 429") == std::string::npos) {
        // Only cache rate limit errors, not other API errors
        return;
    }

    _cache[key] = CacheEntry(response, is_error);
    enforce_limits();
}

// Check if cache entry is expired
bool RequestCache::is_expired(const CacheEntry& entry) const {
    auto now = std::chrono::steady_clock::now();
    auto age = now - entry.timestamp;

    // Error responses expire faster (5 minutes) than successful responses (30 minutes)
    auto ttl = entry.is_error ? std::chrono::minutes(5) : _default_ttl;
    return age >= ttl;
}

// Remove expired entries and enforce size limit
void RequestCache::enforce_limits() {
    auto now = std::chrono::steady_clock::now();

    // Remove expired entries
    for (auto it = _cache.begin(); it != _cache.end();) {
        if (is_expired(it->second)) {
            it = _cache.erase(it);
        } else {
            ++it;
        }
    }

    // If still over limit, remove oldest entries
    if (_cache.size() > _max_cache_size) {
        // Convert to vector and sort by timestamp
        std::vector<std::pair<std::string, std::chrono::steady_clock::time_point>> entries;
        for (const auto& pair : _cache) {
            entries.emplace_back(pair.first, pair.second.timestamp);
        }

        // Sort by timestamp (oldest first)
        std::sort(entries.begin(), entries.end(),
            [](const auto& a, const auto& b) {
                return a.second < b.second;
            });

        // Remove oldest entries until we're under the limit
        size_t to_remove = _cache.size() - _max_cache_size;
        for (size_t i = 0; i < to_remove; ++i) {
            _cache.erase(entries[i].first);
        }
    }
}

// Cache management functions
void RequestCache::clear() {
    std::lock_guard<std::mutex> lock(_cache_mutex);
    _cache.clear();
}

size_t RequestCache::size() const {
    std::lock_guard<std::mutex> lock(_cache_mutex);
    return _cache.size();
}

void RequestCache::cleanup_expired() {
    std::lock_guard<std::mutex> lock(_cache_mutex);
    enforce_limits();
}

// Get cache statistics (simplified implementation)
RequestCache::CacheStats RequestCache::get_stats() const {
    std::lock_guard<std::mutex> lock(_cache_mutex);

    size_t error_count = 0;
    for (const auto& pair : _cache) {
        if (pair.second.is_error) {
            error_count++;
        }
    }

    // Note: This is a simplified stats implementation
    // In a full implementation, you'd track hit/miss counts separately
    return {
        _cache.size(),
        error_count,
        0, // hit_count - would need atomic counters
        0  // miss_count - would need atomic counters
    };
}
