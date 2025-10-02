#pragma once

#include <memory>
#include <vector>

#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>

#include "../service_layer/unified_ai_client.hpp"
// Forward declaration to avoid circular dependency
class UnifiedAIClient;

class aida_plugin_t : public plugmod_t
{
public:
    std::unique_ptr<UnifiedAIClient> ai_client;
    qstrvec_t actions_list;

    aida_plugin_t();
    ~aida_plugin_t() override;

    bool idaapi run(size_t arg) override;
    void reinit_ai_client();

private:
    void register_actions();
    void unregister_actions();
};
