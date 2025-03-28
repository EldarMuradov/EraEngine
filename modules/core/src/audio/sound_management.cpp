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
    static std::unordered_map<uint64, sound_spec> soundRegistry;
    static const fs::path soundRegistryPath = fs::path(L"resources/sounds.yaml").lexically_normal();

    void loadSoundRegistry()
    {
        std::ifstream stream(soundRegistryPath);
        YAML::Node n = YAML::Load(stream);

        for (auto entryNode : n)
        {
            sound_spec spec = {};

            YAML_LOAD(entryNode, spec.asset, "Asset");
            YAML_LOAD_ENUM(entryNode, spec.type, "Type");
            YAML_LOAD(entryNode, spec.stream, "Stream");
            YAML_LOAD(entryNode, spec.name, "Tag");

            uint64 hash = hash_string64(spec.name.c_str());

            soundRegistry[hash] = spec;
        }
    }

    static void saveSoundRegistry()
    {
        YAML::Node out;

        for (auto [id, spec] : soundRegistry)
        {
            YAML::Node n;
            n["Tag"] = spec.name;
            n["Asset"] = spec.asset;
            n["Type"] = (uint32)spec.type;
            n["Stream"] = spec.stream;

            out.push_back(n);
        }

        fs::create_directories(soundRegistryPath.parent_path());

        LOG_MESSAGE("Rewriting sound registry");
        std::ofstream fout(soundRegistryPath);
        fout << out;
    }

    void drawSoundEditor(bool& open)
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
                    saveSoundRegistry();

                    unloadAllSounds(); // Unload, such that the spec gets reloaded when the sound is played the next time.
                    restartAllSounds();

                    dirty = false;
                }

                for (auto& [id, spec] : soundRegistry)
                {
                    std::string& name = spec.name;
                    if (filter.PassFilter(name.c_str()))
                    {
                        if (ImGui::BeginTree(name.c_str()))
                        {
                            if (ImGui::BeginProperties())
                            {
                                dirty |= ImGui::PropertyAssetHandle("Asset", EDITOR_ICON_AUDIO, spec.asset);
                                dirty |= ImGui::PropertyDropdown("Type", soundTypeNames, sound_type_count, (uint32&)spec.type);
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
                    sound_spec spec = {};
                    spec.name = input;

                    uint64 hash = hash_string64(input);
                    soundRegistry.insert({ hash, spec });

                    input[0] = 0;
                }
            }
            ImGui::End();
        }
    }

    static sound_spec nullSpec = {};

    NODISCARD const sound_spec& getSoundSpec(const sound_id& id)
    {
        auto it = soundRegistry.find(id.hash);
        return (it != soundRegistry.end()) ? it->second : nullSpec;
    }
}