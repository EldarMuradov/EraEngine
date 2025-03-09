#include "core/debug/debug_var_storage.h"
#include "core/debug/debug_var.h"

#include "core/imgui.h"

#include <mutex>

namespace era_engine
{
    std::mutex lock;

    DebugVarsStorage::DebugVarsStorage()
    {
    }

    DebugVarsStorage* DebugVarsStorage::get_instance()
    {
        static DebugVarsStorage* instance = new DebugVarsStorage();
        return instance;
    }

    void DebugVarsStorage::register_var(RegistrableVar* var)
    {
        std::lock_guard<std::mutex> _lock{lock};
        vars.emplace_back(var);
    }

    std::vector<RegistrableVar*>& DebugVarsStorage::get_vars()
    {
        return vars;
    }

    void DebugVarsStorage::reset_all()
    {
        for (RegistrableVar* var : vars)
        {
            var->reset();
        }
    }

    void DebugVarsStorage::draw_vars(bool& opened)
    {
        static char filter_string[128]{};
        static bool force_focus_filter = false;

        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Debug Vars"))
        {
            bool rebuild = false;

            float window_width = ImGui::GetWindowWidth();
            float combo_width = std::max(50.0f, window_width * 0.1f);
            float filter_width = std::max(30.0f, window_width * 0.15f);
            bool extended_view = (window_width > 400.0f);

            ImGui::AlignTextToFramePadding();

            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();

            if (ImGui::Button("Reset"))
            {
                reset_all();
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Reset to default values");
            }

            ImGui::BeginChild("##ScrollingEngineVars", ImVec2(), true, ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::Columns(2);

            int id = 0;

            for (RegistrableVar* var : vars)
            {
                rttr::variant value = var->get_value_variant();

                ImGui::AlignTextToFramePadding();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0, 1.0f));
                ImGui::TextUnformatted(var->name.c_str());
                ImGui::PopStyleColor();

                ImGui::NextColumn();
                ImGui::PushID(++id);

                rttr::type type = var->get_value_type();

                if (type == rttr::type::get<bool>())
                {
                    bool v = var->get_value_variant().get_value<bool>();
                    if (ImGui::Checkbox(var->name.c_str(), &v))
                    {
                        var->set_variant_value(rttr::variant(std::move(v)));
                    }
                }
                else if (type == rttr::type::get<int32_t>())
                {
                    int32_t v = var->get_value_variant().get_value<int32_t>();
                    if (ImGui::InputInt(var->name.c_str(), &v))
                    {
                        var->set_variant_value(rttr::variant(std::move(v)));
                    }
                }
                else if (type == rttr::type::get<uint32_t>())
                {
                    uint32_t v = var->get_value_variant().get_value<uint32_t>();
                    if (ImGui::InputInt(var->name.c_str(), reinterpret_cast<int32_t*>(&v)))
                    {
                        var->set_variant_value(rttr::variant(std::move(v)));
                    }
                }
                else if (type == rttr::type::get<float>())
                {
                    const char* float_format = "%.4f";

                    float v = var->get_value_variant().get_value<float>();
                    if (ImGui::InputFloat(var->name.c_str(), &v, 0.0f, 0.0f, float_format))
                    {
                        var->set_variant_value(rttr::variant(std::move(v)));
                    }
                }
                else if (type == rttr::type::get<vec2>())
                {
                    const char* float_format = "%.4f";

                    vec2 v = var->get_value_variant().get_value<vec2>();
                    if (ImGui::InputFloat2(var->name.c_str(), v.data, float_format))
                    {
                        var->set_variant_value(rttr::variant(std::move(v)));
                    }
                }
                else if (type == rttr::type::get<vec3>())
                {
                    const char* float_format = "%.4f";

                    vec3 v = var->get_value_variant().get_value<vec3>();
                    if (ImGui::InputFloat3(var->name.c_str(), v.data, float_format))
                    {
                        var->set_variant_value(rttr::variant(std::move(v)));
                    }
                }
                else if (type == rttr::type::get<vec4>())
                {
                    const char* float_format = "%.4f";

                    vec4 v = var->get_value_variant().get_value<vec4>();
                    if (ImGui::InputFloat4(var->name.c_str(), v.data, float_format))
                    {
                        var->set_variant_value(rttr::variant(std::move(v)));
                    }
                }

                ImGui::SameLine();
                if (ImGui::Button("X"))
                {
                    var->reset();
                }

                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Reset value to default");
                }

                ImGui::NextColumn();
                ImGui::PopID();
            }

            ImGui::Columns(1);
            ImGui::EndChild();
        }
        ImGui::End();
    }
}
