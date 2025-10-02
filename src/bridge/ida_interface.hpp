#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>

// ============================================================================
// IDA Interface Layer - Clean C++ boundary to IDA SDK
// This file contains NO IDA SDK headers or macros
// ============================================================================

class IDABridge {
public:
    // Modern C++ types (no IDA SDK dependencies)
    using Address = uint64_t;

    struct FunctionInfo {
        Address address;
        std::string name;
        std::string demangled_name;
        size_t size;
        std::string comment;
    };

    struct AnalysisContext {
        std::string function_code;
        std::vector<std::string> xrefs_from;
        std::vector<std::string> xrefs_to;
        std::vector<std::string> related_functions;
        std::string type_info;
        std::string struct_info;
    };

    struct Error {
        std::string message;
        int code;
    };

    // Constructor - initializes IDA SDK connection
    IDABridge();
    ~IDABridge();

    // Disable copy/move for safety
    IDABridge(const IDABridge&) = delete;
    IDABridge& operator=(const IDABridge&) = delete;
    IDABridge(IDABridge&&) = delete;
    IDABridge& operator=(IDABridge&&) = delete;

    // Core IDA operations with modern C++ interface
    std::optional<FunctionInfo> get_function_info(Address address) const;
    std::vector<FunctionInfo> find_functions_by_name(const std::string& pattern) const;
    std::vector<FunctionInfo> get_all_functions() const;

    // Code and disassembly
    std::string get_function_code(Address address, size_t max_length = 1000) const;
    std::string get_disassembly(Address address, size_t line_count = 50) const;

    // Cross-references
    std::vector<Address> get_function_xrefs(Address address) const;
    std::vector<Address> get_data_xrefs(Address address) const;

    // Function management
    std::string get_function_name(Address address) const;
    bool set_function_name(Address address, const std::string& name);
    std::string get_comment(Address address) const;
    bool set_comment(Address address, const std::string& comment);

    // Navigation
    std::vector<Address> get_functions_in_range(Address start, Address end) const;
    Address get_function_start(Address address) const;
    Address get_function_end(Address address) const;

    // Type and structure information
    std::string get_type_info(Address address) const;
    bool apply_struct_at_address(Address address, const std::string& struct_def);

    // Utility functions
    bool is_valid_address(Address address) const;
    bool is_function_address(Address address) const;
    bool is_code_address(Address address) const;
    bool is_data_address(Address address) const;
    std::string format_address(Address address) const;

    // Context gathering for AI analysis
    AnalysisContext gather_analysis_context(Address address, int depth = 1) const;

    // Error handling
    std::optional<Error> get_last_error() const;
    void clear_error();

    // Database information
    std::string get_database_filename() const;
    std::string get_root_filename() const;
    size_t get_database_size() const;

private:
    // Opaque implementation - completely hides IDA SDK
    class BridgeImpl;
    std::unique_ptr<BridgeImpl> _impl;
};

// Global IDA bridge instance
extern IDABridge g_ida_bridge;

// Convenience functions for common operations
namespace ida_utils {

    // Quick access to common IDA operations
    inline std::string get_function_name(IDABridge::Address address) {
        return g_ida_bridge.get_function_name(address);
    }

    inline std::string get_function_code(IDABridge::Address address, size_t max_length = 1000) {
        return g_ida_bridge.get_function_code(address, max_length);
    }

    inline bool set_function_name(IDABridge::Address address, const std::string& name) {
        return g_ida_bridge.set_function_name(address, name);
    }

    // Context building for AI prompts
    std::string build_function_context(IDABridge::Address address, int xref_depth = 1);
    std::string build_struct_context(IDABridge::Address address);
    std::string build_naming_context(IDABridge::Address address);

    // Address validation and conversion
    bool is_valid_address(IDABridge::Address address);
    std::string format_address(IDABridge::Address address);

}
