// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "editor/file_browser.h"
#include "editor/asset_editor_panel.h"
#include "editor/system_calls.h"

#include "core/editor_icons.h"
#include "core/imgui.h"
#include "core/project.h"
#include "core/string.h"

#include "audio/sound.h"

#include "asset/image.h"
#include "asset/model_asset.h"

#include <imgui/imgui_internal.h>

#include <shellapi.h>

namespace era_engine
{
    FileBrowser::FileBrowser()
    {
        std::string path = get_asset_path("/resources/assets");
        change_current_path(path);
    }

    static const char* get_type_icon(FileBrowser::DirEntryType type)
    {
        if (type == FileBrowser::DIR_ENTRY_TYPE_DIRECTORY)
            return ICON_FA_FOLDER_OPEN;

        if (type == FileBrowser::DIR_ENTRY_TYPE_EMPTY_DIRECTORY)
            return ICON_FA_FOLDER;

        if (type == FileBrowser::DIR_ENTRY_TYPE_MESH)
            return EDITOR_ICON_MESH;

        if (type == FileBrowser::DIR_ENTRY_TYPE_IMAGE)
            return EDITOR_ICON_IMAGE;

        if (type == FileBrowser::DIR_ENTRY_TYPE_IMAGE_HDR)
            return EDITOR_ICON_IMAGE_HDR;

        if (type == FileBrowser::DIR_ENTRY_TYPE_FONT)
            return EDITOR_ICON_FONT;

        if (type == FileBrowser::DIR_ENTRY_TYPE_AUDIO)
            return EDITOR_ICON_AUDIO;

        if (type == FileBrowser::DIR_ENTRY_TYPE_CODE)
            return ICON_FA_FILE_CODE;

        if (type == FileBrowser::DIR_ENTRY_TYPE_PREFAB)
            return EDITOR_ICON_PREFAB;

        return ICON_FA_FILE;
    }

    static bool is_directory(FileBrowser::DirEntryType type)
    {
        return type == FileBrowser::DIR_ENTRY_TYPE_DIRECTORY || type == FileBrowser::DIR_ENTRY_TYPE_EMPTY_DIRECTORY;
    }

    static bool is_file(FileBrowser::DirEntryType type)
    {
        return !is_directory(type);
    }

    void FileBrowser::draw()
    {
        static ImGuiTextFilter filter;
        static bool show_only_passing_items = true;

        if (ImGui::Begin("Assets"))
        {
            if (ImGui::DisableableButton(ICON_FA_LEVEL_UP_ALT, current_path.has_parent_path()))
            {
                change_current_path(current_path.parent_path());
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Up");
            }
            ImGui::SameLine();

            if (ImGui::Button(ICON_FA_REDO_ALT))
            {
                refresh();
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Refresh");
            }
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::Text("Current path: ");
            fs::path acc_path;
            for (auto p : current_path)
            {
                if (p == "\\")
                {
                    acc_path = std::move(acc_path / p);
                    continue;
                }

                acc_path = acc_path.empty() ? p : std::move((acc_path / p));
                ImGui::SameLine(0, 0);
                if (ImGui::SmallButton(p.string().c_str()))
                {
                    change_current_path(acc_path);
                    break;
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Change to '%s'", acc_path.u8string().c_str());
                }
                ImGui::SameLine(0, 0);
                ImGui::Text("/");
            }
            ImGui::PopStyleColor();

            ImGui::Dummy(ImVec2(0.f, ImGui::GetStyle().FramePadding.y));
            ImGui::Selectable(ICON_FA_FILTER, &show_only_passing_items, 0, ImGui::CalcTextSize(ICON_FA_FILTER));
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Show only items passing the filter search");
            }
            ImGui::SameLine();
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetStyle().FramePadding.y);
            filter.Draw("Search...");

            ImGui::Separator();

            int width = (int)ImGui::GetContentRegionAvail().x;
            int entry_width = 256;
            int num_columns = max(1, width / entry_width);

            if (ImGui::BeginTable("#direntries", num_columns))
            {
                bool skip_next_table_column = false;

                for (auto& p : current_path_entries)
                {
                    ImGui::PushID(&p);

                    if (skip_next_table_column || ImGui::TableNextColumn())
                    {
                        std::string filename = p.filename.string();

                        bool passes_search = filter.PassFilter(filename.c_str());
                        bool should_display = passes_search || !show_only_passing_items;

                        if (!should_display)
                        {
                            skip_next_table_column = true;
                            ImGui::PopID();
                            continue;
                        }
                        skip_next_table_column = false;

                        if (!passes_search)
                        {
                            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.2f);
                        }

                        char buffer[256];
                        snprintf(buffer, sizeof(buffer), "%s  %s", get_type_icon(p.type), filename.c_str());
                        ImGui::SelectableWrapped(buffer, entry_width, false);

                        if (!passes_search)
                        {
                            ImGui::PopItemFlag();
                            ImGui::PopStyleVar();
                        }

                        if (passes_search)
                        {
                            // Tooltip and directory navigation.
                            if (ImGui::IsItemHovered())
                            {
                                if (is_directory(p.type) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                                {
                                    change_current_path(current_path / p.filename);
                                    ImGui::PopID();
                                    break;
                                }
                                else if (p.type == DIR_ENTRY_TYPE_MESH || p.type == DIR_ENTRY_TYPE_IMAGE_HDR)
                                {
                                    ImGui::SetTooltip("Drag&drop into scene to instantiate.");
                                }
                                else if (p.type == DIR_ENTRY_TYPE_PREFAB)
                                {
                                    ImGui::SetTooltip("Drag&drop into scene or hierarchy to instantiate, or onto a prefab property.");
                                }
                            }

                            // Context menu
                            if (ImGui::BeginPopupContextItem(0, ImGuiPopupFlags_MouseButtonRight))
                            {
                                if (ImGui::MenuItem("Show in Explorer"))
                                {
                                    os::SystemCalls::show_in_explorer(current_path.string());
                                }

                                if (is_file(p.type) && ImGui::MenuItem("Open in default program"))
                                {
                                    fs::path full_path = current_path / p.filename;
                                    os::SystemCalls::open_file(full_path);
                                }
                                ImGui::EndPopup();
                            }

                            // Drag&drop
                            if (p.type == DIR_ENTRY_TYPE_MESH)
                            {
                                if (ImGui::BeginDragDropSource())
                                {
                                    fs::path full_path = current_path / p.filename;
                                    std::string str = full_path.string();
                                    ImGui::SetDragDropPayload(EDITOR_ICON_MESH, str.c_str(), str.length() + 1, ImGuiCond_Once);
                                    ImGui::Text("Drop into scene to instantiate.");
                                    ImGui::EndDragDropSource();
                                }
                            }
                            if (p.type == DIR_ENTRY_TYPE_IMAGE)
                            {
                                if (ImGui::BeginDragDropSource())
                                {
                                    fs::path full_path = current_path / p.filename;
                                    std::string str = full_path.string();
                                    ImGui::SetDragDropPayload(EDITOR_ICON_IMAGE, str.c_str(), str.length() + 1, ImGuiCond_Once);
                                    ImGui::EndDragDropSource();
                                }
                            }
                            if (p.type == DIR_ENTRY_TYPE_AUDIO)
                            {
                                if (ImGui::BeginDragDropSource())
                                {
                                    fs::path full_path = current_path / p.filename;
                                    std::string str = full_path.string();
                                    ImGui::SetDragDropPayload(EDITOR_ICON_AUDIO, str.c_str(), str.length() + 1, ImGuiCond_Once);
                                    ImGui::EndDragDropSource();
                                }
                            }
                            if (p.type == DIR_ENTRY_TYPE_PREFAB)
                            {
                                if (ImGui::BeginDragDropSource())
                                {
                                    fs::path full_path = current_path / p.filename;
                                    std::string str = full_path.string();
                                    ImGui::SetDragDropPayload(EDITOR_ICON_PREFAB, str.c_str(), str.length() + 1, ImGuiCond_Once);
                                    ImGui::Text("Drop into scene or hierarchy to instantiate.");
                                    ImGui::EndDragDropSource();
                                }
                            }
                            if (p.type == DIR_ENTRY_TYPE_NONE || p.type == DIR_ENTRY_TYPE_CODE || p.type == DIR_ENTRY_TYPE_FONT)
                            {
                                if (ImGui::BeginDragDropSource())
                                {
                                    fs::path full_path = current_path / p.filename;
                                    std::string str = full_path.string();
                                    ImGui::SetDragDropPayload(EDITOR_ICON_FILE, str.c_str(), str.length() + 1, ImGuiCond_Once);
                                    ImGui::Text("%s", p.filename.string().c_str());
                                    ImGui::EndDragDropSource();
                                }
                            }
                            if (p.type == DIR_ENTRY_TYPE_IMAGE_HDR)
                            {
                                if (ImGui::BeginDragDropSource())
                                {
                                    fs::path full_path = current_path / p.filename;
                                    std::string str = full_path.string();
                                    ImGui::SetDragDropPayload(EDITOR_ICON_IMAGE_HDR, str.c_str(), str.length() + 1, ImGuiCond_Once);
                                    ImGui::Text("Drop into scene to instantiate.");
                                    ImGui::EndDragDropSource();
                                }
                            }
                        }
                    }

                    ImGui::PopID();
                }

                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

    void FileBrowser::refresh()
    {
        current_path_entries.clear();
        for (auto p : fs::directory_iterator(current_path))
        {
            DirEntryType type = DIR_ENTRY_TYPE_NONE;

            if (fs::is_directory(p.path()))
            {
                uint32 num_children = (uint32)(std::distance(std::filesystem::directory_iterator{p.path()}, std::filesystem::directory_iterator{}));
                type = num_children ? DIR_ENTRY_TYPE_DIRECTORY : DIR_ENTRY_TYPE_EMPTY_DIRECTORY;
            }
            else
            {
                fs::path extension = p.path().extension();
                if (is_mesh_extension(extension))
                {
                    type = DIR_ENTRY_TYPE_MESH;
                }
                else if (is_image_extension(extension))
                {
                    type = DIR_ENTRY_TYPE_IMAGE;
                    if (extension == ".hdr")
                    {
                        type = DIR_ENTRY_TYPE_IMAGE_HDR;
                    }
                }
                else if (extension == ".ttf")
                {
                    type = DIR_ENTRY_TYPE_FONT;
                }
                else if (extension == ".cs")
                {
                    type = DIR_ENTRY_TYPE_CODE;
                }
                else if (extension == ".eprefab")
                {
                    type = DIR_ENTRY_TYPE_PREFAB;
                }
                else if (is_sound_extension(extension))
                {
                    type = DIR_ENTRY_TYPE_AUDIO;
                }
            }

            current_path_entries.push_back({p.path().filename(), type});
        }

        std::sort(current_path_entries.begin(), current_path_entries.end(), [](const DirEntry& a, const DirEntry& b)
            {
                if (is_directory(a.type) == is_directory(b.type))
                {
                    return a.filename < b.filename;
                }
                return is_directory(a.type);
            });
    }

    void FileBrowser::change_current_path(const fs::path& path)
    {
        current_path = path;
        refresh();
    }
}