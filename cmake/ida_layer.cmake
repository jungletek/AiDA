# ============================================================================
# IDA Layer CMake Configuration
# ============================================================================

# IDA Layer Sources (IDA SDK dependent)
set(IDA_LAYER_SOURCES
    src/ida_layer/aida.cpp
    src/ida_layer/actions.cpp
    src/ida_layer/ui.cpp
)

# Verify all source files exist
foreach(source_file ${IDA_LAYER_SOURCES})
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${source_file})
        message(WARNING "IDA layer source file not found: ${source_file}")
    endif()
endforeach()

# ============================================================================
# IDA Layer Target
# ============================================================================

add_library(AiDA_ida_layer SHARED
    ${IDA_LAYER_SOURCES}
    $<TARGET_OBJECTS:AiDA_shared>
)

target_include_directories(AiDA_ida_layer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${IDASDK_INCLUDE_DIR}
)

# CRITICAL: IDA Layer uses IDA SDK macros - NO macro overrides
target_compile_definitions(AiDA_ida_layer PRIVATE
    __NT__
    __IDP__
    __X64__
    __EA64__
    _CRT_SECURE_NO_WARNINGS
    _SCL_SECURE_NO_WARNINGS
    # WIN32_LEAN_AND_MEAN  # ❌ Removed - defined by IDA SDK
    NOMINMAX
)

# Suppress known IDA SDK conflicts
if(MSVC)
    target_compile_options(AiDA_ida_layer PRIVATE
        -wd4005  # Macro redefinition warnings
        -wd4996  # Deprecated function warnings
        -wd4068  # Unknown pragma warnings
    )
endif()

# IDA layer links against IDA SDK
target_link_libraries(AiDA_ida_layer PRIVATE ${IDA_LIBRARY})
