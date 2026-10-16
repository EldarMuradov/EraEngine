// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "audio/sound_management.h"
#include "audio/audio.h"
#include "audio/sound.h"

#include "core/editor_icons.h"
#include "core/imgui.h"
#include "core/yaml.h"
#include "core/log.h"

namespace era_engine
{
    static std::unordered_map<uint64, SoundSpec> sound_registry;
    static const fs::path sound_registry_path = fs::path(L"resources/sounds.yaml").lexically_normal();

    void load_sound_registry()
    {
        std::ifstream stream(sound_registry_path);
        YAML::Node n = YAML::Load(stream);

        for (auto entry_node : n)
        {
            SoundSpec spec = {};

            YAML_LOAD(entry_node, spec.asset, "Asset");
            YAML_LOAD_ENUM(entry_node, spec.type, "Type");
            YAML_LOAD(entry_node, spec.stream, "Stream");
            YAML_LOAD(entry_node, spec.name, "Tag");

            uint64 hash = hash_string64(spec.name.c_str());

            sound_registry[hash] = spec;
        }
    }

    static void save_sound_registry()
    {
        YAML::Node out;

        for (auto [id, spec] : sound_registry)
        {
            YAML::Node n;
            n["Tag"] = spec.name;
            n["Asset"] = spec.asset;
            n["Type"] = (uint32)spec.type;
            n["Stream"] = spec.stream;

            out.push_back(n);
        }

        fs::create_directories(sound_registry_path.parent_path());

        LOG_MESSAGE("Rewriting sound registry");
        std::ofstream fout(sound_registry_path);
        fout << out;
    }

    void draw_sound_editor(bool& open)
    {
        if (open)
        {
            if (ImGui::Begin(EDITOR_ICON_AUDIO "  Sound Editor", &open))
            {
                static ImGuiTextFilter filter;
                filter.Draw();

                static bool dirty = true;

                if (ImGui::DisableableButton("Save registry", dirty))
                {
                    save_sound_registry();

                    unload_all_sounds(); // Unload, such that the spec gets reloaded when the sound is played the next time.
                    restart_all_sounds();

                    dirty = false;
                }

                for (auto& [id, spec] : sound_registry)
                {
                    std::string& name = spec.name;
                    if (filter.PassFilter(name.c_str()))
                    {
                        if (ImGui::BeginTree(name.c_str()))
                        {
                            if (ImGui::BeginProperties())
                            {
                                dirty |= ImGui::PropertyAssetHandle("Asset", EDITOR_ICON_AUDIO, spec.asset);
                                dirty |= ImGui::PropertyDropdown("Type", sound_type_names, SOUND_TYPE_COUNT, (uint32&)spec.type);
                                dirty |= ImGui::PropertyCheckbox("Stream", spec.stream);

                                ImGui::EndProperties();
                            }

                            ImGui::EndTree();
                        }
                    }
                }

                ImGui::Separator();

                static char input[128] = "";
                ImGui::InputText("##input", input, sizeof(input));
                ImGui::SameLine();
                if (ImGui::DisableableButton("Add sound", input[0] != 0))
                {
                    SoundSpec spec = {};
                    spec.name = input;

                    uint64 hash = hash_string64(input);
                    sound_registry.insert({ hash, spec });

                    input[0] = 0;
                }
            }
            ImGui::End();
        }
    }

    static SoundSpec null_spec = {};

    NODISCARD const SoundSpec& get_sound_spec(const SoundID& id)
    {
        auto it = sound_registry.find(id.hash);
        return (it != sound_registry.end()) ? it->second : null_spec;
    }
}