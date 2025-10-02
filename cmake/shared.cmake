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

target_include_directories(AiDA_shared PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Shared utilities use standard C++ - no IDA SDK dependencies
target_compile_definitions(AiDA_shared PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)
