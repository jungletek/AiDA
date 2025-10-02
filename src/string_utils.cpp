#include "string_utils.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <cctype>

// Static error state
static string_utils::StringError g_last_error = {string_utils::StringError::SUCCESS, "", 0};

namespace string_utils {

    // Safe string operations with error handling
    std::optional<std::string> safe_copy(const std::string& source) {
        try {
            return std::string(source);
        } catch (const std::exception& e) {
            g_last_error = {StringError::MEMORY_ERROR, e.what(), 0};
            return std::nullopt;
        }
    }

    std::optional<std::string> safe_substring(const std::string& str, size_t pos, size_t len) {
        try {
            if (pos >= str.length()) {
                g_last_error = {StringError::OUT_OF_RANGE, "Position out of range", pos};
                return std::nullopt;
            }

            if (len == std::string::npos) {
                return str.substr(pos);
            }

            return str.substr(pos, len);
        } catch (const std::exception& e) {
            g_last_error = {StringError::MEMORY_ERROR, e.what(), pos};
            return std::nullopt;
        }
    }

    std::optional<std::string> safe_trim(const std::string& str) {
        try {
            size_t start = str.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) {
                return std::string("");
            }

            size_t end = str.find_last_not_of(" \t\r\n");
            return str.substr(start, end - start + 1);
        } catch (const std::exception& e) {
            g_last_error = {StringError::MEMORY_ERROR, e.what(), 0};
            return std::nullopt;
        }
    }

    std::optional<std::string> safe_replace(const std::string& str, const std::string& from, const std::string& to) {
        try {
            if (from.empty()) {
                return std::string(str);
            }

            std::string result = str;
            size_t pos = 0;
            while ((pos = result.find(from, pos)) != std::string::npos) {
                result.replace(pos, from.length(), to);
                pos += to.length();
            }
            return result;
        } catch (const std::exception& e) {
            g_last_error = {StringError::MEMORY_ERROR, e.what(), 0};
            return std::nullopt;
        }
    }

    // String validation utilities
    bool is_valid_utf8(const std::string& str) {
        try {
            // Simple UTF-8 validation
            for (size_t i = 0; i < str.length(); ++i) {
                unsigned char c = str[i];
                if (c <= 0x7F) continue; // ASCII

                if (c >= 0xC0 && c <= 0xDF) {
                    if (i + 1 >= str.length() || (str[i + 1] & 0xC0) != 0x80) return false;
                    i++;
                } else if (c >= 0xE0 && c <= 0xEF) {
                    if (i + 2 >= str.length() ||
                        (str[i + 1] & 0xC0) != 0x80 ||
                        (str[i + 2] & 0xC0) != 0x80) return false;
                    i += 2;
                } else if (c >= 0xF0 && c <= 0xF7) {
                    if (i + 3 >= str.length() ||
                        (str[i + 1] & 0xC0) != 0x80 ||
                        (str[i + 2] & 0xC0) != 0x80 ||
                        (str[i + 3] & 0xC0) != 0x80) return false;
                    i += 3;
                } else {
                    return false; // Invalid UTF-8
                }
            }
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    bool is_printable_ascii(const std::string& str) {
        return std::all_of(str.begin(), str.end(), [](char c) {
            return std::isprint(static_cast<unsigned char>(c));
        });
    }

    bool has_null_bytes(const std::string& str) {
        return str.find('\0') != std::string::npos;
    }

    // Conversion utilities
    std::string to_lower(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    std::string to_upper(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::toupper);
        return result;
    }

    std::string to_hex_string(uint64_t value, size_t width) {
        std::stringstream ss;
        ss << "0x" << std::hex << std::setw(width) << std::setfill('0') << value;
        return ss.str();
    }

    std::optional<uint64_t> from_hex_string(const std::string& hex_str) {
        try {
            if (hex_str.empty()) {
                g_last_error = {StringError::CONVERSION_FAILED, "Empty string", 0};
                return std::nullopt;
            }

            std::string str = hex_str;
            if (str.substr(0, 2) == "0x" || str.substr(0, 2) == "0X") {
                str = str.substr(2);
            }

            if (!std::all_of(str.begin(), str.end(), ::isxdigit)) {
                g_last_error = {StringError::CONVERSION_FAILED, "Invalid hex characters", 0};
                return std::nullopt;
            }

            uint64_t value = std::stoull(str, nullptr, 16);
            return value;
        } catch (const std::exception& e) {
            g_last_error = {StringError::CONVERSION_FAILED, e.what(), 0};
            return std::nullopt;
        }
    }

    // String formatting utilities
    std::string format_address(uint64_t address) {
        return to_hex_string(address, 8);
    }

    std::string format_size(size_t size) {
        std::stringstream ss;
        if (size < 1024) {
            ss << size << " B";
        } else if (size < 1024 * 1024) {
            ss << (size / 1024) << " KB";
        } else if (size < 1024 * 1024 * 1024) {
            ss << (size / (1024 * 1024)) << " MB";
        } else {
            ss << (size / (1024 * 1024 * 1024)) << " GB";
        }
        return ss.str();
    }

    std::string format_bytes(const std::vector<uint8_t>& data, size_t max_len) {
        std::stringstream ss;
        size_t len = std::min(data.size(), max_len);

        for (size_t i = 0; i < len; ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0')
               << static_cast<int>(data[i]) << " ";
        }

        if (data.size() > max_len) {
            ss << "...";
        }

    return ss.str();
}

// IDA SDK string conversion utilities moved to ida_utils.cpp
// These functions require IDA SDK headers and should not be in the general string_utils

// String splitting and joining
    std::vector<std::string> split(const std::string& str, char delimiter) {
        std::vector<std::string> result;
        std::stringstream ss(str);
        std::string item;

        while (std::getline(ss, item, delimiter)) {
            result.push_back(item);
        }

        return result;
    }

    std::vector<std::string> split_lines(const std::string& str) {
        return split(str, '\n');
    }

    std::string join(const std::vector<std::string>& parts, const std::string& separator) {
        if (parts.empty()) return "";

        std::string result = parts[0];
        for (size_t i = 1; i < parts.size(); ++i) {
            result += separator + parts[i];
        }

        return result;
    }

    // String analysis
    size_t count_lines(const std::string& str) {
        if (str.empty()) return 0;

        size_t lines = 1;
        for (char c : str) {
            if (c == '\n') lines++;
        }

        return lines;
    }

    size_t count_words(const std::string& str) {
        if (str.empty()) return 0;

        size_t words = 0;
        bool in_word = false;

        for (char c : str) {
            if (std::isspace(c)) {
                in_word = false;
            } else if (!in_word) {
                words++;
                in_word = true;
            }
        }

        return words;
    }

    std::string get_line(const std::string& str, size_t line_number) {
        std::vector<std::string> lines = split_lines(str);
        if (line_number >= lines.size()) {
            g_last_error = {StringError::OUT_OF_RANGE, "Line number out of range", line_number};
            return "";
        }

        return lines[line_number];
    }

    // Safe memory operations
    std::optional<std::string> safe_read_string(const void* data, size_t max_len) {
        if (data == nullptr) {
            g_last_error = {StringError::CONVERSION_FAILED, "Null pointer", 0};
            return std::nullopt;
        }

        try {
            const char* char_data = static_cast<const char*>(data);
            size_t actual_len = 0;

            // Find null terminator or max length
            for (size_t i = 0; i < max_len; ++i) {
                if (char_data[i] == '\0') {
                    actual_len = i;
                    break;
                }
                actual_len = i + 1;
            }

            return std::string(char_data, actual_len);
        } catch (const std::exception& e) {
            g_last_error = {StringError::MEMORY_ERROR, e.what(), 0};
            return std::nullopt;
        }
    }

    std::vector<uint8_t> to_bytes(const std::string& str) {
        return std::vector<uint8_t>(str.begin(), str.end());
    }

    // Error handling
    StringError get_last_error() {
        return g_last_error;
    }

    void clear_error() {
        g_last_error = {StringError::SUCCESS, "", 0};
    }

} // namespace string_utils

// StringBuffer implementation
StringBuffer::StringBuffer(size_t size) : _size(size), _valid(false) {
    try {
        _buffer = std::make_unique<char[]>(size);
        std::fill(_buffer.get(), _buffer.get() + size, 0);
        _valid = true;
    } catch (const std::exception&) {
        _size = 0;
        _valid = false;
    }
}

StringBuffer::StringBuffer(const std::string& content) : _size(content.size()), _valid(false) {
    try {
        _buffer = std::make_unique<char[]>(content.size() + 1);
        std::copy(content.begin(), content.end(), _buffer.get());
        _buffer[content.size()] = '\0';
        _valid = true;
    } catch (const std::exception&) {
        _size = 0;
        _valid = false;
    }
}

StringBuffer::~StringBuffer() {
    // RAII cleanup is automatic
}

std::string StringBuffer::to_string() const {
    if (!_valid || !_buffer) {
        return "";
    }

    // Find null terminator
    size_t len = 0;
    while (len < _size && _buffer[len] != '\0') {
        len++;
    }

    return std::string(_buffer.get(), len);
}

// ClipboardManager implementation (Windows specific)
ClipboardManager::ClipboardManager() : _opened(false), _memory_handle(nullptr) {
    _opened = OpenClipboard(nullptr);
}

ClipboardManager::~ClipboardManager() {
    if (_opened) {
        CloseClipboard();
    }
    if (_memory_handle) {
        GlobalFree(_memory_handle);
    }
}

bool ClipboardManager::set_text(const std::string& text) {
    if (!is_valid()) {
        return false;
    }

    if (!EmptyClipboard()) {
        return false;
    }

    // Convert UTF-8 string to UTF-16 for Windows clipboard
    int wide_len = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (wide_len <= 0) {
        return false;
    }

    HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, wide_len * sizeof(wchar_t));
    if (hg == nullptr) {
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

    wchar_t* locked_mem = (wchar_t*)GlobalLock(hg);
    if (locked_mem == nullptr) {
        return false;
    }

    if (MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, locked_mem, wide_len) == 0) {
        GlobalUnlock(hg);
        return false;
    }

    GlobalUnlock(hg);

    if (SetClipboardData(CF_UNICODETEXT, hg) == nullptr) {
        return false;
    }

    // Success - Windows now owns the memory
    mem_guard.release();
    _memory_handle = nullptr; // Prevent double-free in destructor

    return true;
}
