// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core/camera_controller.h"
#include "core/system_info.h"
#include "core/imgui.h"

#include "core/editor_icons.h"
#include "editor/undo_stack.h"
#include "editor/transformation_gizmo.h"
#include "editor/asset_editor_panel.h"

#include "rendering/main_renderer.h"

#include "ecs/editor/editor_scene.h"

#include "asset/file_registry.h"
#include "dx/dx_texture.h"

namespace era_engine
{
    struct EEditor
    {
        void initialize(ref<EditorScene> _scene, MainRenderer* _renderer, EditorPanels* _editor_panels);

        bool update(const UserInput& input, LDRRenderPass* ldr_render_pass, float dt);

        void render(LDRRenderPass* ldr_render_pass, float dt);

        Entity selected_entity = Entity::null;

    private:
        struct UndoBuffer
        {
            uint8 before[128];
            template <typename T>
            inline T& as()
            {
                return *(T*)before;
            }
        };

        void force_start();
        void force_pause();
        void force_stop();

        void visualize_physics(LDRRenderPass* ldr_render_pass) const;

        void draw_settings(float dt);
        bool draw_main_menu_bar();
        bool draw_scene_hierarchy();
        void render_childs(Entity& entity);
        bool handle_user_input(const UserInput& input, LDRRenderPass* ldr_render_pass, float dt);
        bool draw_entity_creation_popup();

        void update_selected_entity_ui_rotation();

        void set_selected_entity(const Entity& entity);

        bool edit_camera(RenderCamera& camera);
        bool edit_tonemapping(TonemapSettings& tonemap);
        bool edit_sun_shadow_parameters(DirectionalLight& sun);
        bool edit_ao(bool& enable, HBAOSettings& settings, const ref<DXTexture>& ao_texture);
        bool edit_sss(bool& enable, SSSSettings& settings, const ref<DXTexture>& sss_texture);
        bool edit_ssr(bool& enable, SSRSettings& settings, const ref<DXTexture>& ssr_texture);
        bool edit_taa(bool& enable, TAASettings& settings, const ref<DXTexture>& velocity_texture);
        bool edit_bloom(bool& enable, BloomSettings& settings, const ref<DXTexture>& bloom_texture);
        bool edit_sharpen(bool& enable, SharpenSettings& settings);

        void on_object_moved();

        void serialize_to_file();
        bool deserialize_from_file();
        bool deserialize_from_current_file(const fs::path& path);

        template <typename Value_, typename Action_, typename... Args_>
        void undoable(const char* undo_label, Value_ before, Value_& now, Args_... args);

        ref<EditorScene> scene = nullptr;

        MainRenderer* renderer = nullptr;
        EditorPanels* editor_panels = nullptr;

        UndoStack undo_stacks[2];
        UndoBuffer undo_buffers[2];

        UndoStack* current_undo_stack = nullptr;
        UndoBuffer* current_undo_buffer = nullptr;

        TransformationGizmo gizmo;

        CameraController camera_controller;

        vec3 selected_entity_euler_rotation;

        SystemInfo system_info;

        bool render_physics_shapes = false;

        friend struct SelectionUndo;
    };

    void edit_texture(const char* name, ref<DXTexture>& tex, uint32 load_flags);

    void edit_mesh(const char* name, ref<MultiMesh>& mesh, uint32 load_flags);

    void edit_material(const ref<PBRMaterial>& material);

    void edit_submesh_transform(trs* transform);
}