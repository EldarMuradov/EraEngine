#include <pch.h>

#include "NvBlastExtPxActorImpl.h"
#include "NvBlastExtPxAsset.h"
#include "NvBlastExtPxManagerImpl.h"
#include "NvBlastExtPxFamilyImpl.h"

#include "PxRigidDynamic.h"
#include "PxPhysics.h"

#include "NvBlastAssert.h"

#include "NvBlastTkActor.h"
#include "NvBlastTkAsset.h"

#include "extensions/PxRigidBodyExt.h"
#include <rendering/pbr_material.h>
#include <asset/model_asset.h>
#include <px/core/px_extensions.h>


namespace Nv
{
namespace Blast
{

static int id = 0;


ExtPxActorImpl::ExtPxActorImpl(ExtPxFamilyImpl* family, TkActor* tkActor, const PxActorCreateInfo& pxActorInfo)
	: m_family(family), m_tkActor(tkActor)
{
	const ExtPxChunk* pxChunks = m_family->m_pxAsset.getChunks();
	const ExtPxSubchunk* pxSubchunks = m_family->m_pxAsset.getSubchunks();
	const NvBlastChunk* chunks = m_tkActor->getAsset()->getChunks();
	uint32_t nodeCount = m_tkActor->getGraphNodeCount();

	PxFilterData simulationFilterData;	// Default constructor = {0,0,0,0}

	// get visible chunk indices list
	{
		auto& chunkIndices = m_family->m_indicesScratch;
		chunkIndices.resize(m_tkActor->getVisibleChunkCount());
		m_tkActor->getVisibleChunkIndices(chunkIndices.begin(), static_cast<uint32_t>(chunkIndices.size()));

		// fill visible chunk indices list with mapped to our asset indices
		m_chunkIndices.reserve(chunkIndices.size());
		for (const uint32_t chunkIndex : chunkIndices)
		{
			const ExtPxChunk& chunk = pxChunks[chunkIndex];
			if (chunk.subchunkCount == 0)
				continue;
			m_chunkIndices.pushBack(chunkIndex);
		}

		// Single lower-support chunk actors might be leaf actors, check for this and disable contact callbacks if so
		if (nodeCount <= 1)
		{
			NVBLAST_ASSERT(chunkIndices.size() == 1);
			if (chunkIndices.size() > 0)
			{
				const NvBlastChunk& chunk = chunks[chunkIndices[0]];
				if (chunk.firstChildIndex == chunk.childIndexStop)
				{
					simulationFilterData.word3 = ExtPxManager::LEAF_CHUNK;	// mark as leaf chunk if chunk has no children
				}
			}
		}
	}

	// create rigidDynamic and setup
	PxPhysics& physics = m_family->m_manager.m_physics;
	m_rigidDynamic = physics.createRigidDynamic(pxActorInfo.m_transform);
	if (m_family->m_pxActorDescTemplate != nullptr)
	{
		m_rigidDynamic->setActorFlags(static_cast<physx::PxActorFlags>(m_family->m_pxActorDescTemplate->flags));
	}

	// fill rigidDynamic with shapes
	PxMaterial* material = m_family->m_spawnSettings.material;
	bool created = false;
	for (uint32_t i = 0; i < m_chunkIndices.size(); ++i)
	{
		uint32_t chunkID = m_chunkIndices[i];
		const ExtPxChunk& chunk = pxChunks[chunkID];
		for (uint32_t c = 0; c < chunk.subchunkCount; c++)
		{
			const uint32_t subchunkIndex = chunk.firstSubchunkIndex + c;
			auto& subchunk = pxSubchunks[subchunkIndex];
			PxShape* shape = physics.createShape(subchunk.geometry, *material);

			{
				auto defaultmat = createPBRMaterialAsync({ "", "" });

				mesh_builder builder;

				//submesh_asset asset{};

				const PxConvexMeshGeometry& convexGeom = subchunk.geometry;

				//Compute triangles for each polygon.
				const PxVec3& scale = convexGeom.scale.scale;
				PxConvexMesh* mesh = convexGeom.convexMesh;
				const PxU32 nbPolys = mesh->getNbPolygons();
				const PxU8* polygons = mesh->getIndexBuffer();
				const PxVec3* verts = mesh->getVertices();
				PxU32 nbVerts = mesh->getNbVertices();

				PxU32 numTotalTriangles = 0;
				for (PxU32 i = 0; i < nbPolys; i++)
				{
					PxHullPolygon data;
					mesh->getPolygonData(i, data);

					const PxU32 nbTris = PxU32(data.mNbVerts - 2);
					const PxU8 vref0 = polygons[data.mIndexBase + 0];
					PX_ASSERT(vref0 < nbVerts);
					for (PxU32 j = 0; j < nbTris; j++)
					{
						const PxU32 vref1 = polygons[data.mIndexBase + 0 + j + 1];
						const PxU32 vref2 = polygons[data.mIndexBase + 0 + j + 2];

						//generate face normal:
						PxVec3 e0 = verts[vref1] - verts[vref0];
						PxVec3 e1 = verts[vref2] - verts[vref0];

						PX_ASSERT(vref1 < nbVerts);
						PX_ASSERT(vref2 < nbVerts);

						PxVec3 fnormal = e0.cross(e1);
						fnormal.normalize();

						/*asset.normals.push_back(createVec3(fnormal));	asset.positions.push_back(createVec3(verts[vref0]));
						asset.normals.push_back(createVec3(fnormal));	asset.positions.push_back(createVec3(verts[vref1]));
						asset.normals.push_back(createVec3(fnormal));	asset.positions.push_back(createVec3(verts[vref2]));

						asset.triangles.push_back(indexed_triangle16{ vref0 , (PxU8)vref1, (PxU8)vref2 });*/

						numTotalTriangles++;
					}
				}

				auto enttScene = physics::physics_holder::physicsRef->app.getCurrentScene();

				if (enttScene->registry.size() > 0)
				{
					builder.pushBox({ vec3(0.f), vec3(scale.x, scale.y, scale.z) });

					auto mm = make_ref<multi_mesh>();

					//builder.pushMesh(asset, 1.0f);

					mm->submeshes.push_back({ builder.endSubmesh(), {}, trs::identity, defaultmat });

					eentity entt = enttScene->
						createEntity(("BlastEntity_" + std::to_string(id++)).c_str())
						.addComponent<transform_component>(vec3(physx::createVec3(pxActorInfo.m_transform.p + shape->getLocalPose().p)), quat::identity, vec3(1.f))
						.addComponent<physics::px_shape_holder_component>()
						.addComponent<mesh_component>(mm);

					uint32_t* h = new uint32_t[1];
					h[0] = (uint32_t)entt.handle;
					shape->userData = h;

					mm->mesh = builder.createDXMesh();
				}
			}

			shape->setLocalPose(subchunk.transform);

			const ExtPxShapeDescTemplate* pxShapeDesc = m_family->m_pxShapeDescTemplate;
			if (pxShapeDesc != nullptr)
			{
				shape->setFlags(static_cast<PxShapeFlags>(pxShapeDesc->flags));
				shape->setSimulationFilterData(pxShapeDesc->simulationFilterData);
				shape->setQueryFilterData(pxShapeDesc->queryFilterData);
				shape->setContactOffset(pxShapeDesc->contactOffset);
				shape->setRestOffset(pxShapeDesc->restOffset);
			}
			else
			{
				shape->setSimulationFilterData(simulationFilterData);
			}

			m_rigidDynamic->attachShape(*shape);

			NVBLAST_ASSERT_WITH_MESSAGE(m_family->m_subchunkShapes[subchunkIndex] == nullptr, "Chunk has some shapes(live).");
			m_family->m_subchunkShapes[subchunkIndex] = shape;
		}
	}

	auto enttScene = physics::physics_holder::physicsRef->app.getCurrentScene();
	if (enttScene->registry.size() > 0)
	{
		eentity rbe = enttScene->
			createEntity(("BlastEntity_Core_" + std::to_string(id++)).c_str())
			.addComponent<transform_component>(vec3(0.0f), quat::identity, vec3(1.f))
			.addComponent<physics::px_rigid_shape_holder_component>();
		uint32_t* h = new uint32_t[1];
		h[0] = (uint32_t)rbe.handle;
		m_rigidDynamic->userData = h;
	}

	// search for static chunk in actor's graph (make actor static if it contains static chunk)
	bool staticFound = m_tkActor->isBoundToWorld();
	if (nodeCount > 0)
	{
		auto& graphNodeIndices = m_family->m_indicesScratch;
		graphNodeIndices.resize(nodeCount);
		m_tkActor->getGraphNodeIndices(graphNodeIndices.begin(), static_cast<uint32_t>(graphNodeIndices.size()));
		const NvBlastSupportGraph graph = m_tkActor->getAsset()->getGraph();

		for (uint32_t i = 0; !staticFound && i < graphNodeIndices.size(); ++i)
		{
			const uint32_t chunkIndex = graph.chunkIndices[graphNodeIndices[i]];
			const ExtPxChunk& chunk = pxChunks[chunkIndex];
			staticFound = chunk.isStatic;
		}
	}
	m_rigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, staticFound);

	// store pointer to actor in px userData
	m_family->m_manager.registerActor(m_rigidDynamic, this);

	// store pointer to actor in blast userData
	m_tkActor->userData = this;

	// update mass properties
	PxRigidBodyExt::updateMassAndInertia(*m_rigidDynamic, m_family->m_spawnSettings.density);

	// set initial velocities
	if (!(m_rigidDynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC))
	{
		const PxVec3 COM = m_rigidDynamic->getGlobalPose().transform(m_rigidDynamic->getCMassLocalPose().p);
		const PxVec3 linearVelocity = pxActorInfo.m_parentLinearVelocity + pxActorInfo.m_parentAngularVelocity.cross(COM - pxActorInfo.m_parentCOM);
		const PxVec3 angularVelocity = pxActorInfo.m_parentAngularVelocity;
		m_rigidDynamic->setLinearVelocity(linearVelocity);
		m_rigidDynamic->setAngularVelocity(angularVelocity);
	}
}

void ExtPxActorImpl::release()
{
	if (m_rigidDynamic != nullptr)
	{
		PxShape* shapes[1024];
		PxU32 nbShapes = m_rigidDynamic->getShapes(shapes, 1024);

		for (int j = 0; j < nbShapes; j++)
		{
			if(shapes[j]->userData)
				physics::physics_holder::physicsRef->app.getCurrentScene()->deleteEntity((entity_handle)((uint32*)shapes[j]->userData)[0]);
		}

		if (m_rigidDynamic->userData)
			physics::physics_holder::physicsRef->app.getCurrentScene()->deleteEntity((entity_handle)((uint32*)m_rigidDynamic->userData)[0]);
		m_family->m_manager.unregisterActor(m_rigidDynamic);
		m_rigidDynamic->release();
		m_rigidDynamic = nullptr;
	}

	const ExtPxChunk* pxChunks = m_family->m_pxAsset.getChunks();
	for (uint32_t chunkID : m_chunkIndices)
	{
		const ExtPxChunk& chunk = pxChunks[chunkID];
		for (uint32_t c = 0; c < chunk.subchunkCount; c++)
		{
			const uint32_t subchunkIndex = chunk.firstSubchunkIndex + c;
			m_family->m_subchunkShapes[subchunkIndex] = nullptr;
		}
	}
	m_chunkIndices.clear();

	m_tkActor->userData = nullptr;
}

ExtPxFamily& ExtPxActorImpl::getFamily() const
{
	return *m_family;
}


} // namespace Blast
} // namespace Nv
