#pragma once

// ============================================================================
// Enhanced Third-party Library Compatibility Header
// Comprehensive IDA SDK macro conflict resolution
// ============================================================================

// ============================================================================
// CRITICAL: Windows Header Include Order Fix
// ============================================================================

// Undefine problematic macros BEFORE including any standard headers
#ifdef snprintf
#undef snprintf
#endif

#ifdef vsnprintf
#undef vsnprintf
#endif

#ifdef swprintf
#undef swprintf
#endif

#ifdef vswprintf
#undef vswprintf
#endif

#ifdef wait
#undef wait
#endif

#ifdef MD5
#undef MD5
#endif

#ifdef MD5_CTX
#undef MD5_CTX
#endif

// Handle pid_t conflict by ensuring IDA SDK is included first
// The IDA SDK defines pid_t as int, but system headers define it as _pid_t
// We'll let IDA SDK win by not undefining pid_t here

// Force correct Windows header include order - this MUST be first
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601  // Windows 7+
#endif

// Include Windows headers in correct order BEFORE any other includes
#include <winsock2.h>
#include <windows.h>

// Forward declarations for IDA SDK types to avoid conflicts
extern "C" {
    typedef int pid_t;  // IDA SDK definition
}

// Include IDA SDK headers first to establish type definitions
#include <ida.hpp>
#include <idd.hpp>

// ============================================================================
// THIRD-PARTY LIBRARY INCLUDES WITH MACRO ISOLATION
// ============================================================================

// Include third-party libraries with minimal macro isolation
#include <httplib.h>

// Note: nlohmann/json.hpp should be included in individual source files
// to avoid macro conflicts and include path issues

// ============================================================================
// SAFE TYPE ALIASES AND COMPATIBILITY LAYER
// ============================================================================

namespace thirdparty_compat {
    // Safe type aliases that avoid IDA SDK conflicts
    using pid_type = int;
    using wait_function = void(*)();

    // Safe function wrappers for standard library functions (simplified)
    // Note: Using standard library functions directly to avoid IDA SDK conflicts

    // Safe MD5 hash replacement (IDA SDK conflicts with OpenSSL MD5)
    inline std::string safe_md5_hash(const std::string& input) {
        // Simple hash to avoid IDA SDK MD5 macro conflicts
        size_t hash = 0x5381;  // FNV-1a initial hash
        for (char c : input) {
            hash = hash * 33 + static_cast<unsigned char>(c);
        }
        std::stringstream ss;
        ss << std::hex << std::setw(8) << std::setfill('0') << hash;
        return ss.str();
    }

    // Safe type definitions (json defined in individual files to avoid include issues)
    // using json = nlohmann::json;
    using Client = httplib::Client;
    using Headers = httplib::Headers;
    using Result = httplib::Result;

    // Safe standard library aliases to avoid IDA SDK macro conflicts
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
}
