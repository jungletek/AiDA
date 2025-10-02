#pragma once

// ============================================================================
// String Utilities - IDA SDK Independent Version
// This file contains string utilities that don't depend on IDA SDK
// IDA SDK specific string operations are in ida_layer/string_utils.hpp
// ============================================================================

#include <string>
#include <optional>
#include <vector>
#include <memory>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace string_utils {
    // Standard C++ string utilities (IDA SDK independent)

    // Safe string operations with error handling
    std::optional<std::string> safe_copy(const std::string& source);
    std::optional<std::string> safe_substring(const std::string& str, size_t pos, size_t len = std::string::npos);
    std::optional<std::string> safe_trim(const std::string& str);
    std::optional<std::string> safe_replace(const std::string& str, const std::string& from, const std::string& to);

    // String validation utilities
    bool is_valid_utf8(const std::string& str);
    bool is_printable_ascii(const std::string& str);
    bool has_null_bytes(const std::string& str);

    // Conversion utilities
    std::string to_lower(const std::string& str);
    std::string to_upper(const std::string& str);
    std::string to_hex_string(uint64_t value, size_t width = 0);
    std::optional<uint64_t> from_hex_string(const std::string& hex_str);

    // String formatting utilities
    std::string format_address(uint64_t address);
    std::string format_size(size_t size);
    std::string format_bytes(const std::vector<uint8_t>& data, size_t max_len = 256);

    // IDA SDK string conversion utilities (implemented in ida_utils.cpp)
    // These functions are declared here but implemented with IDA SDK includes
    std::string to_std(const qstring& qs);
    qstring to_qstring(const std::string& s);

    // String splitting and joining
    std::vector<std::string> split(const std::string& str, char delimiter);
    std::vector<std::string> split_lines(const std::string& str);
    std::string join(const std::vector<std::string>& parts, const std::string& separator);

    // String analysis
    size_t count_lines(const std::string& str);
    size_t count_words(const std::string& str);
    std::string get_line(const std::string& str, size_t line_number);

    // Safe memory operations
    std::optional<std::string> safe_read_string(const void* data, size_t max_len);
    std::vector<uint8_t> to_bytes(const std::string& str);

    // Error handling
    struct StringError {
        enum Code {
            SUCCESS = 0,
            INVALID_UTF8 = 1,
            NULL_BYTE_DETECTED = 2,
            OUT_OF_RANGE = 3,
            CONVERSION_FAILED = 4,
            MEMORY_ERROR = 5
        };

        Code error_code;
        std::string message;
        size_t position;
    };

    StringError get_last_error();
    void clear_error();
}

// RAII string buffer with automatic cleanup and error handling
class StringBuffer {
private:
    std::unique_ptr<char[]> _buffer;
    size_t _size;
    bool _valid;

public:
    explicit StringBuffer(size_t size);
    explicit StringBuffer(const std::string& content);
    ~StringBuffer();

    // Accessors
    char* data() { return _buffer.get(); }
    const char* data() const { return _buffer.get(); }
    size_t size() const { return _size; }
    bool is_valid() const { return _valid && _buffer != nullptr; }

    // String conversion
    std::string to_string() const;

    // Disable copying
    StringBuffer(const StringBuffer&) = delete;
    StringBuffer& operator=(const StringBuffer&) = delete;
};

// Enhanced clipboard handling with proper RAII
class ClipboardManager {
private:
    bool _opened;
    HGLOBAL _memory_handle;

public:
    ClipboardManager();
    ~ClipboardManager();

    bool is_valid() const { return _opened; }
    bool set_text(const std::string& text);

    // Disable copying
    ClipboardManager(const ClipboardManager&) = delete;
    ClipboardManager& operator=(const ClipboardManager&) = delete;
};

// String operation result with error handling
struct StringOperationResult {
    bool success;
    std::string data;
    std::string error_message;

    static StringOperationResult create_success(const std::string& data) {
        return {true, data, ""};
    }

    static StringOperationResult create_failure(const std::string& error) {
        return {false, "", error};
    }
};
