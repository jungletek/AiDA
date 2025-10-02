# ============================================================================
# Shared Utilities CMake Configuration
# ============================================================================

# Shared Utility Sources (Used by multiple layers)
set(SHARED_SOURCES
    src/string_utils.cpp
    src/ida_utils.cpp
)

# Verify all source files exist
foreach(source_file ${SHARED_SOURCES})
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${source_file})
        message(WARNING "Shared source file not found: ${source_file}")
    endif()
endforeach()

# ============================================================================
# Shared Utilities Target
# ============================================================================

add_library(AiDA_shared OBJECT ${SHARED_SOURCES})

# Shared utilities need IDA SDK access for ida_utils.cpp
target_include_directories(AiDA_shared PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${IDASDK_INCLUDE_DIR}
)

# Shared utilities need IDA SDK definitions (WIN32_LEAN_AND_MEAN defined by IDA SDK)
target_compile_definitions(AiDA_shared PRIVATE
    # WIN32_LEAN_AND_MEAN  # ❌ Removed - already defined by IDA SDK
    NOMINMAX
    # IDA SDK compatibility definitions
    __NT__ __IDP__ __X64__ __EA64__
    _CRT_SECURE_NO_WARNINGS _SCL_SECURE_NO_WARNINGS
)

# Suppress known IDA SDK conflicts
if(MSVC)
    target_compile_options(AiDA_shared PRIVATE
        -wd4005  # Macro redefinition warnings
        -wd4996  # Deprecated function warnings
        -wd4068  # Unknown pragma warnings
    )
endif()
