// ============================================================================
// AiDA Plugin Entry Point - Main Plugin Interface
// This file serves as the main entry point for the IDA plugin
// and coordinates between the different architectural layers
// ============================================================================

#include "ida_layer/aida_pro.hpp"

// Plugin entry point - delegate to IDA layer
plugin_t PLUGIN =
{
  IDP_INTERFACE_VERSION,
  PLUGIN_MULTI,
  init,
  nullptr,
  nullptr,
  "AI-powered game reversing assistant",
  "Right-click in code views or use the Tools->AI Assistant menu",
  "AI Assistant",
  ""
};
