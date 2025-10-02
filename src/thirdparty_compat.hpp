#pragma once

// ============================================================================
// Enhanced Third-party Library Compatibility Header
// Pure C++ compatibility layer - NO IDA SDK DEPENDENCIES
// ============================================================================

// ============================================================================
// Windows Header Include Order Fix (for third-party libraries)
// ============================================================================

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601  // Windows 7+
#endif

// Include Windows headers in correct order for third-party libraries
#include <winsock2.h>
#include <windows.h>
#include <cstdarg>

// ============================================================================
// THIRD-PARTY LIBRARY INCLUDES
// ============================================================================

// Note: Third-party libraries should be included in individual source files
// where needed to maintain clean separation and avoid conflicts
// #include <httplib.h>  // Include only where needed
// #include <nlohmann/json.hpp>  // Include only where needed

// ============================================================================
// SAFE TYPE ALIASES AND COMPATIBILITY LAYER
// ============================================================================

namespace thirdparty_compat {
    // Safe type aliases for standard library compatibility
    using string = std::string;
    using vector = std::vector<std::string>;
    using mutex = std::mutex;
    using condition_variable = std::condition_variable;
    using thread = std::thread;
    using ostringstream = std::ostringstream;
    using istringstream = std::istringstream;

    // Safe chrono aliases
    using steady_clock = std::chrono::steady_clock;
    using system_clock = std::chrono::system_clock;
    using milliseconds = std::chrono::milliseconds;
    using seconds = std::chrono::seconds;
    using minutes = std::chrono::minutes;

    // Safe atomic aliases
    using atomic_bool = std::atomic<bool>;
    using atomic_size_t = std::atomic<size_t>;

    // Safe MD5 hash replacement (simple hash to avoid any potential conflicts)
    inline std::string safe_md5_hash(const std::string& input) {
        // Simple hash to avoid any potential MD5 macro conflicts
        size_t hash = 0x5381;  // FNV-1a initial hash
        for (char c : input) {
            hash = hash * 33 + static_cast<unsigned char>(c);
        }
        std::stringstream ss;
        ss << std::hex << std::setw(8) << std::setfill('0') << hash;
        return ss.str();
    }

    // Type definitions for third-party libraries
    using Client = httplib::Client;
    using Headers = httplib::Headers;
    using Result = httplib::Result;

    // Standard library function wrappers to ensure consistency
    inline int safe_snprintf(char* buffer, size_t bufsz, const char* format, ...) {
        va_list va;
        va_start(va, format);
        int result = std::vsnprintf(buffer, bufsz, format, va);
        va_end(va);
        return result;
    }

    inline size_t safe_strftime(char* ptr, size_t maxsize, const char* format, const std::tm* timeptr) {
        return std::strftime(ptr, maxsize, format, timeptr);
    }
};
