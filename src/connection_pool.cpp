#include "connection_pool.hpp"
#include "thirdparty_compat.hpp"
#include "debug_logger.hpp"
#include "constants.hpp"

// Global connection pool instance
ConnectionPool g_connection_pool(10); // Default pool size of 10 connections
