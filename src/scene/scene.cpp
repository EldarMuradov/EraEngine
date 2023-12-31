#pragma once
#include "pch.h"

#include "scene.h"
#include "physics/physics.h"
#include "physics/collision_broad.h"
#include "terrain/heightmap_collider.h"
#include "rendering/raytracing.h"

#ifndef PHYSICS_ONLY
#include <px/core/px_physics_engine.h>
#include <px/physics/px_rigidbody_component.h>
#include <px/physics/px_collider_component.h>
#include <px/physics/px_character_controller_component.h>
#include <EraScriptingLauncher-Lib/src/script.h>
#include <ai/navigation_component.h>
#endif

std::mutex eentity_container::sync;
std::unordered_map<entity_handle, eentity_node> eentity_container::container;

escene::escene()
{
	// Construct groups early. Ignore the return types.
	(void)registry.group<collider_component, sap_endpoint_indirection_component>(); // Colliders and SAP endpoints are always sorted in the same order.
	(void)registry.group<transform_component, dynamic_transform_component, rigid_body_component, physics_transform0_component, physics_transform1_component>();
	(void)registry.group<transform_component, rigid_body_component, physics_transform0_component, physics_transform1_component>();

#ifndef PHYSICS_ONLY
	(void)registry.group<position_component, point_light_component>();
	(void)registry.group<position_rotation_component, spot_light_component>();
#endif
}

void escene::clearAll()
{
	void clearBroadphase(escene & scene);
	clearBroadphase(*this);

	deleteAllConstraints(*this);
	registry.clear();
}

void escene::cloneTo(escene& target)
{
	target.registry.assign(registry.data(), registry.data() + registry.size(), registry.released());

	using copy_components = component_group_t<
		tag_component,
		transform_component,
		position_component,
		position_rotation_component,
		position_scale_component,
		dynamic_transform_component,

#ifndef PHYSICS_ONLY
		px_rigidbody_component,
		px_box_collider_component,
		px_collider_component_base,
		px_sphere_collider_component,
		px_capsule_collider_component,
		px_triangle_mesh_collider_component,
		px_bounding_box_collider_component,
		px_cct_component_base,
		px_capsule_cct_component,
		px_box_cct_component,

		navigation_component,
		script_component,
#endif

		child_component,

#ifndef PHYSICS_ONLY
		point_light_component,
		spot_light_component,
		cloth_render_component,

		terrain_component,
		grass_component,
		proc_placement_component,
		water_component,
		tree_component,
#endif
		heightmap_collider_component,

		mesh_component,

		raytrace_component,

		animation_component,

		collider_component,
		rigid_body_component,
		force_field_component,
		trigger_component,
		cloth_component,
		physics_reference_component,
		sap_endpoint_indirection_component,
		constraint_entity_reference_component,

		physics_transform0_component,
		physics_transform1_component,

		distance_constraint,
		ball_constraint,
		fixed_constraint,
		hinge_constraint,
		cone_twist_constraint,
		slider_constraint
	>;

	copyComponentPoolsTo(copy_components{}, target);

	target.registry.ctx() = registry.ctx();
}

eentity escene::copyEntity(eentity src)
{
	ASSERT(src.hasComponent<tag_component>());

	tag_component& tag = src.getComponent<tag_component>();
	eentity dest = createEntity(tag.name);
	dest.setParent(src.getParentHandle());

	if (auto* c = src.getComponentIfExists<transform_component>()) { dest.addComponent<transform_component>(*c); }
	if (auto* c = src.getComponentIfExists<position_component>()) { dest.addComponent<position_component>(*c); }
	if (auto* c = src.getComponentIfExists<position_rotation_component>()) { dest.addComponent<position_rotation_component>(*c); }
	if (auto* c = src.getComponentIfExists<dynamic_transform_component>()) { dest.addComponent<dynamic_transform_component>(*c); }

#ifndef PHYSICS_ONLY
	if (auto* c = src.getComponentIfExists<point_light_component>()) { dest.addComponent<point_light_component>(*c); }
	if (auto* c = src.getComponentIfExists<spot_light_component>()) { dest.addComponent<spot_light_component>(*c); }
	if (auto* c = src.getComponentIfExists<cloth_render_component>()) { dest.addComponent<cloth_render_component>(*c); }

	if (auto* c = src.getComponentIfExists<terrain_component>()) { dest.addComponent<terrain_component>(*c); }
#endif

	if (auto* c = src.getComponentIfExists<animation_component>()) { dest.addComponent<animation_component>(*c); }
	if (auto* c = src.getComponentIfExists<mesh_component>()) { dest.addComponent<mesh_component>(*c); }
	if (auto* c = src.getComponentIfExists<raytrace_component>()) { dest.addComponent<raytrace_component>(*c); }

	for (collider_component& collider : collider_component_iterator(src))
		dest.addComponent<collider_component>(collider);

	if (auto* c = src.getComponentIfExists<rigid_body_component>()) { dest.addComponent<rigid_body_component>(*c); }
	if (auto* c = src.getComponentIfExists<force_field_component>()) { dest.addComponent<force_field_component>(*c); }
	if (auto* c = src.getComponentIfExists<trigger_component>()) { dest.addComponent<trigger_component>(*c); }
	if (auto* c = src.getComponentIfExists<cloth_component>()) { dest.addComponent<cloth_component>(*c); }

	return dest;
}

void escene::deleteEntity(eentity e)
{
	void removeColliderFromBroadphase(eentity entity);

	if (physics_reference_component* reference = e.getComponentIfExists<physics_reference_component>())
	{
		eentity colliderEntity = { reference->firstColliderEntity, &registry };
		while (colliderEntity)
		{
			collider_component& collider = colliderEntity.getComponent<collider_component>();
			entity_handle next = collider.nextEntity;

			removeColliderFromBroadphase(colliderEntity);
		
			registry.destroy(colliderEntity.handle);
		
			colliderEntity = { next, &registry };
		}

		deleteAllConstraintsFromEntity(e);
	}

	// TODO: make it thread-safe
	auto childs = e.getChilds();
	auto iter = childs.begin();
	const auto& end = childs.end();
	for (; iter != end; ++iter)
		deleteEntity(*iter);

	registry.destroy(e.handle);
}
