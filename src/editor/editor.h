#pragma once

#include "scene/scene.h"
#include "core/camera_controller.h"
#include "core/system.h"
#include "undo_stack.h"
#include "transformation_gizmo.h"
#include "asset_editor_panel.h"
#include "rendering/main_renderer.h"
#include "physics/physics.h"

struct eeditor
{
	void initialize(editor_scene* scene, main_renderer* renderer, editor_panels* editorPanels);

	bool update(const user_input& input, ldr_render_pass* ldrRenderPass, float dt);

	void render(ldr_render_pass* ldrRenderPass, float dt);

	eentity selectedEntity;
	physics_settings physicsSettings;

private:
	struct undo_buffer
	{
		uint8 before[128];
		template <typename T> inline T& as() { return *(T*)before; }
	};

	eentity selectedColliderEntity;
	eentity selectedConstraintEntity;

	void forceStart();
	void forcePause();
	void forceStop();

	void drawSettings(float dt);
	bool drawMainMenuBar();
	bool drawSceneHierarchy();
	void renderChilds(eentity& entity);
	bool handleUserInput(const user_input& input, ldr_render_pass* ldrRenderPass, float dt);
	bool drawEntityCreationPopup();

	void updateSelectedEntityUIRotation();

	void setSelectedEntity(eentity entity);

	bool editCamera(render_camera& camera);
	bool editTonemapping(tonemap_settings& tonemap);
	bool editSunShadowParameters(directional_light& sun);
	bool editAO(bool& enable, hbao_settings& settings, const ref<dx_texture>& aoTexture);
	bool editSSS(bool& enable, sss_settings& settings, const ref<dx_texture>& sssTexture);
	bool editSSR(bool& enable, ssr_settings& settings, const ref<dx_texture>& ssrTexture);
	bool editTAA(bool& enable, taa_settings& settings, const ref<dx_texture>& velocityTexture);
	bool editBloom(bool& enable, bloom_settings& settings, const ref<dx_texture>& bloomTexture);
	bool editSharpen(bool& enable, sharpen_settings& settings);

	void onObjectMoved();

	void serializeToFile();
	bool deserializeFromFile();

	template <typename value_t, typename action_t, typename... args_t>
	void undoable(const char* undoLabel, value_t before, value_t& now, args_t... args);

	editor_scene* scene;
	main_renderer* renderer;
	editor_panels* editorPanels;

	application* app;

	undo_stack undoStacks[2];
	undo_buffer undoBuffers[2];

	undo_stack* currentUndoStack;
	undo_buffer* currentUndoBuffer;

	transformation_gizmo gizmo;

	float physicsTestForce = 1000.f;

	camera_controller cameraController;

	vec3 selectedEntityEulerRotation;

	system_info systemInfo;

	friend struct selection_undo;
	friend struct application;
};