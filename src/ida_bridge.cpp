// ============================================================================
// IDA SDK Bridge Implementation - COMPLETE ISOLATION LAYER
// This file contains ALL IDA SDK usage and is completely isolated
// from the modern C++ codebase
// ============================================================================

#include "ida_interface.hpp"
#include "string_utils.hpp"
#include <sstream>
#include <regex>
#include <algorithm>

// ============================================================================
// IDA SDK Bridge Implementation
// This file contains IDA SDK usage but follows the same pattern
// as the original working aida_pro.hpp
// ============================================================================

// Include IDA SDK headers the same way as the working original
#include <ida.hpp>
#include <kernwin.hpp>
#include <funcs.hpp>
#include <name.hpp>
#include <xref.hpp>
#include <typeinf.hpp>
#include <nalt.hpp>
#include <bytes.hpp>
#include <auto.hpp>
#include <demangle.hpp>
#include <lines.hpp>
#include <search.hpp>
#include <segment.hpp>
#include <loader.hpp>

// ============================================================================
// Bridge Implementation Class
// ============================================================================

class IDABridge::BridgeImpl {
private:
    mutable std::optional<Error> _last_error;

    // Helper methods for type conversion - use string_utils for IDA compatibility
    std::string qstring_to_std(qstring qs) const {
        return string_utils::to_std(qs);
    }

    qstring std_to_qstring(const std::string& s) const {
        return string_utils::to_qstring(s);
    }

    void set_error(const std::string& message, int code = 0) const {
        _last_error = Error{message, code};
    }

public:
    void clear_error() const {
        _last_error.reset();
    }

public:
    BridgeImpl() {
        clear_error();
        // IDA SDK should already be initialized by the plugin framework
    }

    ~BridgeImpl() = default;

    // Core function information
    std::optional<FunctionInfo> get_function_info(ea_t address) const {
        try {
            clear_error();

            if (!is_mapped(address)) {
                set_error("Address is not mapped in database", -1);
                return std::nullopt;
            }

            func_t* func = get_func(address);
            if (!func) {
                set_error("No function found at address", -2);
                return std::nullopt;
            }

            FunctionInfo info;
            info.address = static_cast<Address>(func->start_ea);

            // Get function name
            qstring func_name;
            get_func_name(&func_name, func->start_ea);
            info.name = qstring_to_std(func_name);

            // Get demangled name
            qstring demangled_name;
            get_demangled_name(&demangled_name, func->start_ea, 0, 0, 0);
            info.demangled_name = qstring_to_std(demangled_name);

            // Get function size
            info.size = static_cast<size_t>(func->end_ea - func->start_ea);

            // Get comment
            qstring comment;
            get_func_cmt(&comment, func, true);
            info.comment = qstring_to_std(comment);

            return info;
        }
        catch (const std::exception& e) {
            set_error("Exception getting function info: " + std::string(e.what()), -3);
            return std::nullopt;
        }
    }

    std::vector<FunctionInfo> find_functions_by_name(const std::string& pattern) const {
        std::vector<FunctionInfo> results;

        try {
            clear_error();

            func_t* func = nullptr;
            for (func = get_next_func(0); func != nullptr; func = get_next_func(func->start_ea)) {
                qstring func_name;
                get_func_name(&func_name, func->start_ea);
                std::string name = qstring_to_std(func_name);

                if (name.find(pattern) != std::string::npos) {
                    auto info = get_function_info(func->start_ea);
                    if (info) {
                        results.push_back(*info);
                    }
                }
            }
        }
        catch (const std::exception& e) {
            set_error("Exception finding functions: " + std::string(e.what()), -4);
        }

        return results;
    }

    std::vector<FunctionInfo> get_all_functions() const {
        return find_functions_by_name("*");
    }

    std::string get_function_code(ea_t address, size_t max_length) const {
        try {
            clear_error();

            func_t* func = get_func(address);
            if (!func) {
                set_error("No function at address", -5);
                return "";
            }

            std::string code;
            ea_t current_ea = func->start_ea;
            ea_t end_ea = func->end_ea;

            // Generate disassembly for function
            qstring disassembly;
            for (ea_t ea = current_ea; ea < end_ea; ea = next_head(ea, end_ea)) {
                generate_disasm_line(&disassembly, ea, GENDSM_FORCE_CODE);
                code += qstring_to_std(disassembly) + "\n";

                if (max_length > 0 && code.length() > max_length) {
                    code = code.substr(0, max_length) + "\n... (truncated)";
                    break;
                }
            }

            return code;
        }
        catch (const std::exception& e) {
            set_error("Exception getting function code: " + std::string(e.what()), -6);
            return "";
        }
    }

    std::string get_disassembly(ea_t address, size_t line_count) const {
        try {
            clear_error();

            std::string disassembly;
            ea_t current_ea = address;

            for (size_t i = 0; i < line_count; ++i) {
                qstring line;
                generate_disasm_line(&line, current_ea, GENDSM_FORCE_CODE);
                disassembly += qstring_to_std(line) + "\n";

                current_ea = next_head(current_ea, BADADDR);
                if (current_ea == BADADDR) break;
            }

            return disassembly;
        }
        catch (const std::exception& e) {
            set_error("Exception getting disassembly: " + std::string(e.what()), -7);
            return "";
        }
    }

    std::vector<Address> get_function_xrefs(ea_t address) const {
        std::vector<Address> xrefs;

        try {
            clear_error();

            // Get cross-references to this function
            xrefblk_t xb;
            for (bool ok = xb.first_to(address, XREF_ALL); ok; ok = xb.next_to()) {
                if (xb.iscode) {
                    xrefs.push_back(static_cast<Address>(xb.from));
                }
            }
        }
        catch (const std::exception& e) {
            set_error("Exception getting function xrefs: " + std::string(e.what()), -8);
        }

        return xrefs;
    }

    std::vector<Address> get_data_xrefs(ea_t address) const {
        std::vector<Address> xrefs;

        try {
            clear_error();

            // Get cross-references from this address
            xrefblk_t xb;
            for (bool ok = xb.first_from(address, XREF_ALL); ok; ok = xb.next_from()) {
                xrefs.push_back(static_cast<Address>(xb.to));
            }
        }
        catch (const std::exception& e) {
            set_error("Exception getting data xrefs: " + std::string(e.what()), -9);
        }

        return xrefs;
    }

    std::string get_function_name(ea_t address) const {
        try {
            clear_error();

            qstring func_name;
            get_func_name(&func_name, address);
            return qstring_to_std(func_name);
        }
        catch (const std::exception& e) {
            set_error("Exception getting function name: " + std::string(e.what()), -10);
            return "";
        }
    }

    bool set_function_name(ea_t address, const std::string& name) {
        try {
            clear_error();

            qstring qname = std_to_qstring(name);
            return set_name(address, qname.c_str(), SN_NOWARN) != 0;
        }
        catch (const std::exception& e) {
            set_error("Exception setting function name: " + std::string(e.what()), -11);
            return false;
        }
    }

    std::string get_comment(ea_t address) const {
        try {
            clear_error();

            qstring comment;
            get_cmt(&comment, address, false);
            return qstring_to_std(comment);
        }
        catch (const std::exception& e) {
            set_error("Exception getting comment: " + std::string(e.what()), -12);
            return "";
        }
    }

    bool set_comment(ea_t address, const std::string& comment_text) {
        try {
            clear_error();

            qstring qcomment = std_to_qstring(comment_text);
            return set_cmt(address, qcomment.c_str(), false) != 0;
        }
        catch (const std::exception& e) {
            set_error("Exception setting comment: " + std::string(e.what()), -13);
            return false;
        }
    }

    std::vector<Address> get_functions_in_range(ea_t start, ea_t end) const {
        std::vector<Address> functions;

        try {
            clear_error();

            func_t* func = nullptr;
            for (func = get_next_func(start); func != nullptr && func->start_ea < end; func = get_next_func(func->start_ea)) {
                functions.push_back(static_cast<Address>(func->start_ea));
            }
        }
        catch (const std::exception& e) {
            set_error("Exception getting functions in range: " + std::string(e.what()), -14);
        }

        return functions;
    }

    Address get_function_start(ea_t address) const {
        try {
            clear_error();

            func_t* func = get_func(address);
            return func ? static_cast<Address>(func->start_ea) : 0;
        }
        catch (const std::exception& e) {
            set_error("Exception getting function start: " + std::string(e.what()), -15);
            return 0;
        }
    }

    Address get_function_end(ea_t address) const {
        try {
            clear_error();

            func_t* func = get_func(address);
            return func ? static_cast<Address>(func->end_ea) : 0;
        }
        catch (const std::exception& e) {
            set_error("Exception getting function end: " + std::string(e.what()), -16);
            return 0;
        }
    }

    bool is_valid_address(ea_t address) const {
        return is_mapped(address);
    }

    bool is_function_address(ea_t address) const {
        return get_func(address) != nullptr;
    }

    bool is_code_address(ea_t address) const {
        return is_code(get_flags(address));
    }

    bool is_data_address(ea_t address) const {
        return is_data(get_flags(address));
    }

    std::string format_address(ea_t address) const {
        std::ostringstream ss;
        ss << "0x" << std::hex << address;
        return ss.str();
    }

    std::string get_type_info(ea_t address) const {
        try {
            clear_error();

            // Simplified type information - return basic type info
            char type_info[256];
            get_type_info(address, type_info, sizeof(type_info));
            return std::string(type_info);
        }
        catch (const std::exception& e) {
            set_error("Exception getting type info: " + std::string(e.what()), -17);
            return "";
        }
    }

    bool apply_struct_at_address(ea_t address, const std::string& struct_def) {
        try {
            clear_error();

            // Simplified struct application - would need proper implementation
            set_error("Struct application not implemented", -18);
            return false;
        }
        catch (const std::exception& e) {
            set_error("Exception applying struct: " + std::string(e.what()), -19);
            return false;
        }
    }

    std::string get_database_filename() const {
        try {
            clear_error();

            // Use get_idb_path to get the database filename
            char filename[QMAXPATH];
            get_idb_path(filename, sizeof(filename));
            return std::string(filename);
        }
        catch (const std::exception& e) {
            set_error("Exception getting database filename: " + std::string(e.what()), -20);
            return "";
        }
    }

    std::string get_root_filename() const {
        try {
            clear_error();

            // Use get_idb_path to get the root filename
            char filename[QMAXPATH];
            get_idb_path(filename, sizeof(filename));
            return std::string(filename);
        }
        catch (const std::exception& e) {
            set_error("Exception getting root filename: " + std::string(e.what()), -21);
            return "";
        }
    }

    size_t get_database_size() const {
        try {
            clear_error();

            // Return approximate database size
            return static_cast<size_t>(get_database_size());
        }
        catch (const std::exception& e) {
            set_error("Exception getting database size: " + std::string(e.what()), -22);
            return 0;
        }
    }

    std::optional<Error> get_last_error() const {
        return _last_error;
    }
};

// ============================================================================
// Public Interface Implementation
// ============================================================================

IDABridge::IDABridge() : _impl(std::make_unique<BridgeImpl>()) {}
IDABridge::~IDABridge() = default;

std::optional<IDABridge::FunctionInfo> IDABridge::get_function_info(Address address) const {
    return _impl->get_function_info(static_cast<ea_t>(address));
}

std::vector<IDABridge::FunctionInfo> IDABridge::find_functions_by_name(const std::string& pattern) const {
    return _impl->find_functions_by_name(pattern);
}

std::vector<IDABridge::FunctionInfo> IDABridge::get_all_functions() const {
    return _impl->get_all_functions();
}

std::string IDABridge::get_function_code(Address address, size_t max_length) const {
    return _impl->get_function_code(static_cast<ea_t>(address), max_length);
}

std::string IDABridge::get_disassembly(Address address, size_t line_count) const {
    return _impl->get_disassembly(static_cast<ea_t>(address), line_count);
}

std::vector<IDABridge::Address> IDABridge::get_function_xrefs(Address address) const {
    return _impl->get_function_xrefs(static_cast<ea_t>(address));
}

std::vector<IDABridge::Address> IDABridge::get_data_xrefs(Address address) const {
    return _impl->get_data_xrefs(static_cast<ea_t>(address));
}

std::string IDABridge::get_function_name(Address address) const {
    return _impl->get_function_name(static_cast<ea_t>(address));
}

bool IDABridge::set_function_name(Address address, const std::string& name) {
    return _impl->set_function_name(static_cast<ea_t>(address), name);
}

std::string IDABridge::get_comment(Address address) const {
    return _impl->get_comment(static_cast<ea_t>(address));
}

bool IDABridge::set_comment(Address address, const std::string& comment) {
    return _impl->set_comment(static_cast<ea_t>(address), comment);
}

std::vector<IDABridge::Address> IDABridge::get_functions_in_range(Address start, Address end) const {
    return _impl->get_functions_in_range(static_cast<ea_t>(start), static_cast<ea_t>(end));
}

IDABridge::Address IDABridge::get_function_start(Address address) const {
    return _impl->get_function_start(static_cast<ea_t>(address));
}

IDABridge::Address IDABridge::get_function_end(Address address) const {
    return _impl->get_function_end(static_cast<ea_t>(address));
}

std::string IDABridge::get_type_info(Address address) const {
    return _impl->get_type_info(static_cast<ea_t>(address));
}

bool IDABridge::apply_struct_at_address(Address address, const std::string& struct_def) {
    return _impl->apply_struct_at_address(static_cast<ea_t>(address), struct_def);
}

bool IDABridge::is_valid_address(Address address) const {
    return _impl->is_valid_address(static_cast<ea_t>(address));
}

bool IDABridge::is_function_address(Address address) const {
    return _impl->is_function_address(static_cast<ea_t>(address));
}

bool IDABridge::is_code_address(Address address) const {
    return _impl->is_code_address(static_cast<ea_t>(address));
}

bool IDABridge::is_data_address(Address address) const {
    return _impl->is_data_address(static_cast<ea_t>(address));
}

std::string IDABridge::format_address(Address address) const {
    return _impl->format_address(static_cast<ea_t>(address));
}

IDABridge::AnalysisContext IDABridge::gather_analysis_context(Address address, int depth) const {
    AnalysisContext context;

    try {
        auto func_info = get_function_info(address);
        if (!func_info) return context;

        // Get function code
        context.function_code = get_function_code(address, 1000);

        // Get cross-references
        auto xrefs_from = get_function_xrefs(address);
        for (Address xref : xrefs_from) {
            context.xrefs_from.push_back(format_address(xref));
        }

        auto xrefs_to = get_data_xrefs(address);
        for (Address xref : xrefs_to) {
            context.xrefs_to.push_back(format_address(xref));
        }

        // Get related functions
        auto related_functions = get_functions_in_range(address - 0x1000, address + 0x1000);
        for (Address func_addr : related_functions) {
            if (func_addr != address) {
                context.related_functions.push_back(get_function_name(func_addr));
            }
        }

        // Get type information
        context.type_info = get_type_info(address);
    }
    catch (const std::exception& e) {
        // Context gathering failed, but don't throw
        context.function_code = "Error gathering context: " + std::string(e.what());
    }

    return context;
}

std::optional<IDABridge::Error> IDABridge::get_last_error() const {
    return _impl->get_last_error();
}

void IDABridge::clear_error() {
    _impl->clear_error();
}

std::string IDABridge::get_database_filename() const {
    return _impl->get_database_filename();
}

std::string IDABridge::get_root_filename() const {
    return _impl->get_root_filename();
}

size_t IDABridge::get_database_size() const {
    return _impl->get_database_size();
}

// ============================================================================
// Global Instance
// ============================================================================

IDABridge g_ida_bridge;

// ============================================================================
// Convenience Function Implementations
// ============================================================================

namespace ida_utils {

std::string build_function_context(IDABridge::Address address, int xref_depth) {
    auto context = g_ida_bridge.gather_analysis_context(address, xref_depth);

    std::stringstream ss;
    ss << "Function Code:\n" << context.function_code << "\n\n";

    if (!context.xrefs_from.empty()) {
        ss << "Cross-references to this function:\n";
        for (const std::string& xref : context.xrefs_from) {
            ss << "  " << xref << "\n";
        }
        ss << "\n";
    }

    if (!context.xrefs_to.empty()) {
        ss << "Cross-references from this function:\n";
        for (const std::string& xref : context.xrefs_to) {
            ss << "  " << xref << "\n";
        }
        ss << "\n";
    }

    return ss.str();
}

std::string build_struct_context(IDABridge::Address address) {
    return "Struct context not implemented in bridge layer";
}

std::string build_naming_context(IDABridge::Address address) {
    auto func_info = g_ida_bridge.get_function_info(address);
    if (!func_info) return "";

    std::stringstream ss;
    ss << "Function at " << g_ida_bridge.format_address(address) << ":\n";
    ss << "Current name: " << func_info->name << "\n";
    ss << "Demangled name: " << func_info->demangled_name << "\n";
    ss << "Size: " << func_info->size << " bytes\n";
    ss << "Comment: " << func_info->comment << "\n";

    return ss.str();
}

bool is_valid_address(IDABridge::Address address) {
    return g_ida_bridge.is_valid_address(address);
}

std::string format_address(IDABridge::Address address) {
    return g_ida_bridge.format_address(address);
}

}
