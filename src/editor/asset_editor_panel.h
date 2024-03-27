// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include "dx/dx_texture.h"
#include "core/camera.h"
#include "rendering/main_renderer.h"
#include <scene/scene.h>
#include <core/camera_controller.h>

struct asset_editor_panel
{
	virtual void beginFrame();
	virtual void endFrame() {}

	bool isOpen() const { return windowOpen; }
	void open();
	void close();

protected:
	const char* title;
	const char* dragDropTarget;

	bool windowOpen = false;
	bool windowOpenInternal = false;

private:
	virtual void edit(uint32 renderWidth, uint32 renderHeight) = 0;
	virtual ref<dx_texture> getRendering() = 0;
	virtual void setDragDropData(void* data, uint32 size) {}

};

struct mesh_editor_panel : asset_editor_panel
{
	mesh_editor_panel();

	virtual void beginFrame() override;
	virtual void endFrame() override;

	void setMesh(ref<multi_mesh> m) noexcept { mesh = m; }
	void setScene(editor_scene* scene) noexcept { this->scene = scene; }

	bool isHovered() const noexcept { return hovered; }

	render_camera camera;

private:
	virtual void edit(uint32 renderWidth, uint32 renderHeight) override;
	virtual ref<dx_texture> getRendering() override;
	virtual void setDragDropData(void* data, uint32 size) override;

	ref<multi_mesh> mesh;
	editor_scene* scene = nullptr;
	main_renderer renderer;

	uint32 width;
	uint32 height;

	user_input input = {};
	bool appFocusedLastFrame = true;
	bool hovered = false;

	camera_controller controller;

	opaque_render_pass renderPass;
};

struct editor_panels
{
	mesh_editor_panel meshEditor;
};