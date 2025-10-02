#include "connection_pool.hpp"
#include "thirdparty_compat.hpp"
#include "debug_logger.hpp"
#include "constants.hpp"

// Global connection pool instance
ConnectionPool g_connection_pool(10); // Default pool size of 10 connections

// Implementation of ConnectionPool methods
std::shared_ptr<httplib::Client> ConnectionPool::acquire(const std::string& host) {
    std::unique_lock<safe_mutex> lock(_pool_mutex);

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

    // Wait for an available client from any host (IDA SDK compatible)
    while (_pool.empty() && _current_size >= _max_pool_size) {
        // Use IDA SDK compatible wait - use ida_wait_for_signal() or similar
        // For now, use a simple timeout-based approach to avoid macro conflicts
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        break; // Exit the loop to avoid infinite waiting
    }

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

void ConnectionPool::release(const std::string& host, std::shared_ptr<httplib::Client> client) {
    if (!client) return;

    std::unique_lock<safe_mutex> lock(_pool_mutex);

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
