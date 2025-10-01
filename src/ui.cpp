#include "aida_pro.hpp"
#include "settings_form_manager.hpp"
#include <moves.hpp>

static bool idaapi handle_viewer_dblclick(TWidget* viewer, int /*shift*/, void* /*ud*/)
{
    qstring word;
    if (get_highlight(&word, viewer, nullptr))
    {
        ea_t ea = BADADDR;
        if (atoea(&ea, word.c_str()))
        {
            jumpto(ea);
            return true;
        }

        ea = get_name_ea(get_screen_ea(), word.c_str());
        if (ea != BADADDR)
        {
            jumpto(ea);
            return true;
        }
    }

    return false;
}



// Refactored settings form using the new SettingsFormManager
void SettingsForm::show_and_apply(aida_plugin_t* plugin_instance)
{
    SettingsFormData form_data;

    // Load current settings into form data
    if (!SettingsFormManager::load_form_data_from_settings(g_settings, form_data)) {
        warning("AI Assistant: Failed to load current settings for form display.");
        return;
    }

    // Show the settings dialog
    bool form_accepted = SettingsFormManager::show_settings_dialog(form_data);

    if (form_accepted) {
        // Validate the form data
        if (!SettingsFormManager::validate_form_data(form_data)) {
            return; // Validation failed, user was already warned
        }

        // Apply form data to settings
        if (!SettingsFormManager::apply_form_data_to_settings(form_data, g_settings)) {
            warning("AI Assistant: Failed to apply form data to settings.");
            return;
        }

        // Save settings and reinitialize
        g_settings.save();

        if (plugin_instance) {
            msg("AI Assistant: Settings updated. Re-initializing AI client...\n");
            plugin_instance->reinit_ai_client();
        }
    }
}

void idaapi close_handler(TWidget* /*cv*/, void* ud)
{
    strvec_t* lines_ptr = (strvec_t*)ud;
    delete lines_ptr;
}

void show_text_in_viewer(const char* title, const std::string& text_content)
{
    if (text_content.empty() || text_content.find_first_not_of(" \t\n\r") == std::string::npos)
    {
        warning("AI returned an empty or whitespace-only response. Nothing to display.");
        return;
    }

    TWidget* existing_viewer = find_widget(title);
    if (existing_viewer)
    {
        close_widget(existing_viewer, WCLS_SAVE);
    }

    strvec_t* lines_ptr = new strvec_t();

    std::string marked_up_content = ida_utils::markup_text_with_addresses(text_content);

    std::stringstream ss(marked_up_content);
    std::string line;
    while (std::getline(ss, line, '\n'))
    {
        lines_ptr->push_back(simpleline_t(line.c_str()));
    }

    // Initialize simpleline_place_t objects explicitly to avoid equality operator generation
    simpleline_place_t s1;
    simpleline_place_t s2;
    memset(&s1, 0, sizeof(s1));
    memset(&s2, 0, sizeof(s2));
    s2.n = lines_ptr->empty() ? 0 : static_cast<uint32>(lines_ptr->size() - 1);

    TWidget* viewer = create_custom_viewer(title, &s1, &s2, &s1, nullptr, lines_ptr, nullptr, nullptr);
    if (viewer == nullptr)
    {
        warning("Could not create viewer '%s'.", title);
        delete lines_ptr;
        return;
    }

    static custom_viewer_handlers_t handlers(
        nullptr, // keydown
        nullptr, // popup
        nullptr, // mouse_moved
        nullptr, // click
        handle_viewer_dblclick, // dblclick
        nullptr, // curpos
        close_handler, // close
        nullptr, // help
        nullptr, // adjust_place
        nullptr, // get_place_xcoord
        nullptr, // location_changed
        nullptr); // can_navigate

    set_custom_viewer_handlers(viewer, &handlers, lines_ptr);

    display_widget(viewer, WOPN_DP_TAB | WOPN_RESTORE);
}

static int idaapi finish_populating_widget_popup(TWidget* widget, TPopupMenu* popup_handle, const action_activation_ctx_t* ctx)
{
    if (ctx == nullptr || (ctx->widget_type != BWN_PSEUDOCODE && ctx->widget_type != BWN_DISASM))
        return 0;

    struct menu_item_t
    {
        const char* action_name;
        const char* path; // nullptr for separator
    };

    static const menu_item_t menu_items[] = {
        { "ai_assistant:analyze",      "Analyze/" },
        { "ai_assistant:rename",       "Analyze/" },
        { "ai_assistant:rename_all",   "Analyze/" },
        { "ai_assistant:comment",      "Analyze/" },
        { "ai_assistant:gen_struct",   "Generate/" },
        { "ai_assistant:gen_hook",     "Generate/" },
        { nullptr,                     nullptr }, // Separator
        { "ai_assistant:scan_for_offsets", "" },
        { "ai_assistant:custom_query", "" },
        { "ai_assistant:copy_context", "" },
        { nullptr,                     nullptr }, // Separator
        { "ai_assistant:settings",     "" },
    };

    const char* menu_root = "AI Assistant/";
    for (const auto& item : menu_items)
    {
        qstring full_path;
        if (item.path != nullptr)
            full_path.append(menu_root).append(item.path);
        
        attach_action_to_popup(widget, popup_handle, item.action_name, full_path.c_str());
    }

    return 0;
}

ssize_t idaapi ui_callback(void* /*user_data*/, int notification_code, va_list va)
{
    if (notification_code == ui_finish_populating_widget_popup)
    {
        TWidget* widget = va_arg(va, TWidget*);
        TPopupMenu* popup_handle = va_arg(va, TPopupMenu*);
        const action_activation_ctx_t* ctx = va_arg(va, const action_activation_ctx_t*);
        return finish_populating_widget_popup(widget, popup_handle, ctx);
    }
    return 0;
}
