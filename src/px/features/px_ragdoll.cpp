#include "pch.h"
#include "px_ragdoll.h"
#include <px/physics/px_collider_component.h>

//px_ragdoll::px_ragdoll(escene* sc) : scene(sc)
//{
//	root = scene->createEntity("Ragdoll")
//		.addComponent<transform_component>(vec3(0.0), quat::identity, vec3(1.0));
//	init();
//}
//
//void px_ragdoll::init() noexcept
//{
//	group = new px_aggregate(RG_NB_ACTORS);
//
//	createBaseMesh();
//
//	createActors();
//}
//
//void px_ragdoll::release() noexcept
//{
//	rigidbodies.clear();
//	RELEASE_ARRAY_PTR(joints)
//
//	RELEASE_PTR(group)
//}
//
//void px_ragdoll::createBaseMesh()
//{
//	mesh_builder builder;
//	meshBase = make_ref<multi_mesh>();
//	builder.pushCapsule(capsule_mesh_desc(vec3(0.f, -1.f, 0.f), vec3(0.f, 1.f, 0.f), 0.4f));
//
//	auto defaultmat = createPBRMaterialAsync({ "", "" });
//
//	meshBase->submeshes.push_back({ builder.endSubmesh(), {}, trs::identity, defaultmat });
//
//	meshBase->mesh = builder.createDXMesh();
//}
//
//void px_ragdoll::createActors()
//{
//	// Torso
//	{
//		torso = scene->createEntity("Torso")
//			.addComponent<transform_component>(vec3(), quat::identity, vec3(1.0))
//			.addComponent<mesh_component>(meshBase)
//			.addComponent<px_capsule_collider_component>(1.7f, 5.0f)
//			.addComponent<px_rigidbody_component>(px_rigidbody_type::Dynamic, false);
//
//		root.addChild(torso);
//		rigidbodies.emplace("Torso", &torso.getComponent<px_rigidbody_component>());
//	
//		group->addActor(rigidbodies["Torso"]->getRigidActor());
//	}
//
//	// Head
//	{
//		head = scene->createEntity("Head")
//			.addComponent<transform_component>(vec3(0.0f, 3.0f, 0.0f), quat::identity, vec3(1.0))
//			.addComponent<mesh_component>(meshBase)
//			.addComponent<px_capsule_collider_component>(1.0f, 1.2f)
//			.addComponent<px_rigidbody_component>(px_rigidbody_type::Dynamic, false);
//
//		torso.addChild(head);
//		rigidbodies.emplace("Head", &head.getComponent<px_rigidbody_component>());
//
//		px_distance_joint_desc jd;
//		jd.maxDistance = 1.0f;
//		jd.minDistance = 0.0f;
//
//		torsoHead = new px_distance_joint(jd,
//			rigidbodies["Torso"]->getRigidActor(),
//			rigidbodies["Head"]->getRigidActor());
//
//		group->addActor(rigidbodies["Head"]->getRigidActor());
//	}
//
//	// Left arm
//	{
//		// Left upper arm
//		{
//			leftUpperArm = scene->createEntity("LeftUpperArm")
//				.addComponent<transform_component>(vec3(3.0f, 1.0f, 0.0f), quat::identity, vec3(1.0))
//				.addComponent<mesh_component>(meshBase)
//				.addComponent<px_capsule_collider_component>(0.8f, 2.5f)
//				.addComponent<px_rigidbody_component>(px_rigidbody_type::Dynamic, false);
//
//			torso.addChild(leftUpperArm);
//			rigidbodies.emplace("LeftUpperArm", &leftUpperArm.getComponent<px_rigidbody_component>());
//
//			px_spherical_joint_desc jd;
//			jd.limitCone.contactDistance = 1.0f;
//			jd.limitCone.yAngle = M_PI / 4.0f;
//			jd.limitCone.zAngle = M_PI / 4.0f;
//
//			leftArmTorso = new px_spherical_joint(jd,
//				rigidbodies["Torso"]->getRigidActor(),
//				rigidbodies["LeftUpperArm"]->getRigidActor());
//
//			group->addActor(rigidbodies["LeftUpperArm"]->getRigidActor());
//		}
//
//		// Left lower arm
//		{
//			leftLowerArm = scene->createEntity("LeftLowerArm")
//				.addComponent<transform_component>(vec3(3.0f, 0.0f, 0.0f), quat::identity, vec3(1.0))
//				.addComponent<mesh_component>(meshBase)
//				.addComponent<px_capsule_collider_component>(0.8f, 1.6f)
//				.addComponent<px_rigidbody_component>(px_rigidbody_type::Dynamic, false);
//
//			leftUpperArm.addChild(leftLowerArm);
//			rigidbodies.emplace("LeftLowerArm", &leftLowerArm.getComponent<px_rigidbody_component>());
//
//			px_revolute_joint_desc jd;
//			jd.angularPair.contactDistance = 0.0f;
//			jd.angularPair.lower = 0.0f;
//			jd.angularPair.upper = M_PI / 4.0f;
//
//			leftArm = new px_revolute_joint(jd,
//				rigidbodies["LeftUpperArm"]->getRigidActor(),
//				rigidbodies["LeftLowerArm"]->getRigidActor());
//
//			group->addActor(rigidbodies["LeftLowerArm"]->getRigidActor());
//		}
//
//		// Left toes
//		{
//			leftToes = scene->createEntity("LeftToes")
//				.addComponent<transform_component>(vec3(3.0f, -1.0f, 0.0f), quat::identity, vec3(1.0))
//				.addComponent<mesh_component>(meshBase)
//				.addComponent<px_capsule_collider_component>(0.8f, 1.6f)
//				.addComponent<px_rigidbody_component>(px_rigidbody_type::Dynamic, false);
//
//			leftLowerArm.addChild(leftToes);
//			rigidbodies.emplace("LeftToes", &leftToes.getComponent<px_rigidbody_component>());
//
//			px_distance_joint_desc jd;
//			jd.maxDistance = 1.0f;
//			jd.minDistance = 0.0f;
//
//			leftToesArm = new px_distance_joint(jd,
//				rigidbodies["LeftToes"]->getRigidActor(),
//				rigidbodies["LeftLowerArm"]->getRigidActor());
//
//			group->addActor(rigidbodies["LeftToes"]->getRigidActor());
//		}
//	}
//
//	// Right arm
//	{
//		// Right upper arm
//		{
//			rightUpperArm = scene->createEntity("RightUpperArm")
//				.addComponent<transform_component>(vec3(0.0f, 1.0f, 3.0f), quat::identity, vec3(1.0))
//				.addComponent<mesh_component>(meshBase)
//				.addComponent<px_capsule_collider_component>(0.8f, 2.5f)
//				.addComponent<px_rigidbody_component>(px_rigidbody_type::Dynamic, false);
//
//			torso.addChild(rightUpperArm);
//			rigidbodies.emplace("RightUpperArm", &rightUpperArm.getComponent<px_rigidbody_component>());
//
//			px_spherical_joint_desc jd;
//			jd.limitCone.contactDistance = 1.0f;
//			jd.limitCone.yAngle = M_PI / 4.0f;
//			jd.limitCone.zAngle = M_PI / 4.0f;
//
//			rightArmTorso = new px_spherical_joint(jd,
//				rigidbodies["Torso"]->getRigidActor(),
//				rigidbodies["RightUpperArm"]->getRigidActor());
//
//			group->addActor(rigidbodies["RightUpperArm"]->getRigidActor());
//		}
//
//		// Right lower arm
//		{
//			rightLowerArm = scene->createEntity("RightLowerArm")
//				.addComponent<transform_component>(vec3(0.0f, 0.0f, 3.0f), quat::identity, vec3(1.0))
//				.addComponent<mesh_component>(meshBase)
//				.addComponent<px_capsule_collider_component>(0.8f, 1.6f)
//				.addComponent<px_rigidbody_component>(px_rigidbody_type::Dynamic, false);
//
//			rightUpperArm.addChild(rightLowerArm);
//			rigidbodies.emplace("RightLowerArm", &rightLowerArm.getComponent<px_rigidbody_component>());
//
//			px_revolute_joint_desc jd;
//			jd.angularPair.contactDistance = 0.0f;
//			jd.angularPair.lower = 0.0f;
//			jd.angularPair.upper = M_PI / 4.0f;
//
//			rightArm = new px_revolute_joint(jd,
//				rigidbodies["RightUpperArm"]->getRigidActor(),
//				rigidbodies["RightLowerArm"]->getRigidActor());
//
//			group->addActor(rigidbodies["RightLowerArm"]->getRigidActor());
//		}
//
//		// Right toes
//		{
//			rightToes = scene->createEntity("RightToes")
//				.addComponent<transform_component>(vec3(0.0f, -1.0f, 3.0f), quat::identity, vec3(1.0))
//				.addComponent<mesh_component>(meshBase)
//				.addComponent<px_capsule_collider_component>(0.8f, 1.6f)
//				.addComponent<px_rigidbody_component>(px_rigidbody_type::Dynamic, false);
//
//			rightLowerArm.addChild(rightToes);
//			rigidbodies.emplace("RightToes", &rightToes.getComponent<px_rigidbody_component>());
//
//			px_distance_joint_desc jd;
//			jd.maxDistance = 1.0f;
//			jd.minDistance = 0.0f;
//
//			rightToesArm = new px_distance_joint(jd,
//				rigidbodies["RightToes"]->getRigidActor(),
//				rigidbodies["RightLowerArm"]->getRigidActor());
//
//			group->addActor(rigidbodies["RightToes"]->getRigidActor());
//		}
//	}
//}