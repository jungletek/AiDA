# ============================================================================
# Service Layer CMake Configuration
# ============================================================================

# Service Layer Sources (Pure C++ with third-party libraries)
set(SERVICE_LAYER_SOURCES
    src/service_layer/ai_client.cpp
    src/service_layer/ai_provider_config.cpp
    src/service_layer/connection_pool.cpp
    src/service_layer/constants.cpp
    src/service_layer/debug_logger.cpp
    src/service_layer/request_cache.cpp
    src/service_layer/settings.cpp
    src/service_layer/settings_form_manager.cpp
    src/service_layer/unified_ai_client.cpp
)

# Verify all source files exist
foreach(source_file ${SERVICE_LAYER_SOURCES})
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${source_file})
        message(WARNING "Service layer source file not found: ${source_file}")
    endif()
endforeach()

# ============================================================================
# Third-Party Dependencies Target
# ============================================================================

# Create interface library for third-party dependencies
add_library(AiDA_thirdparty_dependencies INTERFACE)

# Link nlohmann/json and cpp-httplib to the interface library
target_link_libraries(AiDA_thirdparty_dependencies INTERFACE
    nlohmann_json::nlohmann_json
    httplib::httplib
)

# Include directories for third-party libraries
target_include_directories(AiDA_thirdparty_dependencies INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ============================================================================
# Service Layer Target
# ============================================================================

add_library(AiDA_service_layer SHARED
    ${SERVICE_LAYER_SOURCES}
    $<TARGET_OBJECTS:AiDA_shared>
)

target_include_directories(AiDA_service_layer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# CRITICAL: Service Layer uses standard C++ - SELECTIVE MACRO OVERRIDES
target_compile_definitions(AiDA_service_layer PRIVATE
    # Standard C++ definitions (WIN32_LEAN_AND_MEAN defined by thirdparty_compat.hpp)
    # WIN32_LEAN_AND_MEAN  # ❌ Removed - defined by thirdparty_compat.hpp
    NOMINMAX
    CPPHTTPLIB_OPENSSL_SUPPORT

    # Force include order for Windows headers
    _WINSOCKAPI_ _WINSOCK_H

    # SELECTIVE MACRO OVERRIDES - Only override macros that actually conflict
    # snprintf=ida_snprintf vsnprintf=ida_vsnprintf  # ❌ Removed - breaks std library
    # swprintf=ida_swprintf vswprintf=ida_vswprintf  # ❌ Removed - breaks std library
    # wait=dont_use_wait pid_t=ida_pid_t            # ❌ Removed - not needed here
    # MD5=ida_MD5 MD5_CTX=ida_MD5_CTX              # ❌ Removed - use safe replacements
)

# Minimal compiler-level macro overrides for service layer
if(MSVC)
    target_compile_options(AiDA_service_layer PRIVATE
        # Minimal Windows compatibility flags
        -DNOMINMAX
        # -DWIN32_LEAN_AND_MEAN  # ❌ Removed - defined by thirdparty_compat.hpp
        -D_WINSOCKAPI_
        # Suppress some common warnings
        -wd4005  # Macro redefinition warnings
        -wd4996  # Deprecated function warnings
    )
endif()

# Link third-party dependencies to service layer
target_link_libraries(AiDA_service_layer PRIVATE
    AiDA_thirdparty_dependencies
    OpenSSL::SSL
    OpenSSL::Crypto
    ws2_32.lib
    crypt32.lib
)
