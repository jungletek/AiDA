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
# Service Layer Target
# ============================================================================

add_library(AiDA_service_layer SHARED
    ${SERVICE_LAYER_SOURCES}
    $<TARGET_OBJECTS:AiDA_shared>
)

target_include_directories(AiDA_service_layer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# CRITICAL: Service Layer uses standard C++ - OVERRIDE IDA SDK macros
target_compile_definitions(AiDA_service_layer PRIVATE
    # Override IDA SDK macros with standard library functions
    snprintf=snprintf
    vsnprintf=vsnprintf
    MD5=MD5
    MD5_CTX=MD5_CTX
    # Standard C++ definitions
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    CPPHTTPLIB_OPENSSL_SUPPORT
)

# Force macro overrides at compiler level for service layer
if(MSVC)
    target_compile_options(AiDA_service_layer PRIVATE
        -Dsnprintf=snprintf
        -Dvsnprintf=vsnprintf
        -DMD5=MD5
        -DMD5_CTX=MD5_CTX
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
