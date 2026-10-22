#pragma once

#include <ecs/system.h>
#include <ecs/entity.h>

#include <core/math.h>

#include <ecs/editor/scene_editor_panels.h>

#include <memory>

namespace era_engine
{
    class RendererHolderRootComponent;
    class VehicleImportPanel;

    class EditorToolsSystem final : public System
    {
    public:
        EditorToolsSystem(World* _world);
        ~EditorToolsSystem() override;

        void init() override;
        void update(float dt) override;

        ERA_VIRTUAL_REFLECT(System)

    private:
        void draw_world();

        void draw_main_menu();
        void draw_settings();

    private:
        RendererHolderRootComponent* renderer_holder_rc = nullptr;

        std::unique_ptr<SceneEditorPanels> scene_panels;
        std::unique_ptr<VehicleImportPanel> vehicle_import_panel;
    };
}