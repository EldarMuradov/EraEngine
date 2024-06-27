// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "scene/scene.h"
#include "core/camera_controller.h"
#include "core/system.h"
#include "undo_stack.h"
#include "transformation_gizmo.h"
#include "asset_editor_panel.h"
#include "rendering/main_renderer.h"
#include "physics/physics.h"
#include "editor_icons.h"
#include <asset/file_registry.h>
#include "core/imgui.h"

struct application;

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

	void forceStart();
	void forcePause();
	void forceStop();

	void drawSettings(float dt);
	bool drawMainMenuBar();
	bool drawSceneHierarchy();
	void renderChilds(eentity& entity);
	bool handleUserInput(const user_input& input, ldr_render_pass* ldrRenderPass, float dt);
	bool drawEntityCreationPopup();
	void drawPhysicsShapes(ldr_render_pass* renderPass);

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
	bool deserializeFromCurrentFile(const fs::path& path);

	template <typename value_t, typename action_t, typename... args_t>
	void undoable(const char* undoLabel, value_t before, value_t& now, args_t... args);

	eentity selectedColliderEntity;
	eentity selectedConstraintEntity;

	editor_scene* scene = nullptr;
	main_renderer* renderer = nullptr;
	editor_panels* editorPanels = nullptr;

	application* app = nullptr;

	undo_stack undoStacks[2];
	undo_buffer undoBuffers[2];

	undo_stack* currentUndoStack = nullptr;
	undo_buffer* currentUndoBuffer = nullptr;

	transformation_gizmo gizmo;

	camera_controller cameraController;

	vec3 selectedEntityEulerRotation;

	system_info systemInfo;
	bool renderPhysicsShapes = false;

	friend struct selection_undo;
	friend struct application;
};

inline vec3 getEuler(quat q)
{
	vec3 euler = quatToEuler(q);
	euler.x = rad2deg(angleToZeroToTwoPi(euler.x));
	euler.y = rad2deg(angleToZeroToTwoPi(euler.y));
	euler.z = rad2deg(angleToZeroToTwoPi(euler.z));
	return euler;
}

inline quat getQuat(vec3 euler)
{
	euler.x = deg2rad(euler.x);
	euler.y = deg2rad(euler.y);
	euler.z = deg2rad(euler.z);
	return eulerToQuat(euler);
}

inline void editTexture(const char* name, ref<dx_texture>& tex, uint32 loadFlags)
{
	asset_handle asset = {};
	if (tex)
	{
		asset = tex->handle;
	}

	if (ImGui::PropertyTextureAssetHandle(name, EDITOR_ICON_IMAGE, asset, tex))
	{
		fs::path path = getPathFromAssetHandle(asset);
		fs::path relative = fs::relative(path, fs::current_path());
		if (auto newTex = loadTextureFromFileAsync(relative.string(), loadFlags))
		{
			tex = newTex;
		}
	}
}

inline void editMesh(const char* name, ref<multi_mesh>& mesh, uint32 loadFlags)
{
	asset_handle asset = {};
	if (mesh)
	{
		asset = mesh->handle;
	}

	if (ImGui::PropertyAssetHandle(name, EDITOR_ICON_MESH, asset))
	{
		fs::path path = getPathFromAssetHandle(asset);
		fs::path relative = fs::relative(path, fs::current_path());
		if (auto newMesh = loadMeshFromFile(relative.string(), loadFlags))
		{
			mesh = newMesh;
		}
	}
}

inline void editMaterial(const ref<pbr_material>& material)
{
	if (ImGui::BeginProperties())
	{
		asset_handle dummy = {};

		editTexture("Albedo", material->albedo, image_load_flags_default);
		editTexture("Normal", material->normal, image_load_flags_default_noncolor);
		editTexture("Roughness", material->roughness, image_load_flags_default_noncolor);
		editTexture("Metallic", material->metallic, image_load_flags_default_noncolor);

		ImGui::PropertyColor("Emission", material->emission);
		ImGui::PropertyColor("Albedo tint", material->albedoTint);
		ImGui::PropertyDropdown("Shader", pbrMaterialShaderNames, pbr_material_shader_count, (uint32&)material->shader);
		ImGui::PropertySlider("UV scale", material->uvScale, 0.0f, 150.0f);
		ImGui::PropertySlider("Translucency", material->translucency);

		if (!material->roughness)
		{
			ImGui::PropertySlider("Roughness override", material->roughnessOverride);
		}

		if (!material->metallic)
		{
			ImGui::PropertySlider("Metallic override", material->metallicOverride);
		}

		ImGui::EndProperties();
	}
}

inline void editSubmeshTransform(trs* transform)
{
	ImGui::Drag("Position", transform->position, 0.1f);
	vec3 selectedEntityEulerRotation = getEuler(transform->rotation);

	if (ImGui::Drag("Rotation", selectedEntityEulerRotation, 0.1f))
	{
		transform->rotation = getQuat(selectedEntityEulerRotation);
	}

	ImGui::Drag("Scale", transform->scale, 0.1f);
}