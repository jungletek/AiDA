#include "string_utils.hpp"
#include "debug_logger.hpp"
#include <Windows.h> // For clipboard operations

namespace string_utils {

// Safe conversion with error handling
std::optional<std::string> safe_to_std(const qstring& qs) {
    try {
        if (qs.empty()) {
            return std::string("");
        }

        const char* cstr = qs.c_str();
        if (cstr == nullptr) {
            DebugLogger::log_error("qstring to std::string conversion failed",
                std::runtime_error("qstring::c_str() returned nullptr"));
            return std::nullopt;
        }

        return std::string(cstr);
    } catch (const std::exception& e) {
        DebugLogger::log_error("Exception in safe_to_std", e);
        return std::nullopt;
    }
}

std::optional<qstring> safe_to_qstring(const std::string& s) {
    try {
        if (s.empty()) {
            return qstring("");
        }

        // Check for reasonable size limits
        if (s.length() > 10 * 1024 * 1024) { // 10MB limit
            DebugLogger::log_error("String too large for qstring conversion",
                std::runtime_error("String size: " + std::to_string(s.length())));
            return std::nullopt;
        }

        return qstring(s.c_str());
    } catch (const std::exception& e) {
        DebugLogger::log_error("Exception in safe_to_qstring", e);
        return std::nullopt;
    }
}

// Legacy compatibility functions
std::string to_std(const qstring& qs) {
    auto result = safe_to_std(qs);
    return result ? *result : std::string("");
}

qstring to_qstring(const std::string& s) {
    auto result = safe_to_qstring(s);
    return result ? *result : qstring("");
}

qstring to_qstring(const char* s) {
    if (s == nullptr) {
        return qstring("");
    }
    return qstring(s);
}

// Batch conversion utilities
std::vector<std::string> qstringvec_to_stdvec(const qstrvec_t& qvec) {
    std::vector<std::string> result;
    result.reserve(qvec.size());

    for (const auto& qs : qvec) {
        auto converted = safe_to_std(qs);
        if (converted) {
            result.push_back(*converted);
        } else {
            DebugLogger::log_error("Failed to convert qstring in batch conversion",
                std::runtime_error("Index: " + std::to_string(result.size())));
            result.push_back(""); // Add empty string as fallback
        }
    }

    return result;
}

qstrvec_t stdvec_to_qstringvec(const std::vector<std::string>& vec) {
    qstrvec_t result;
    result.reserve(vec.size());

    for (const auto& s : vec) {
        auto converted = safe_to_qstring(s);
        if (converted) {
            result.push_back(*converted);
        } else {
            DebugLogger::log_error("Failed to convert std::string in batch conversion",
                std::runtime_error("Index: " + std::to_string(result.size())));
            result.push_back(qstring("")); // Add empty qstring as fallback
        }
    }

    return result;
}

} // namespace string_utils

// QStringConverter implementations with proper RAII
QStringConverter::QStringConverter(const std::string& s) : _valid(false) {
    try {
        auto qstr_result = string_utils::safe_to_qstring(s);
        if (qstr_result) {
            _qstr = *qstr_result;
            _stdstr = s;
            _valid = true;
        } else {
            _qstr = qstring("");
            _stdstr = "";
        }
    } catch (const std::exception& e) {
        DebugLogger::log_error("QStringConverter constructor failed", e);
        _qstr = qstring("");
        _stdstr = "";
        _valid = false;
    }
}

QStringConverter::QStringConverter(const qstring& qs) : _valid(false) {
    try {
        auto str_result = string_utils::safe_to_std(qs);
        if (str_result) {
            _stdstr = *str_result;
            _qstr = qs;
            _valid = true;
        } else {
            _stdstr = "";
            _qstr = qstring("");
        }
    } catch (const std::exception& e) {
        DebugLogger::log_error("QStringConverter constructor failed", e);
        _stdstr = "";
        _qstr = qstring("");
        _valid = false;
    }
}

QStringConverter::QStringConverter(const char* s) : _valid(false) {
    try {
        std::string temp_str(s ? s : "");
        auto qstr_result = string_utils::safe_to_qstring(temp_str);
        if (qstr_result) {
            _qstr = *qstr_result;
            _stdstr = temp_str;
            _valid = true;
        } else {
            _qstr = qstring("");
            _stdstr = "";
        }
    } catch (const std::exception& e) {
        DebugLogger::log_error("QStringConverter constructor failed", e);
        _qstr = qstring("");
        _stdstr = "";
        _valid = false;
    }
}

QStringConverter::~QStringConverter() {
    // RAII cleanup is automatic
}

// ClipboardManager implementation with proper RAII
ClipboardManager::ClipboardManager() : _opened(false), _memory_handle(nullptr) {
    _opened = OpenClipboard(nullptr);
    if (!_opened) {
        DebugLogger::log_error("Failed to open clipboard",
            std::runtime_error("OpenClipboard failed"));
    }
}

ClipboardManager::~ClipboardManager() {
    if (_opened) {
        CloseClipboard();
    }
    if (_memory_handle) {
        GlobalFree(_memory_handle);
    }
}

bool ClipboardManager::set_text(const qstring& text) {
    if (!is_valid()) {
        return false;
    }

    if (!EmptyClipboard()) {
        DebugLogger::log_error("Failed to empty clipboard",
            std::runtime_error("EmptyClipboard failed"));
        return false;
    }

    // Convert qstring to UTF-16 for Windows clipboard
    qwstring wtext;
    if (!utf8_utf16(&wtext, text.c_str())) {
        DebugLogger::log_error("Failed to convert text to UTF-16",
            std::runtime_error("utf8_utf16 conversion failed"));
        return false;
    }

    size_t wlen = wtext.length();
    HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, (wlen + 1) * sizeof(wchar16_t));
    if (hg == nullptr) {
        DebugLogger::log_error("GlobalAlloc failed for clipboard",
            std::runtime_error("Memory allocation failed"));
        return false;
    }

    // RAII for memory management
    struct MemoryGuard {
        HGLOBAL hg;
        bool should_free;

        explicit MemoryGuard(HGLOBAL h) : hg(h), should_free(true) {}
        ~MemoryGuard() {
            if (should_free && hg) {
                GlobalFree(hg);
            }
        }
        void release() { should_free = false; }
    };

    MemoryGuard mem_guard(hg);

    wchar16_t* locked_mem = (wchar16_t*)GlobalLock(hg);
    if (locked_mem == nullptr) {
        DebugLogger::log_error("GlobalLock failed for clipboard",
            std::runtime_error("Memory lock failed"));
        return false;
    }

    memcpy(locked_mem, wtext.c_str(), (wlen + 1) * sizeof(wchar16_t));
    GlobalUnlock(hg);

    if (SetClipboardData(CF_UNICODETEXT, hg) == nullptr) {
        DebugLogger::log_error("SetClipboardData failed",
            std::runtime_error("Failed to set clipboard data"));
        return false;
    }

    // Success - Windows now owns the memory
    mem_guard.release();
    _memory_handle = nullptr; // Prevent double-free in destructor

    return true;
}

bool ClipboardManager::set_text(const std::string& text) {
    try {
        auto qstr_result = string_utils::safe_to_qstring(text);
        if (!qstr_result) {
            DebugLogger::log_error("String conversion failed for clipboard",
                std::runtime_error("Cannot convert std::string to qstring"));
            return false;
        }

        return set_text(*qstr_result);
    } catch (const std::exception& e) {
        DebugLogger::log_error("Exception in clipboard set_text", e);
        return false;
    }
}
