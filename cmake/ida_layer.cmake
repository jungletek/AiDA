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
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

# IDA layer links against IDA SDK
target_link_libraries(AiDA_ida_layer PRIVATE ${IDA_LIBRARY})
