// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core/camera.h"
#include "core/camera_controller.h"

#include "dx/dx_texture.h"

#include "rendering/main_renderer.h"

#include "ecs/editor/editor_scene.h"

namespace era_engine
{
	struct EditorScene;

	struct AssetEditorPanel
	{
		virtual void begin_frame();
		virtual void end_frame() {}

		bool is_open() const { return window_open; }
		void open();
		void close();

	private:
		virtual void edit(uint32 render_width, uint32 render_height) = 0;
		virtual ref<DXTexture> get_rendering() = 0;
		virtual void set_drag_drop_data(void* data, uint32 size) {}

	protected:
		const char* title;
		const char* drag_drop_target;

		bool window_open = false;
		bool window_open_internal = false;
	};

	struct MeshEditorPanel : AssetEditorPanel
	{
		MeshEditorPanel();

		virtual void begin_frame() override;
		virtual void end_frame() override;

		void set_mesh(ref<MultiMesh> m) { mesh = m; }
		void set_scene(ref<EditorScene> _scene) { scene = _scene; }

		bool is_hovered() const { return hovered; }

		RenderCamera camera;

	private:
		virtual void edit(uint32 render_width, uint32 render_height) override;
		virtual ref<DXTexture> get_rendering() override;
		virtual void set_drag_drop_data(void* data, uint32 size) override;

		ref<MultiMesh> mesh;
		ref<EditorScene> scene = nullptr;
		MainRenderer renderer;

		UserInput input = {};

		CameraController controller;

		OpaqueRenderPass render_pass;

		uint32 width;
		uint32 height;

		bool app_focused_last_frame = true;
		bool hovered = false;
	};

	struct EditorPanels
	{
		MeshEditorPanel mesh_editor;
	};
}