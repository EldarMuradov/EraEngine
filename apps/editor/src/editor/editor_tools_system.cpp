#include "editor/editor_tools_system.h"
#include "editor/file_utils.h"

#include <engine/engine.h>

#include <core/editor_icons.h>
#include <core/imgui.h>

#include <rendering/ecs/renderer_holder_root_component.h>

#include <ecs/update_groups.h>
#include <ecs/base_components/base_components.h>
#include <ecs/rendering/mesh_component.h>
#include <ecs/editor/entity_editor_utils.h>

#include <rttr/policy.h>
#include <rttr/registration>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui_internal.h>

namespace era_engine
{

	RTTR_REGISTRATION
	{
		using namespace rttr;

		registration::class_<EditorToolsSystem>("EditorToolsSystem")
			.constructor<World*>()(policy::ctor::as_raw_ptr)(policy::ctor::as_raw_ptr, metadata("Tag", std::string("editor")))
			.method("update", &EditorToolsSystem::update)(metadata("update_group", update_types::GAMEPLAY_NORMAL));
	}

	EditorToolsSystem::EditorToolsSystem(World* _world)
		: System(_world)
	{

	}

	EditorToolsSystem::~EditorToolsSystem()
	{
	}

	void EditorToolsSystem::init()
	{
		renderer_holder_rc = world->add_root_component<RendererHolderRootComponent>();
		ASSERT(renderer_holder_rc != nullptr);
	}

	void EditorToolsSystem::update(float dt)
	{
		draw_world();
		draw_creation_popup();

		if (ImGui::Begin("Settings"))
		{
			ImGui::Text("%.3f ms, %u FPS", dt * 1000.f, (uint32)(1.f / dt));

			if (ImGui::BeginProperties())
			{
				ImGui::PropertySlider("Time scale", renderer_holder_rc->timestep_scale);

				dx_memory_usage memoryUsage = get_dx_mem_usage();

				ImGui::PropertyValue("Video memory usage", "%u / %uMB", memoryUsage.currentlyUsed, memoryUsage.available);

				ImGui::EndProperties();
			}
		}
		ImGui::End();
	}

	void EditorToolsSystem::draw_creation_popup()
	{
		bool clicked = false;

		render_camera& camera = renderer_holder_rc->camera;

		if (ImGui::BeginPopup("CreateEntityPopup"))
		{
			if (ImGui::MenuItem("Point light", "P") || ImGui::IsKeyPressed('P'))
			{
				Entity pl = world->create_entity("Point light");
				pl.add_component<PointLightComponent>(
						vec3(1.f, 1.f, 1.f),
						1.f,
						10.f,
						true,
						2048u
					);

				TransformComponent* transform = pl.get_component<TransformComponent>();
				transform->set_world_position(camera.position + camera.rotation * vec3(0.f, 0.f, -3.f));

				set_selected_entity(pl);
				clicked = true;
			}

			if (ImGui::MenuItem("Spot light", "S") || ImGui::IsKeyPressed('S'))
			{
				Entity sl = world->create_entity("Spot light");
				sl.add_component<SpotLightComponent>(
						vec3(1.f, 1.f, 1.f),
						1.f,
						25.f,
						deg2rad(20.f),
						deg2rad(30.f),
						true,
						2048u
					);

				TransformComponent* transform = sl.get_component<TransformComponent>();
				transform->set_world_position(camera.position + camera.rotation * vec3(0.f, 0.f, -3.f));

				set_selected_entity(sl);
				clicked = true;
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Empty", "E") || ImGui::IsKeyPressed('E'))
			{
				auto empty = world->create_entity("Empty");
				TransformComponent* transform = empty.get_component<TransformComponent>();
				transform->set_world_position(camera.position + camera.rotation * vec3(0.f, 0.f, -3.f));

				set_selected_entity(empty);
				clicked = true;
			}

			if (clicked)
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void EditorToolsSystem::draw_world()
	{
		bool objectMovedByWidget = false;

		if (ImGui::Begin("Scene Hierarchy"))
		{
			if (ImGui::BeginChild("Outliner"))
			{
				world->view<NameComponent>()
					.each([this](Entity::Handle entityHandle, NameComponent& tag)
						{
							ImGui::PushID((uint32)entityHandle);
							const char* name = tag.name;
							Entity entity = world->get_entity(entityHandle);

							Entity parent = world->get_entity(entity.get_parent_handle());

							if (parent.is_valid())
							{
								ImGuiTreeNodeFlags flags = ((selected_entity == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
								if (ImGui::TreeNodeEx((void*)(uint32)entityHandle, flags, name))
								{
									if (ImGui::IsItemClicked())
									{
										set_selected_entity(entity);
									}

									draw_childs(entity);

									ImGui::TreePop();
								}
							}

							bool entityDeleted = false;
							if (ImGui::BeginPopupContextItem(name))
							{
								if (ImGui::MenuItem("Delete"))
								{
									entityDeleted = true;
								}

								ImGui::EndPopup();
							}

							if (entityDeleted)
							{
								if (entity == selected_entity)
								{
									set_selected_entity({});
								}
								world->destroy_entity(entity);
							}
							ImGui::PopID();
						});
			}
			ImGui::EndChild();

			if (selected_entity.is_valid())
			{
				if (ImGui::Begin("Components"))
				{
					ImGui::AlignTextToFramePadding();

					ImGui::PushID((uint32)selected_entity.get_handle());

					{
						NameComponent* name_component = selected_entity.get_component_if_exists<NameComponent>();

						NameComponent tagBefore = *name_component;
						ImGui::InputText("Name", name_component->name, sizeof(NameComponent::name));
					}

					ImGui::PopID();

					ImGui::SameLine();
					if (ImGui::Button(ICON_FA_TRASH_ALT))
					{
						world->destroy_entity(selected_entity);
						set_selected_entity({});
						objectMovedByWidget = true;
					}

					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("Delete entity");
					}

					if (selected_entity.is_valid())
					{
						EntityEditorUtils::edit_entity(world, selected_entity.get_handle());
					}

					if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !ImGui::IsAnyItemHovered())
					{
						ImGui::OpenPopup("CreateComponentPopup");
					}

					if (ImGui::BeginPopup("CreateComponentPopup"))
					{
						ImGui::Text("Create component");
						ImGui::Separator();

						ImGui::EndPopup();
					}
				}
				ImGui::End();
			}
		}
		ImGui::End();
	}

	void EditorToolsSystem::draw_childs(const Entity& entity)
	{
		static int i = 0;

		auto childs = EntityContainer::get_childs(entity.get_handle());

		if (!childs.empty())
		{
			auto iter = childs.begin();
			const auto& end = childs.end();
			for (; iter != end; ++iter)
			{
				Entity child = world->get_entity(*iter);
				NameComponent* name_component = child.get_component_if_exists<NameComponent>();
				ImGuiTreeNodeFlags flags = ((selected_entity == *iter) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
				if (name_component)
				{
					if (ImGui::TreeNodeEx((void*)(uint32)(*iter), flags, name_component->name))
					{
						if (ImGui::IsItemClicked())
						{
							set_selected_entity(child);
						}

						draw_childs(child);

						ImGui::TreePop();
					}
				}
				else
				{
					if (ImGui::TreeNodeEx((void*)(uint32)(*iter), flags, std::to_string(i++).c_str()))
					{
						if (ImGui::IsItemClicked())
						{
							set_selected_entity(child);
						}

						draw_childs(child);

						ImGui::TreePop();
					}
				}
			}
		}
	}

	void EditorToolsSystem::update_selected_entity_ui_rotation()
	{
		if (selected_entity.is_valid())
		{
			quat rotation = quat::identity;

			if (const TransformComponent* transform = selected_entity.get_component_if_exists<TransformComponent>())
			{
				rotation = transform->transform.rotation;
			}

			selected_entity_euler_rotation = quat_to_euler(rotation);
		}
	}

	void EditorToolsSystem::set_selected_entity(const Entity& entity)
	{
		selected_entity = entity;
		update_selected_entity_ui_rotation();
	}

}