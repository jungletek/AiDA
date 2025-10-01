#pragma once

#include "constants.hpp"
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <string>
#include <chrono>

// Connection pool for HTTP clients to improve performance and resource management
class ConnectionPool {
private:
    std::queue<std::shared_ptr<httplib::Client>> _pool;
    std::mutex _pool_mutex;
    std::condition_variable _pool_cv;
    size_t _max_pool_size;
    size_t _current_size;

    // Track clients by host for better connection reuse
    std::unordered_map<std::string, std::queue<std::shared_ptr<httplib::Client>>> _host_pools;

    // Cleanup timer to remove stale connections
    std::chrono::steady_clock::time_point _last_cleanup;
    static constexpr auto CLEANUP_INTERVAL = std::chrono::minutes(5);

public:
    explicit ConnectionPool(size_t max_pool_size = 10)
        : _max_pool_size(max_pool_size), _current_size(0), _last_cleanup(std::chrono::steady_clock::now()) {}

    ~ConnectionPool() {
        clear();
    }

    // Get a client for the specified host
    std::shared_ptr<httplib::Client> acquire(const std::string& host) {
        std::unique_lock<std::mutex> lock(_pool_mutex);

        // Cleanup stale connections periodically
        cleanup_if_needed(lock);

        // Try to get a client from the host-specific pool first
        auto host_it = _host_pools.find(host);
        if (host_it != _host_pools.end() && !host_it->second.empty()) {
            auto client = host_it->second.front();
            host_it->second.pop();

            // Verify client is still valid
            if (client && client->is_valid()) {
                return client;
            } else if (client) {
                // Client is invalid, clean it up
                client->stop();
                _current_size--;
            }
        }

        // If we haven't reached the pool limit, create a new client
        if (_current_size < _max_pool_size) {
            lock.unlock();
            auto new_client = std::make_shared<httplib::Client>(host.c_str());
            new_client->set_read_timeout(AiDAConstants::READ_TIMEOUT_MS / 1000);
            new_client->set_connection_timeout(AiDAConstants::CONNECTION_TIMEOUT_MS / 1000);

            lock.lock();
            _current_size++;
            return new_client;
        }

        // Wait for an available client from any host
        _pool_cv.wait(lock, [this]() {
            return !_pool.empty() || _current_size < _max_pool_size;
        });

        // Try again to get a client
        if (!_pool.empty()) {
            auto client = _pool.front();
            _pool.pop();

            // Verify client is still valid
            if (client && client->is_valid()) {
                return client;
            } else if (client) {
                // Client is invalid, clean it up
                client->stop();
                _current_size--;
            }
        }

        // Create a new client if pool is still under limit
        if (_current_size < _max_pool_size) {
            lock.unlock();
            auto new_client = std::make_shared<httplib::Client>(host.c_str());
            new_client->set_read_timeout(AiDAConstants::READ_TIMEOUT_MS / 1000);
            new_client->set_connection_timeout(AiDAConstants::CONNECTION_TIMEOUT_MS / 1000);

            lock.lock();
            _current_size++;
            return new_client;
        }

        // Pool is full and no valid clients available
        throw std::runtime_error("Connection pool exhausted");
    }

    // Return a client to the pool
    void release(const std::string& host, std::shared_ptr<httplib::Client> client) {
        if (!client) return;

        std::unique_lock<std::mutex> lock(_pool_mutex);

        // Only return client if it's still valid and pool isn't full
        if (client->is_valid() && _current_size <= _max_pool_size) {
            // Try to return to host-specific pool first
            _host_pools[host].push(client);
            _pool_cv.notify_one();
        } else {
            // Client is invalid or pool is full, stop and discard it
            client->stop();
            if (_current_size > 0) {
                _current_size--;
            }
        }
    }

    // Get pool statistics
    size_t get_current_size() const {
        std::lock_guard<std::mutex> lock(_pool_mutex);
        return _current_size;
    }

    size_t get_host_pool_count(const std::string& host) const {
        std::lock_guard<std::mutex> lock(_pool_mutex);
        auto it = _host_pools.find(host);
        return (it != _host_pools.end()) ? it->second.size() : 0;
    }

    // Clear all connections
    void clear() {
        std::lock_guard<std::mutex> lock(_pool_mutex);

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
    void cleanup_if_needed(std::unique_lock<std::mutex>& lock) {
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
