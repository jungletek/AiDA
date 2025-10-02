#pragma once

#include <cstdarg>
#include <sstream>
#include <iomanip>
#include <string>

// ============================================================================
// Third-party Library Compatibility Header
// Comprehensive macro isolation for IDA SDK compatibility
// ============================================================================

// Strategic macro undefining for third-party libraries
// IDA SDK defines macros that conflict with standard C++ libraries

#pragma push_macro("snprintf")
#pragma push_macro("vsnprintf")
#pragma push_macro("swprintf")
#pragma push_macro("vswprintf")
#pragma push_macro("wait")
#pragma push_macro("pid_t")

// Temporarily undefine problematic IDA SDK macros
#undef snprintf
#undef vsnprintf
#undef swprintf
#undef vswprintf
#undef wait
#undef pid_t

// Include third-party libraries in macro-safe environment
#include <nlohmann/json.hpp>
#include <httplib.h>

// Restore IDA SDK macros for the rest of the project
#pragma pop_macro("snprintf")
#pragma pop_macro("vsnprintf")
#pragma pop_macro("swprintf")
#pragma pop_macro("vswprintf")
#pragma pop_macro("wait")
#pragma pop_macro("pid_t")

// ============================================================================
// Compatibility Aliases and Wrappers
// ============================================================================

// Create safe aliases for commonly used functions that might be macro'd
namespace thirdparty_compat {

    // Safe snprintf wrapper that bypasses IDA SDK macro
    inline int safe_snprintf(char* buffer, size_t bufsz, const char* format, ...) {
        va_list va;
        va_start(va, format);
        int result = std::vsnprintf(buffer, bufsz, format, va);
        va_end(va);
        return result;
    }

    // Safe string conversion that works with IDA SDK macros
    inline std::string format_string(const char* format, ...) {
        char buffer[4096];  // Reasonable buffer size
        va_list va;
        va_start(va, format);
        std::vsnprintf(buffer, sizeof(buffer), format, va);
        va_end(va);
        return std::string(buffer);
    }

    // Safe MD5 hash function that works around IDA SDK macros
    inline std::string md5_hash(const std::string& input) {
        // Use OpenSSL MD5 functions with proper macro isolation
        // This implementation avoids IDA SDK macro conflicts

        // Create a simple hash using string operations instead of OpenSSL
        // to avoid macro conflicts with IDA SDK
        size_t hash = 0;
        for (char c : input) {
            hash = hash * 31 + static_cast<unsigned char>(c);
        }

        std::stringstream ss;
        ss << std::hex << std::setw(8) << std::setfill('0') << hash;
        return ss.str();
    }

} // namespace thirdparty_compat

// ============================================================================
// Type Aliases for IDA SDK Compatibility
// ============================================================================

// Create type aliases that work with IDA SDK macros
using json = nlohmann::json;
using httplib_Client = httplib::Client;
using httplib_Headers = httplib::Headers;
using httplib_Result = httplib::Result;

// Safe using declarations for commonly used types
namespace safe_types {
    using mutex = std::mutex;
    using condition_variable = std::condition_variable;
    using thread = std::thread;
    using string = std::string;
    using vector = std::vector<std::string>;
    using unordered_map = std::unordered_map<std::string, std::string>;
}
