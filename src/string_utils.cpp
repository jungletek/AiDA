#include "string_utils.hpp"

namespace string_utils {

std::string to_std(const qstring& qs) {
    return std::string(qs.c_str());
}

qstring to_qstring(const std::string& s) {
    return qstring(s.c_str());
}

qstring to_qstring(const char* s) {
    return qstring(s);
}

} // namespace string_utils

// StringConverter implementations
StringConverter::StringConverter(const qstring& qs) : _stdstr(string_utils::to_std(qs)) {}
StringConverter::StringConverter(const std::string& s) : _qstr(string_utils::to_qstring(s)) {}
StringConverter::StringConverter(const char* s) : _qstr(string_utils::to_qstring(s)) {}
