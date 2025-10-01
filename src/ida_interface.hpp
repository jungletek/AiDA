#pragma once

#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <memory>

// IDA Interface Layer - Clean boundary between IDA SDK and modern C++
// This class provides a modern C++ interface to IDA SDK functionality
// while isolating IDA SDK headers from modern libraries

class IDAInterface {
public:
    // Core IDA types (converted to standard types)
    using Address = unsigned long long;  // ea_t equivalent
    using FunctionInfo = struct {
        Address address;
        std::string name;
        std::string demangled_name;
        size_t size;
    };

    // Constructor - initializes IDA SDK connection
    IDAInterface();
    ~IDAInterface();

    // Disable copy/move for safety
    IDAInterface(const IDAInterface&) = delete;
    IDAInterface& operator=(const IDAInterface&) = delete;
    IDAInterface(IDAInterface&&) = delete;
    IDAInterface& operator=(IDAInterface&&) = delete;

    // Function analysis
    std::optional<FunctionInfo> get_function_info(Address address) const;
    std::vector<FunctionInfo> find_functions_by_name(const std::string& name_pattern) const;
    std::vector<FunctionInfo> get_all_functions() const;

    // Code analysis
    std::string get_function_code(Address address, size_t max_length = 0) const;
    std::vector<Address> get_function_xrefs(Address address) const;
    std::vector<Address> get_data_xrefs(Address address) const;

    // String operations (IDA qstring <-> std::string conversion)
    std::string get_function_name(Address address) const;
    bool set_function_name(Address address, const std::string& name);
    std::string get_comment(Address address) const;
    bool set_comment(Address address, const std::string& comment);

    // Navigation and exploration
    std::vector<Address> get_functions_in_range(Address start, Address end) const;
    Address get_function_start(Address address) const;
    Address get_function_end(Address address) const;

    // Type information
    std::string get_type_info(Address address) const;
    bool apply_struct_at_address(Address address, const std::string& struct_definition);

    // Utility functions
    bool is_valid_address(Address address) const;
    bool is_function_address(Address address) const;
    std::string format_address(Address address) const;

    // Context gathering for AI prompts
    struct AnalysisContext {
        std::string function_code;
        std::vector<std::string> xrefs_from;
        std::vector<std::string> xrefs_to;
        std::vector<std::string> related_functions;
        std::string struct_context;
        std::string type_info;
    };

    AnalysisContext gather_analysis_context(Address address, int depth = 1) const;

    // Error handling
    struct Error {
        std::string message;
        int ida_error_code;
    };

    std::optional<Error> get_last_error() const;

private:
    // Private implementation details
    class Impl;
    std::unique_ptr<Impl> _impl;

    // Helper methods for type conversion
    std::string qstring_to_std_string(const void* qstr) const;
    void* std_string_to_qstring(const std::string& str) const;
};

// Global IDA interface instance
extern IDAInterface g_ida_interface;

// Convenience functions for common operations
namespace ida_utils {

    // Quick access to common IDA operations
    std::string get_function_name(IDAInterface::Address address);
    std::string get_function_code(IDAInterface::Address address, size_t max_length = 0);
    bool set_function_name(IDAInterface::Address address, const std::string& name);

    // Context building for AI prompts
    std::string build_function_context(IDAInterface::Address address, int xref_depth = 1);
    std::string build_struct_context(IDAInterface::Address address);
    std::string build_naming_context(IDAInterface::Address address);

}
