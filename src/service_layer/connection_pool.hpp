#pragma once

#include "constants.hpp"
#include "thirdparty_compat.hpp"
#include <memory>
#include <queue>
#include <unordered_map>

// Use safe types from thirdparty_compat to avoid IDA SDK macro conflicts
using safe_mutex = thirdparty_compat::mutex;
using safe_condition_variable = thirdparty_compat::condition_variable;
using safe_steady_clock = thirdparty_compat::steady_clock;
using safe_chrono_minutes = thirdparty_compat::minutes;

// Include httplib for proper type definitions
#include <httplib.h>

// Connection pool for HTTP clients to improve performance and resource management
class ConnectionPool {
private:
    std::queue<std::shared_ptr<httplib::Client>> _pool;
    mutable safe_mutex _pool_mutex;  // ← Mutable for const method locking
    safe_condition_variable _pool_cv;
    size_t _max_pool_size;
    size_t _current_size;

    // Track clients by host for better connection reuse
    std::unordered_map<std::string, std::queue<std::shared_ptr<httplib::Client>>> _host_pools;

    // Cleanup timer to remove stale connections
    safe_steady_clock::time_point _last_cleanup;
    static constexpr auto CLEANUP_INTERVAL = safe_chrono_minutes(5);

public:
    explicit ConnectionPool(size_t max_pool_size = 10)
        : _max_pool_size(max_pool_size), _current_size(0), _last_cleanup(safe_steady_clock::now()) {}

    ~ConnectionPool() {
        clear();
    }

    // Get a client for the specified host
    std::shared_ptr<httplib::Client> acquire(const std::string& host);

    // Return a client to the pool
    void release(const std::string& host, std::shared_ptr<httplib::Client> client);

    // Get pool statistics
    size_t get_current_size() const {
        std::lock_guard<safe_mutex> lock(_pool_mutex);
        return _current_size;
    }

    size_t get_host_pool_count(const std::string& host) const {
        std::lock_guard<safe_mutex> lock(_pool_mutex);
        auto it = _host_pools.find(host);
        return (it != _host_pools.end()) ? it->second.size() : 0;
    }

    // Clear all connections
    void clear() {
        std::lock_guard<safe_mutex> lock(_pool_mutex);

        // Stop all clients in the general pool
        while (!_pool.empty()) {
            auto client = _pool.front();
            _pool.pop();
            if (client) {
                client->stop();
            }
        }

        // Stop all clients in host-specific pools
        for (auto& host_pair : _host_pools) {
            while (!host_pair.second.empty()) {
                auto client = host_pair.second.front();
                host_pair.second.pop();
                if (client) {
                    client->stop();
                }
            }
        }

        _host_pools.clear();
        _current_size = 0;
    }

private:
    void cleanup_if_needed(std::unique_lock<safe_mutex>& lock) {
        auto now = std::chrono::steady_clock::now();
        if (now - _last_cleanup < CLEANUP_INTERVAL) {
            return;
        }

        _last_cleanup = now;

        // Clean up invalid clients from host pools
        for (auto host_it = _host_pools.begin(); host_it != _host_pools.end();) {
            auto& host_queue = host_it->second;

            // Remove invalid clients from this host's queue
            while (!host_queue.empty()) {
                auto client = host_queue.front();
                if (client && client->is_valid()) {
                    break; // Found a valid client, stop here
                }

                if (client) {
                    client->stop();
                    _current_size--;
                }
                host_queue.pop();
            }

            // If host pool is empty, remove it
            if (host_queue.empty()) {
                host_it = _host_pools.erase(host_it);
            } else {
                ++host_it;
            }
        }

        // Clean up invalid clients from general pool
        while (!_pool.empty()) {
            auto client = _pool.front();
            if (client && client->is_valid()) {
                break; // Found a valid client, stop here
            }

            if (client) {
                client->stop();
                _current_size--;
            }
            _pool.pop();
        }
    }
};

// Global connection pool instance
extern ConnectionPool g_connection_pool;
