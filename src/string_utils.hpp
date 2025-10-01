#pragma once

#include <string>
#include <ida.hpp>  // For qstring

namespace string_utils {
    // Convert qstring to std::string
    std::string to_std(const qstring& qs);
    
    // Convert std::string to qstring
    qstring to_qstring(const std::string& s);
    
    // Convert C-string to qstring
    qstring to_qstring(const char* s);
    
    // Utility for converting string literals at compile time
    template<size_t N>
    qstring to_qstring(const char (&s)[N]) {
        return qstring(s);
    }
}

// RAII helper for temporary string conversions
class StringConverter {
private:
    qstring _qstr;
    std::string _stdstr;
    
public:
    explicit StringConverter(const qstring& qs);
    explicit StringConverter(const std::string& s);
    explicit StringConverter(const char* s);
    
    operator std::string() const { return _stdstr; }
    operator qstring() const { return _qstr; }
    
    const std::string& std_str() const { return _stdstr; }
    const qstring& q_str() const { return _qstr; }
};
