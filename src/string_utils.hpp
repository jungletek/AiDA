#pragma once

#include <string>
#include <optional>
#include <vector>
#include <memory>
#include <ida.hpp>  // For qstring

#ifdef _WIN32
#include <Windows.h>
#endif

namespace string_utils {
    // Safe conversion with error handling
    std::optional<std::string> safe_to_std(const qstring& qs);
    std::optional<qstring> safe_to_qstring(const std::string& s);

    // Convert qstring to std::string (legacy compatibility)
    std::string to_std(const qstring& qs);

    // Convert std::string to qstring (legacy compatibility)
    qstring to_qstring(const std::string& s);

    // Convert C-string to qstring (legacy compatibility)
    qstring to_qstring(const char* s);

    // Utility for converting string literals at compile time
    template<size_t N>
    qstring to_qstring(const char (&s)[N]) {
        return qstring(s);
    }

    // Batch conversion utilities
    std::vector<std::string> qstringvec_to_stdvec(const qstrvec_t& qvec);
    qstrvec_t stdvec_to_qstringvec(const std::vector<std::string>& vec);
}

// RAII string converters with automatic cleanup and error handling
class QStringConverter {
private:
    std::unique_ptr<char[]> _buffer;
    qstring _qstr;
    std::string _stdstr;
    bool _valid;

public:
    explicit QStringConverter(const std::string& s);
    explicit QStringConverter(const qstring& qs);
    explicit QStringConverter(const char* s);
    ~QStringConverter();

    // Conversion operators
    operator std::string() const { return _stdstr; }
    operator qstring() const { return _qstr; }

    // Accessors
    const qstring& qstr() const { return _qstr; }
    const std::string& str() const { return _stdstr; }
    bool is_valid() const { return _valid; }

    // Disable copying
    QStringConverter(const QStringConverter&) = delete;
    QStringConverter& operator=(const QStringConverter&) = delete;
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
    bool set_text(const qstring& text);
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
