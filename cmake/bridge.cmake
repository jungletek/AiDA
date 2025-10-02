# ============================================================================
# Bridge Layer CMake Configuration
# ============================================================================

# Bridge Layer Sources (Communication layer)
set(BRIDGE_LAYER_SOURCES
    src/bridge/ida_bridge.cpp
    src/bridge/ida_interface.cpp
)

# Verify all source files exist
foreach(source_file ${BRIDGE_LAYER_SOURCES})
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${source_file})
        message(WARNING "Bridge layer source file not found: ${source_file}")
    endif()
endforeach()

# ============================================================================
# Bridge Layer Target
# ============================================================================

add_library(AiDA_bridge SHARED
    ${BRIDGE_LAYER_SOURCES}
    $<TARGET_OBJECTS:AiDA_shared>
)

target_include_directories(AiDA_bridge PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${IDASDK_INCLUDE_DIR}
)

# Bridge layer uses IDA SDK but with macro isolation
target_compile_definitions(AiDA_bridge PRIVATE
    __NT__
    __IDP__
    __X64__
    __EA64__
    _CRT_SECURE_NO_WARNINGS
    _SCL_SECURE_NO_WARNINGS
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    # Enable macro isolation for bridge layer
    _IDA_SDK_ISOLATION_LAYER_
)
