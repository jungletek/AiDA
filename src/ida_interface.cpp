#include "ida_interface.hpp"

// IDA SDK headers - only included here to isolate from modern libraries
#include <ida.hpp>
#include <kernwin.hpp>
#include <funcs.hpp>
#include <name.hpp>
#include <xref.hpp>
#include <struct.hpp>
// IDA SDK compatibility fix for missing struct.hpp
#ifndef HAVE_STRUCT_HPP
// Define minimal struct functionality if header is missing
typedef struct _struc_t struc_t;
#endif
#include <typeinf.hpp>
#include <nalt.hpp>
#include <bytes.hpp>
#include <auto.hpp>

#include <sstream>
#include <regex>
#include <algorithm>

// IDAInterface Implementation
class IDAInterface::Impl {
public:
    Impl() {
        // IDA SDK initialization if needed
    }

    ~Impl() = default;

    // Type conversion helpers
    std::string qstring_to_std(const qstring& qs) const {
        return std::string(qs.c_str());
    }

    qstring std_to_qstring(const std::string& s) const {
        return qstring(s.c_str());
    }

    // IDA SDK wrapper functions
    std::optional<FunctionInfo> get_function_info(ea_t address) const {
        if (!is_mapped(address)) return std::nullopt;

        func_t* func = get_func(address);
        if (!func) return std::nullopt;

        FunctionInfo info;
        info.address = static_cast<Address>(func->start_ea);
        info.name = qstring_to_std(get_func_name(func->start_ea));
        info.demangled_name = qstring_to_std(get_demangled_name(func->start_ea));
        info.size = static_cast<size_t>(func->end_ea - func->start_ea);

        return info;
    }

    std::vector<FunctionInfo> find_functions_by_name(const std::string& name_pattern) const {
        std::vector<FunctionInfo> results;

        // This is a simplified implementation
        // In practice, you'd need to iterate through all functions
        for (ea_t ea = get_next_func(0); ea != BADADDR; ea = get_next_func(ea)) {
            qstring func_name = get_func_name(ea);
            std::string name = qstring_to_std(func_name);

            if (name.find(name_pattern) != std::string::npos) {
                auto info = get_function_info(ea);
                if (info) {
                    results.push_back(*info);
                }
            }
        }

        return results;
    }

    std::string get_function_code(ea_t address, size_t max_length) const {
        func_t* func = get_func(address);
        if (!func) return "";

        std::string code;

        // Generate disassembly
        qstring disassembly;
        generate_disasm_line(&disassembly, address, GENDSM_FORCE_CODE);

        code = qstring_to_std(disassembly);

        if (max_length > 0 && code.length() > max_length) {
            code = code.substr(0, max_length) + "\n... (truncated)";
        }

        return code;
    }

    std::string get_function_name(ea_t address) const {
        return qstring_to_std(get_func_name(address));
    }

    bool set_function_name(ea_t address, const std::string& name) {
        qstring qname = std_to_qstring(name);
        return set_name(address, qname.c_str(), SN_NOWARN);
    }

    std::string get_comment(ea_t address) const {
        return qstring_to_std(get_cmt(address));
    }

    bool set_comment(ea_t address, const std::string& comment) {
        qstring qcomment = std_to_qstring(comment);
        return set_cmt(address, qcomment.c_str(), false);
    }

    bool is_valid_address(ea_t address) const {
        return is_mapped(address);
    }

    bool is_function_address(ea_t address) const {
        return get_func(address) != nullptr;
    }

    std::string format_address(ea_t address) const {
        std::stringstream ss;
        ss << "0x" << std::hex << address;
        return ss.str();
    }

    Address get_function_start(ea_t address) const {
        func_t* func = get_func(address);
        return func ? static_cast<Address>(func->start_ea) : 0;
    }

    Address get_function_end(ea_t address) const {
        func_t* func = get_func(address);
        return func ? static_cast<Address>(func->end_ea) : 0;
    }
};

// IDAInterface public implementation
IDAInterface::IDAInterface() : _impl(std::make_unique<Impl>()) {}
IDAInterface::~IDAInterface() = default;

std::optional<IDAInterface::FunctionInfo> IDAInterface::get_function_info(Address address) const {
    return _impl->get_function_info(static_cast<ea_t>(address));
}

std::vector<IDAInterface::FunctionInfo> IDAInterface::find_functions_by_name(const std::string& name_pattern) const {
    return _impl->find_functions_by_name(name_pattern);
}

std::vector<IDAInterface::FunctionInfo> IDAInterface::get_all_functions() const {
    return _impl->find_functions_by_name("*");  // Simplified - return all functions
}

std::string IDAInterface::get_function_code(Address address, size_t max_length) const {
    return _impl->get_function_code(static_cast<ea_t>(address), max_length);
}

std::vector<IDAInterface::Address> IDAInterface::get_function_xrefs(Address address) const {
    std::vector<Address> xrefs;

    // Get cross-references to this function
    ea_t ea = static_cast<ea_t>(address);
    for (bool ok = true; ok; ok = get_next_cref_to(ea)) {
        xrefs.push_back(static_cast<Address>(ea));
    }

    return xrefs;
}

std::vector<IDAInterface::Address> IDAInterface::get_data_xrefs(Address address) const {
    std::vector<Address> xrefs;

    // Get cross-references from this address
    ea_t ea = static_cast<ea_t>(address);
    for (bool ok = true; ok; ok = get_next_cref_from(ea)) {
        xrefs.push_back(static_cast<Address>(ea));
    }

    return xrefs;
}

std::string IDAInterface::get_function_name(Address address) const {
    return _impl->get_function_name(static_cast<ea_t>(address));
}

bool IDAInterface::set_function_name(Address address, const std::string& name) {
    return _impl->set_function_name(static_cast<ea_t>(address), name);
}

std::string IDAInterface::get_comment(Address address) const {
    return _impl->get_comment(static_cast<ea_t>(address));
}

bool IDAInterface::set_comment(Address address, const std::string& comment) {
    return _impl->set_comment(static_cast<ea_t>(address), comment);
}

std::vector<IDAInterface::Address> IDAInterface::get_functions_in_range(Address start, Address end) const {
    std::vector<Address> functions;

    for (ea_t ea = static_cast<ea_t>(start); ea < static_cast<ea_t>(end); ea = get_next_func(ea)) {
        if (ea >= static_cast<ea_t>(end)) break;
        functions.push_back(static_cast<Address>(ea));
    }

    return functions;
}

IDAInterface::Address IDAInterface::get_function_start(Address address) const {
    return _impl->get_function_start(static_cast<ea_t>(address));
}

IDAInterface::Address IDAInterface::get_function_end(Address address) const {
    return _impl->get_function_end(static_cast<ea_t>(address));
}

std::string IDAInterface::get_type_info(Address address) const {
    // Simplified type information
    return "Type info not implemented in interface layer";
}

bool IDAInterface::apply_struct_at_address(Address address, const std::string& struct_definition) {
    // Simplified struct application
    return false;  // Not implemented in interface layer
}

bool IDAInterface::is_valid_address(Address address) const {
    return _impl->is_valid_address(static_cast<ea_t>(address));
}

bool IDAInterface::is_function_address(Address address) const {
    return _impl->is_function_address(static_cast<ea_t>(address));
}

std::string IDAInterface::format_address(Address address) const {
    return _impl->format_address(static_cast<ea_t>(address));
}

IDAInterface::AnalysisContext IDAInterface::gather_analysis_context(Address address, int depth) const {
    AnalysisContext context;

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

    // Get related functions (simplified)
    auto related_functions = get_functions_in_range(address - 0x1000, address + 0x1000);
    for (Address func_addr : related_functions) {
        if (func_addr != address) {
            context.related_functions.push_back(get_function_name(func_addr));
        }
    }

    return context;
}

std::optional<IDAInterface::Error> IDAInterface::get_last_error() const {
    // Simplified error handling
    return std::nullopt;
}

// Global instance
IDAInterface g_ida_interface;

// Convenience function implementations
namespace ida_utils {

std::string get_function_name(IDAInterface::Address address) {
    return g_ida_interface.get_function_name(address);
}

std::string get_function_code(IDAInterface::Address address, size_t max_length) {
    return g_ida_interface.get_function_code(address, max_length);
}

bool set_function_name(IDAInterface::Address address, const std::string& name) {
    return g_ida_interface.set_function_name(address, name);
}

std::string build_function_context(IDAInterface::Address address, int xref_depth) {
    auto context = g_ida_interface.gather_analysis_context(address, xref_depth);

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

std::string build_struct_context(IDAInterface::Address address) {
    return "Struct context not implemented in interface layer";
}

std::string build_naming_context(IDAInterface::Address address) {
    auto func_info = g_ida_interface.get_function_info(address);
    if (!func_info) return "";

    std::stringstream ss;
    ss << "Function at " << g_ida_interface.format_address(address) << ":\n";
    ss << "Current name: " << func_info->name << "\n";
    ss << "Demangled name: " << func_info->demangled_name << "\n";
    ss << "Size: " << func_info->size << " bytes\n";

    return ss.str();
}

}
