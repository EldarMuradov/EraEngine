// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include <pch.h>

#define TINYOBJLOADER_IMPLEMENTATION

#include <px/blast/px_blast_core.h>

#if !_DEBUG

#include <NvBlastExtPxSerialization.h>
#include <numeric>
#include <cooking/PxCooking.h>
#include <core/imgui.h>
#include <scene/components.h>
#include <NvBlastExtAssetUtils.h>
#include <sstream>
#include <fstream>

#include <tiny_obj_loader.h>
#include <NvBlastExtPxCollisionBuilder.h>
#include <asset/mesh_postprocessing.h>
#include <NvBlastExtExporter.h>

using namespace std::chrono;

static int id = 0;

physics::px_blast::px_blast()
	: debugRenderMode(px_blast_family::DEBUG_RENDER_DISABLED)
	, impactDamageEnabled(true)
	, impactDamageToStressEnabled(false)
	, rigidBodyLimitEnabled(true)
	, rigidBodyLimit(40000)
	, blastAssetsSize(0)
	, debugRenderScale(0.01f)
	, taskManager(nullptr)
	, extGroupTaskManager(nullptr)
	, damageDescBuffer(64 * 1024)
	, damageParamsBuffer(1024)
{
	impactDamageToStressFactor = 0.001f;
	draggingToStressFactor = 100.0f;
}

void physics::px_blast::reinitialize()
{
	onSampleStop();
}

void physics::px_blast::onSampleStart(PxPhysics* physics, PxScene* scene)
{
	framework = NvBlastTkFrameworkCreate();

	replay = new px_blast_replay();

	taskManager = PxTaskManager::createTaskManager(errorReporter, scene->getCpuDispatcher());

	TkGroupDesc gdesc{};
	gdesc.workerCount = taskManager->getCpuDispatcher()->getWorkerCount();
	tkGroup = framework->createGroup(gdesc);

	extPxManager = ExtPxManager::create(*physics, *framework, createPxJointCallback, false);
	extPxManager->setActorCountLimit(rigidBodyLimitEnabled ? rigidBodyLimit : 0);
	extImpactDamageManagerSettings.isSelfCollissionEnabled = false;
	extImpactDamageManagerSettings.shearDamage = true;
	extImpactDamageManagerSettings.hardness = 2.f;
	extImpactDamageManagerSettings.damageRadiusMax = 3.f;
	extImpactDamageManagerSettings.damageThresholdMin = 0.001f;

	extImpactDamageManager = ExtImpactDamageManager::create(extPxManager, extImpactDamageManagerSettings);

	extGroupTaskManager = ExtGroupTaskManager::create(*taskManager);
	extGroupTaskManager->setGroup(tkGroup);

	setImpactDamageEnabled(impactDamageEnabled, true);

	extSerialization = NvBlastExtSerializationCreate();
	if (extSerialization != nullptr)
	{
		size_t nb = NvBlastExtTkSerializerLoadSet(*framework, *extSerialization);

		// TODO: Switch serialization to PhysX 5.3.1
		//NvBlastExtLlSerializerLoadSet(*extSerialization);
		
		NvBlastExtPxSerializerLoadSet(*framework, *physics,
			*extSerialization);
	}
}

void physics::px_blast::onSampleStop()
{
	removeAllFamilies();

	PX_RELEASE(extImpactDamageManager);
	RELEASE_PTR(extPxManager);
	PX_RELEASE(tkGroup);
	RELEASE_PTR(replay);
	PX_RELEASE(framework);
	PX_RELEASE(extGroupTaskManager);
	PX_RELEASE(taskManager);
	PX_RELEASE(extSerialization);
}

void physics::px_blast::animate(double dt)
{
	extImpactDamageManager->applyDamage();

	updateDraggingStress();

	fillDebugRender();

	updateImpactDamage();

	for (uint32_t i = 0; i < families.size(); ++i)
	{
		if (families[i])
		{
			families[i]->updatePreSplit(dt);
		}
	}

	replay->update();

#if 0

	extGroupTaskManager->process();
	extGroupTaskManager->wait();

#else  // process group on main thread

	tkGroup->process();

#endif

	damageParamsBuffer.clear();
	damageDescBuffer.clear();

	TkGroupStats gstats;
	tkGroup->getStats(gstats);

	this->lastBlastTimers.damageMaterial = NvBlastTicksToSeconds(gstats.timers.material);
	this->lastBlastTimers.damageFracture = NvBlastTicksToSeconds(gstats.timers.fracture);
	this->lastBlastTimers.splitIsland = NvBlastTicksToSeconds(gstats.timers.island);
	this->lastBlastTimers.splitPartition = NvBlastTicksToSeconds(gstats.timers.partition);
	this->lastBlastTimers.splitVisibility = NvBlastTicksToSeconds(gstats.timers.visibility);

	for (uint32_t i = 0; i < families.size(); ++i)
	{
		if (families[i])
		{
			families[i]->updateAfterSplit(dt);
		}
	}

	tkGroup->endProcess();
}

void physics::px_blast::drawUI()
{
	if(ImGui::BeginTree("Blast"))
	{
		// impact damage
		bool impactEnabled = getImpactDamageEnabled();
		if (ImGui::Checkbox("Impact Damage", &impactEnabled))
		{
			setImpactDamageEnabled(impactEnabled);
		}
		{
			bool refresh = false;
			refresh |= ImGui::Checkbox("Use Shear Damage", &extImpactDamageManagerSettings.shearDamage);
			refresh |= ImGui::DragFloat("Material Hardness", &extImpactDamageManagerSettings.hardness);
			refresh |= ImGui::DragFloat("Damage Radius (Max)", &extImpactDamageManagerSettings.damageRadiusMax);
			refresh |= ImGui::DragFloat("Damage Threshold (Min)", &extImpactDamageManagerSettings.damageThresholdMin,
				1.0f, 0.0f, 1.0f);
			refresh |= ImGui::DragFloat("Damage Threshold (Max)", &extImpactDamageManagerSettings.damageThresholdMax,
				1.0f, 0.0f, 1.0f);
			refresh |= ImGui::DragFloat("Damage Falloff Radius Factor",
				&extImpactDamageManagerSettings.damageFalloffRadiusFactor, 1.0f, 0.0f, 32.0f);
			refresh |= ImGui::Checkbox("Impact Damage To Stress Solver", &impactDamageToStressEnabled);

			if (refresh)
			{
				refreshImpactDamageSettings();
			}
		}

		ImGui::DragFloat("Impact Damage To Stress Factor", &impactDamageToStressFactor, 0.001f, 0.0f, 1000.0f, "%.4f");
		ImGui::DragFloat("Dragging To Stress Factor", &draggingToStressFactor, 0.1f, 0.0f, 1000.0f, "%.3f");

		ImGui::Checkbox("Limit Rigid Body Count", &rigidBodyLimitEnabled);
		if (rigidBodyLimitEnabled)
		{
			ImGui::DragInt("Rigid Body Limit", (int*)&rigidBodyLimit, 100, 1000, 100000);
		}
		extPxManager->setActorCountLimit(rigidBodyLimitEnabled ? rigidBodyLimit : 0);
		ImGui::EndTree();
	}
}

bool physics::px_blast::overlap(const PxGeometry& geometry, const PxTransform& pose, std::function<void(ExtPxActor*, px_blast_family&)> hitCall)
{
	bool anyHit = false;
	for (uint32_t i = 0; i < families.size(); ++i)
	{
		if (families[i])
		{
			anyHit |= families[i]->overlap(geometry, pose, hitCall);
		}
	}
	return anyHit;
}

bool physics::px_blast::stressDamage(ExtPxActor* actor, PxVec3 position, PxVec3 force) const
{
	if (actor->getTkActor().getGraphNodeCount() > 1)
	{
		void* userData = actor->getFamily().userData;
		if (userData)
		{
			auto localForce = force * impactDamageToStressFactor;
			ExtPxStressSolver* solver = reinterpret_cast<ExtPxStressSolver*>(userData);
			solver->getSolver().addForce(*actor->getTkActor().getActorLL(), reinterpret_cast<const NvcVec3&>(position),
				reinterpret_cast<const NvcVec3&>(localForce));
			return true;
		}
	}

	return false;
}

void physics::px_blast::deferDamage(ExtPxActor* actor, px_blast_family& family, const NvBlastDamageProgram& program, const void* damageDesc, uint32_t damageDescSize)
{
	const void* bufferedDamageDesc = damageDescBuffer.push(damageDesc, damageDescSize);
	PX_ASSERT_WITH_MESSAGE(bufferedDamageDesc, "Damage desc buffer exhausted.");

	NvBlastExtProgramParams programParams = { bufferedDamageDesc, &family.getMaterial(),
											  actor->getFamily().getPxAsset().getAccelerator() };

	const void* bufferedProgramParams = damageParamsBuffer.push(&programParams, sizeof(NvBlastExtProgramParams));
	PX_ASSERT_WITH_MESSAGE(bufferedProgramParams, "Damage params buffer exhausted.");

	if (bufferedDamageDesc && bufferedProgramParams)
	{
		actor->getTkActor().damage(program, bufferedProgramParams);
	}
}

void physics::px_blast::immediateDamage(ExtPxActor* actor, px_blast_family& family, const NvBlastDamageProgram& program, const void* damageDesc)
{
	NvBlastExtProgramParams programParams = { damageDesc, &family.getMaterial(),
										  actor->getFamily().getPxAsset().getAccelerator() };

	NvBlastFractureBuffers& fractureEvents = getFractureBuffers(actor);
	actor->getTkActor().generateFracture(&fractureEvents, program, &programParams);
	actor->getTkActor().applyFracture(nullptr, &fractureEvents);
}

NvBlastFractureBuffers& physics::px_blast::getFractureBuffers(ExtPxActor* actor)
{
	const TkAsset* tkAsset = actor->getTkActor().getAsset();
	const uint32_t chunkCount = tkAsset->getChunkCount();
	const uint32_t bondCount = tkAsset->getBondCount();

	fractureBuffers.bondFractureCount = bondCount;
	fractureBuffers.chunkFractureCount = chunkCount;
	fractureData.resize((uint32_t)(fractureBuffers.bondFractureCount * sizeof(NvBlastBondFractureData) +
		fractureBuffers.chunkFractureCount * sizeof(NvBlastChunkFractureData)));  // chunk

	fractureBuffers.chunkFractures = reinterpret_cast<NvBlastChunkFractureData*>(fractureData.data());
	fractureBuffers.bondFractures = reinterpret_cast<NvBlastBondFractureData*>(
		&fractureData.data()[fractureBuffers.chunkFractureCount * sizeof(NvBlastChunkFractureData)]);
	return fractureBuffers;
}

ref<physics::px_blast_family> physics::px_blast::spawnFamily(px_blast_asset* blastAsset, const px_blast_asset::actor_desc& desc)
{
	ref<px_blast_family> actor = blastAsset->createFamily(*extPxManager, desc);
	families.push_back(actor);
	recalculateAssetsSize();
	replay->addFamily(&actor->getFamily()->getTkFamily());
	return actor;
}

void physics::px_blast::removeFamily(ref<px_blast_family> actor)
{
	replay->removeFamily(&actor->getFamily()->getTkFamily());
	families.erase(std::remove(families.begin(), families.end(), actor), families.end());
	recalculateAssetsSize();
}

void physics::px_blast::removeAllFamilies()
{
	while (!families.empty())
	{
		removeFamily(families.back());
	}
	replay->reset();
}

uint32_t physics::px_blast::getActorCount() const
{
	return std::accumulate(families.begin(), families.end(), (uint32_t)0,
		[](uint32_t sum, const ref<px_blast_family>& a) { return sum += a->getActorCount(); });
}

uint32_t physics::px_blast::getTotalVisibleChunkCount() const
{
	return std::accumulate(families.begin(), families.end(), (uint32_t)0,
		[](uint32_t sum, const ref<px_blast_family>& a) { return sum += a->getTotalVisibleChunkCount(); });
}

size_t physics::px_blast::getFamilySize() const
{
	return std::accumulate(families.begin(), families.end(), (size_t)0,
		[](size_t sum, const ref<px_blast_family>& a) { return sum += a->getFamilySize(); });
}

void physics::px_blast::setImpactDamageEnabled(bool enabled, bool forceUpdate)
{
	if (impactDamageEnabled != enabled || forceUpdate)
	{
		impactDamageEnabled = enabled;
		impactDamageUpdatePending = true;
	}
}

float physics::px_blast::getLastStressDelta() const
{
	return 0.0f;
}

void physics::px_blast::notifyPhysXControllerRelease()
{
	PX_RELEASE(extGroupTaskManager);
	PX_RELEASE(taskManager);
}

void physics::px_blast::updateDraggingStress()
{
}

void physics::px_blast::updateImpactDamage()
{
	if (impactDamageUpdatePending)
	{
		refreshImpactDamageSettings();
		impactDamageUpdatePending = false;
	}
}

void physics::px_blast::refreshImpactDamageSettings()
{
	extImpactDamageManagerSettings.damageFunction =
		impactDamageToStressEnabled ? customImpactDamageFunction : nullptr;
	extImpactDamageManagerSettings.damageFunctionData = this;
	extImpactDamageManager->setSettings(extImpactDamageManagerSettings);
}

void physics::px_blast::fillDebugRender()
{
}

void physics::px_blast::recalculateAssetsSize()
{
	std::set<const px_blast_asset*> uniquedAssets;
	blastAssetsSize = 0;
	for (uint32_t i = 0; i < families.size(); ++i)
	{
		if (uniquedAssets.find(&families[i]->getBlastAsset()) == uniquedAssets.end())
		{
			blastAssetsSize += families[i]->getBlastAsset().getBlastAssetSize();
			uniquedAssets.insert(&families[i]->getBlastAsset());
		}
	}
}

bool physics::px_blast::customImpactDamageFunction(void* data, ExtPxActor* actor, PxShape* shape, PxVec3 position, PxVec3 force)
{
	return reinterpret_cast<px_blast*>(data)->stressDamage(actor, position, force);
}

void physics::px_cube_asset_generator::generate(px_asset_generator& asset, const asset_settings& settings)
{
	// cleanup
	asset.solverChunks.clear();
	asset.solverBonds.clear();
	asset.chunks.clear();

	// initial params
	std::vector<uint32_t> depthStartIDs;
	std::vector<PxVec3> depthSlicesPerAxisTotal;
	uint32_t currentID = 0;
	PxVec3 extents = settings.extents;
	asset.extents = extents;

	// Iterate over depths and create children
	for (uint32_t depth = 0; depth < settings.depths.size(); depth++)
	{
		PxVec3 slicesPerAxis = settings.depths[depth].slicesPerAxis;
		PxVec3 slicesPerAxisTotal = (depth == 0) ? slicesPerAxis : slicesPerAxis * (depthSlicesPerAxisTotal[depth - 1]);
		depthSlicesPerAxisTotal.push_back(slicesPerAxisTotal);

		depthStartIDs.push_back(currentID);
		extents.x /= slicesPerAxis.x;
		extents.y /= slicesPerAxis.y;
		extents.z /= slicesPerAxis.z;

		for (uint32_t z = 0; z < (uint32_t)slicesPerAxisTotal.z; ++z)
		{
			uint32_t parent_z = z / (uint32_t)slicesPerAxis.z;
			for (uint32_t y = 0; y < (uint32_t)slicesPerAxisTotal.y; ++y)
			{
				uint32_t parent_y = y / (uint32_t)slicesPerAxis.y;
				for (uint32_t x = 0; x < (uint32_t)slicesPerAxisTotal.x; ++x)
				{
					uint32_t parent_x = x / (uint32_t)slicesPerAxis.x;
					uint32_t parentID = depth == 0 ? UINT32_MAX :
						depthStartIDs[depth - 1] + parent_x + (uint32_t)depthSlicesPerAxisTotal[depth - 1].x * (parent_y + (uint32_t)depthSlicesPerAxisTotal[depth - 1].y * parent_z);

					PxVec3 position;
					position.x = ((float)x - (slicesPerAxisTotal.x / 2) + 0.5f) * extents.x;
					position.y = ((float)y - (slicesPerAxisTotal.y / 2) + 0.5f) * extents.y;
					position.z = ((float)z - (slicesPerAxisTotal.z / 2) + 0.5f) * extents.z;

					NvBlastChunkDesc chunkDesc;
					memcpy(chunkDesc.centroid, &position.x, 3 * sizeof(float));
					chunkDesc.volume = extents.x * extents.y * extents.z;
					chunkDesc.flags = settings.depths[depth].flag;
					chunkDesc.userData = currentID++;
					chunkDesc.parentChunkDescIndex = parentID;
					asset.solverChunks.push_back(chunkDesc);

					if (settings.depths[depth].flag & NvBlastChunkDesc::Flags::SupportFlag)
					{
						// Internal bonds

						// x-neighbor
						if (x > 0 && (settings.bondFlags & bound_flags::X_BONDS))
						{
							PxVec3 xNeighborPosition = position - PxVec3(extents.x, 0, 0);
							uint32_t neighborID = chunkDesc.userData - 1;
							fillBondDesc(asset.solverBonds, chunkDesc.userData, neighborID, position, xNeighborPosition, extents, extents.y * extents.z);
						}

						// y-neighbor
						if (y > 0 && (settings.bondFlags & bound_flags::Y_BONDS))
						{
							PxVec3 yNeighborPosition = position - PxVec3(0, extents.y, 0);
							uint32_t neighborID = chunkDesc.userData - (uint32_t)slicesPerAxisTotal.x;
							fillBondDesc(asset.solverBonds, chunkDesc.userData, neighborID, position, yNeighborPosition, extents, extents.z * extents.x);
						}

						// z-neighbor
						if (z > 0 && (settings.bondFlags & bound_flags::Z_BONDS))
						{
							PxVec3 zNeighborPosition = position - PxVec3(0, 0, extents.z);
							uint32_t neighborID = chunkDesc.userData - (uint32_t)slicesPerAxisTotal.x * (uint32_t)slicesPerAxisTotal.y;
							fillBondDesc(asset.solverBonds, chunkDesc.userData, neighborID, position, zNeighborPosition, extents, extents.x * extents.y);
						}

						// World bonds (only one per chunk is enough, otherwise they will be removed as duplicated, thus 'else if')

						// -x world bond
						if (x == 0 && (settings.bondFlags & bound_flags::X_MINUS_WORLD_BONDS))
						{
							PxVec3 xNeighborPosition = position - PxVec3(extents.x, 0, 0);
							fillBondDesc(asset.solverBonds, chunkDesc.userData, UINT32_MAX, position, xNeighborPosition, extents, extents.y * extents.z);
						}
						// +x world bond
						else if (x == slicesPerAxisTotal.x - 1 && (settings.bondFlags & bound_flags::X_PLUS_WORLD_BONDS))
						{
							PxVec3 xNeighborPosition = position + PxVec3(extents.x, 0, 0);
							fillBondDesc(asset.solverBonds, chunkDesc.userData, UINT32_MAX, position, xNeighborPosition, extents, extents.y * extents.z);
						}
						// -y world bond
						else if (y == 0 && (settings.bondFlags & bound_flags::Y_MINUS_WORLD_BONDS))
						{
							PxVec3 yNeighborPosition = position - PxVec3(0, extents.y, 0);
							fillBondDesc(asset.solverBonds, chunkDesc.userData, UINT32_MAX, position, yNeighborPosition, extents, extents.z * extents.x);
						}
						// +y world bond
						else if (y == slicesPerAxisTotal.y - 1 && (settings.bondFlags & bound_flags::Y_PLUS_WORLD_BONDS))
						{
							PxVec3 yNeighborPosition = position + PxVec3(0, extents.y, 0);
							fillBondDesc(asset.solverBonds, chunkDesc.userData, UINT32_MAX, position, yNeighborPosition, extents, extents.z * extents.x);
						}
						// -z world bond
						else if (z == 0 && (settings.bondFlags & bound_flags::Z_MINUS_WORLD_BONDS))
						{
							PxVec3 zNeighborPosition = position - PxVec3(0, 0, extents.z);
							fillBondDesc(asset.solverBonds, chunkDesc.userData, UINT32_MAX, position, zNeighborPosition, extents, extents.x * extents.y);
						}
						// +z world bond
						else if (z == slicesPerAxisTotal.z - 1 && (settings.bondFlags & bound_flags::Z_PLUS_WORLD_BONDS))
						{
							PxVec3 zNeighborPosition = position + PxVec3(0, 0, extents.z);
							fillBondDesc(asset.solverBonds, chunkDesc.userData, UINT32_MAX, position, zNeighborPosition, extents, extents.x * extents.y);
						}
					}

					asset.chunks.push_back(px_asset_generator::blast_chunk_cube(position, extents/*isStatic*/));
				}
			}
		}
	}

	// Reorder chunks
	std::vector<uint32_t> chunkReorderMap(asset.solverChunks.size());
	std::vector<char> scratch(asset.solverChunks.size() * sizeof(NvBlastChunkDesc));
	NvBlastBuildAssetDescChunkReorderMap(chunkReorderMap.data(), asset.solverChunks.data(), (uint32_t)asset.solverChunks.size(), scratch.data(), nullptr);

	std::vector<px_asset_generator::blast_chunk_cube> chunksTemp = asset.chunks;

	for (uint32_t i = 0; i < chunkReorderMap.size(); ++i)
	{
		asset.chunks[chunkReorderMap[i]] = chunksTemp[i];
	}

	NvBlastApplyAssetDescChunkReorderMapInPlace(asset.solverChunks.data(), (uint32_t)asset.solverChunks.size(), asset.solverBonds.data(), (uint32_t)asset.solverBonds.size(), chunkReorderMap.data(), true, scratch.data(), nullptr);
}

void physics::px_cube_asset_generator::fillBondDesc(std::vector<NvBlastBondDesc>& bondDescs, uint32_t id0, uint32_t id1, PxVec3 pos0, PxVec3 pos1, PxVec3 size, float area)
{
	NV_UNUSED(size);

	NvBlastBondDesc bondDesc{};
	bondDesc.chunkIndices[0] = id0;
	bondDesc.chunkIndices[1] = id1;
	bondDesc.bond.area = area;

	PxVec3 centroid = (pos0 + pos1) * 0.5f;
	bondDesc.bond.centroid[0] = centroid.x;
	bondDesc.bond.centroid[1] = centroid.y;
	bondDesc.bond.centroid[2] = centroid.z;

	PxVec3 normal = (pos0 - pos1).getNormalized();
	bondDesc.bond.normal[0] = normal.x;
	bondDesc.bond.normal[1] = normal.y;
	bondDesc.bond.normal[2] = normal.z;

	bondDescs.push_back(bondDesc);
}

physics::px_blast_asset_boxes::px_blast_asset_boxes(TkFramework& framework, PxPhysics& physics, main_renderer& renderer, const desc& desc)
	: px_blast_asset(renderer)
{
	// generate boxes slices procedurally
	px_cube_asset_generator::generate(generatorAsset, desc.generatorSettings);

	// asset desc / tk asset
	ExtPxAssetDesc assetDesc;
	assetDesc.chunkDescs = generatorAsset.solverChunks.data();
	assetDesc.chunkCount = (uint32_t)generatorAsset.solverChunks.size();
	assetDesc.bondDescs = generatorAsset.solverBonds.data();
	assetDesc.bondCount = (uint32_t)generatorAsset.solverBonds.size();

	std::vector<uint8_t> bondFlags(assetDesc.bondCount);
	std::fill(bondFlags.begin(), bondFlags.end(), desc.jointAllBonds ? 1 : 0);
	assetDesc.bondFlags = bondFlags.data();

	// box convex
	PxVec3 vertices[8] = { { -1, -1, -1 }, { -1, -1, 1 }, { -1, 1, -1 }, { -1, 1, 1 }, { 1, -1, -1 }, { 1, -1, 1 }, { 1, 1, -1 }, { 1, 1, 1 } };
	PxConvexMeshDesc convexMeshDesc;
	convexMeshDesc.points.count = 8;
	convexMeshDesc.points.data = vertices;
	convexMeshDesc.points.stride = sizeof(PxVec3);
	convexMeshDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

	px_convex_mesh_builder builder;

	boxMesh = builder.createConvexMesh(convexMeshDesc);

	// prepare chunks
	const uint32_t chunkCount = (uint32_t)generatorAsset.solverChunks.size();
	std::vector<ExtPxAssetDesc::ChunkDesc> pxChunks(chunkCount);
	std::vector<ExtPxAssetDesc::SubchunkDesc> pxSubchunks;
	pxSubchunks.reserve(chunkCount);
	for (uint32_t i = 0; i < generatorAsset.solverChunks.size(); i++)
	{
		uint32_t chunkID = generatorAsset.solverChunks[i].userData;
		px_asset_generator::blast_chunk_cube& cube = generatorAsset.chunks[chunkID];
		PxVec3 position = *reinterpret_cast<PxVec3*>(&cube.position);
		PxVec3 extents = *reinterpret_cast<PxVec3*>(&cube.extents);
		ExtPxAssetDesc::ChunkDesc& chunk = pxChunks[chunkID];

		ExtPxAssetDesc::SubchunkDesc subchunk =
		{
			PxTransform(position),
			PxConvexMeshGeometry(boxMesh, PxMeshScale(extents / 2))
		};

		pxSubchunks.push_back(subchunk);
		chunk.subchunks = &pxSubchunks.back();
		chunk.subchunkCount = 1;
		chunk.isStatic = (position.y - (extents.y - desc.generatorSettings.extents.y) / 2) <= desc.staticHeight;
	}

	// create asset
	assetDesc.pxChunks = pxChunks.data();
	pxAsset = ExtPxAsset::create(assetDesc, framework);

	initialize();
}

physics::px_blast_asset_boxes::~px_blast_asset_boxes()
{
	boxMesh->release();
	pxAsset->release();
}

ref<physics::px_blast_family> physics::px_blast_asset_boxes::createFamily(ExtPxManager& pxManager, const actor_desc& desc)
{
	return ref<px_blast_family>(new px_blast_family_boxes(pxManager, renderer, *this, desc));
}

physics::px_blast_asset::px_blast_asset(main_renderer& rndr)
	: renderer(rndr), bondHealthMax(1.0f), supportChunkHealthMax(1.0f), damageAccelerator(nullptr)
{
}

physics::px_blast_asset::~px_blast_asset()
{
	if (damageAccelerator)
	{
		damageAccelerator->release();
	}
}

size_t physics::px_blast_asset::getBlastAssetSize() const
{
	return pxAsset->getTkAsset().getDataSize();
}

void physics::px_blast_asset::initialize()
{
	// calc max healths
	auto& actorDesc = pxAsset->getDefaultActorDesc();

	if (actorDesc.initialBondHealths)
	{
		bondHealthMax = FLT_MIN;
		const uint32_t bondCount = pxAsset->getTkAsset().getBondCount();
		for (uint32_t i = 0; i < bondCount; ++i)
		{
			bondHealthMax = std::max<float>(bondHealthMax, actorDesc.initialBondHealths[i]);
		}
	}
	else
	{
		bondHealthMax = actorDesc.uniformInitialBondHealth;
	}

	if (actorDesc.initialSupportChunkHealths)
	{
		supportChunkHealthMax = FLT_MIN;
		const uint32_t nodeCount = pxAsset->getTkAsset().getGraph().nodeCount;
		for (uint32_t i = 0; i < nodeCount; ++i)
		{
			supportChunkHealthMax = std::max<float>(supportChunkHealthMax, actorDesc.initialSupportChunkHealths[i]);
		}
	}
	else
	{
		supportChunkHealthMax = actorDesc.uniformInitialLowerSupportChunkHealth;
	}

	damageAccelerator = NvBlastExtDamageAcceleratorCreate(pxAsset->getTkAsset().getAssetLL(), 3);
}

bool physics::px_blast_family::overlap(const PxGeometry& geometry, const PxTransform& pose, std::function<void(ExtPxActor*, px_blast_family&)> hitCall)
{
	std::set<ExtPxActor*> actorsToDamage;
#if 1
	px_blast_overlap_callback overlapCallback(pxManager, actorsToDamage);
	physics_holder::physicsRef->getScene()->overlap(geometry, pose, overlapCallback);
#else
	for (std::map<NvBlastActor*, PhysXController::Actor*>::iterator it = m_actorsMap.begin(); it != m_actorsMap.end(); it++)
	{
		actorsToDamage.insert(it->first);
	}
#endif

	for (auto actor : actorsToDamage)
	{
		hitCall(actor, *this);
	}

	return !actorsToDamage.empty();
}

void physics::px_blast_family::updatePreSplit(float dt)
{
	if (!spawned)
	{
		ExtPxSpawnSettings spawnSettings = {
			physics_holder::physicsRef->getScene(),
			physics_holder::physicsRef->getPhysics()->createMaterial(0.8f, 0.8f, 0.6f),
			RIGIDBODY_DENSITY
		};

		pxFamily->spawn(initialTransform, PxVec3(1.0f), spawnSettings);
		reloadStressSolver();

		spawned = true;
	}

	// collect potential actors to health update
	actorsToUpdateHealth.clear();

	for (ExtPxActor* actor : actors)
	{
		auto pxactor = &actor->getPhysXActor();

		if (actor->getTkActor().isPending())
		{
			actorsToUpdateHealth.emplace(actor);
		}
	}
}

void physics::px_blast_family::updateAfterSplit(float dt)
{
	for (ExtPxActor* actor : actors)
	{
		onActorUpdate(*actor);

		// update health if neccessary
		if (actorsToUpdateHealth.find(actor) != actorsToUpdateHealth.end())
		{
			onActorHealthUpdate(*actor);
		}
	}

	// update stress
	stressSolveTime = 0;
	if (stressSolver)
	{
		stressSolver->update(settings.stressDamageEnabled);
	}

	onUpdate();

	pxFamily->postSplitUpdate();
}

void physics::px_blast_family::drawUI()
{
	// Blast Material
	ImGui::Spacing();
	ImGui::Text("Blast Material:");
	ImGui::DragFloat("Health", &settings.material.health);
	ImGui::DragFloat("Min Damage Threshold", &settings.material.minDamageThreshold, 0.01f, 0.f, settings.material.maxDamageThreshold);
	ImGui::DragFloat("Max Damage Threshold", &settings.material.maxDamageThreshold, 0.01f, settings.material.minDamageThreshold, 1.f);

	if (ImGui::Checkbox("AABB Tree (Damage Accelerator)", &settings.damageAcceleratorEnabled))
	{
		refreshDamageAcceleratorSettings();
	}
	if (settings.damageAcceleratorEnabled)
	{
		ImGui::DragInt("AABB Tree debug depth", &debugRenderDepth);
	}

	ImGui::Spacing();

	// Stress Solver Settings
	if (ImGui::Checkbox("Stress Solver Enabled", &settings.stressSolverEnabled))
	{
		reloadStressSolver();
	}

	if (settings.stressSolverEnabled)
	{
		// Settings
		bool changed = false;

		//changed |= ImGui::DragInt("Bond Iterations Per Frame", (int*)&settings.stressSolverSettings.bondIterationsPerFrame, 100, 0, 500000);
		//changed |= ImGui::DragFloat("Material Hardness", &settings.stressSolverSettings.hardness, 10.0f, 0.01f, 100000.0f, "%.2f");
		//changed |= ImGui::DragFloat("Stress Linear Factor", &settings.stressSolverSettings.stressLinearFactor, 0.01f, 0.0f, 100.0f, "%.2f");
		//changed |= ImGui::DragFloat("Stress Angular Factor", &settings.stressSolverSettings.stressAngularFactor, 0.01f, 0.0f, 100.0f, "%.2f");
		changed |= ImGui::SliderInt("Graph Reduction Level", (int*)&settings.stressSolverSettings.graphReductionLevel, 0, 32);
		if (changed)
		{
			refreshStressSolverSettings();
		}

		ImGui::Checkbox("Stress Damage Enabled", &settings.stressDamageEnabled);

		if (ImGui::Button("Recalculate Stress"))
		{
			resetStress();
		}
	}
}

void physics::px_blast_family::drawStatsUI()
{
	if (stressSolver)
	{
		const ExtStressSolver& solver = stressSolver->getSolver();
		const float errorLinear = solver.getStressErrorLinear();
		const float errorAngular = solver.getStressErrorAngular();

		ImGui::Text("Stress Bond Count:               %d", solver.getBondCount());
		ImGui::Text("Stress Frames:                   %d", solver.getFrameCount());
		ImGui::Text("Stress Error Lin / Ang:          %.4f / %.4f", errorLinear, errorAngular);
		ImGui::Text("Stress Solve Time:               %.3f ms", stressSolveTime * 1000);

		// plot errors
		{
			static float scale = 1.0f;
			scale = solver.getFrameCount() <= 1 ? 1.0f : scale;
			scale = std::max<float>(scale, errorLinear);
			scale = std::max<float>(scale, errorAngular);

			static ImGui::PlotLinesInstance<> linearErrorPlot;
			linearErrorPlot.plot("Stress Linear Error", errorLinear, "error/frame", 0.0f, 1.0f * scale);
			static ImGui::PlotLinesInstance<> angularErrorPlot;
			angularErrorPlot.plot("Stress Angular Error", errorAngular, "error/frame", 0.0f, 1.0f * scale);
		}
	}
	else
	{
		ImGui::Text("No Stress Solver");
	}
	ImGui::PopStyleColor();
}

void physics::px_blast_family::fillDebugRender(px_debug_render_buffer& debugRenderBuffer, debug_render_mode mode, float renderScale)
{
}

uint32_t physics::px_blast_family::getActorCount() const
{
	return (uint32_t)tkFamily->getActorCount();
}

void physics::px_blast_family::resetStress()
{
	if (stressSolver)
	{
		stressSolver->getSolver().reset();
	}
}

void physics::px_blast_family::refreshStressSolverSettings()
{
	if (stressSolver)
	{
		stressSolver->getSolver().setSettings(settings.stressSolverSettings);
	}
}

void physics::px_blast_family::refreshDamageAcceleratorSettings()
{
	pxFamily->getPxAsset().setAccelerator(settings.damageAcceleratorEnabled ? blastAsset.getAccelerator() : nullptr);
}

void physics::px_blast_family::reloadStressSolver()
{
	if (stressSolver)
	{
		stressSolver->release();
		stressSolver = nullptr;
	}

	if (settings.stressSolverEnabled)
	{
		stressSolver = ExtPxStressSolver::create(*pxFamily, settings.stressSolverSettings);
		pxFamily->userData = stressSolver;
	}
}

void physics::px_blast_family::setSettings(const family_settings& setts)
{
	bool reloadStressSolverNeeded = (settings.stressSolverEnabled != setts.stressSolverEnabled);

	settings = setts;
	refreshStressSolverSettings();
	refreshDamageAcceleratorSettings();

	if (reloadStressSolverNeeded)
	{
		reloadStressSolver();
	}

	pxFamily->setMaterial(&settings.material);
}

physics::px_blast_family::~px_blast_family()
{
	if (stressSolver)
	{
		stressSolver->release();
	}
	if (pxFamily)
	{
		pxFamily->unsubscribe(listener);
		pxFamily->release();
	}
}

physics::px_blast_family::px_blast_family(ExtPxManager& manager, const px_blast_asset& asset)
	: 
	  pxManager(manager)
	, blastAsset(asset)
	, listener(this)
	, totalVisibleChunkCount(0)
	, stressSolver(nullptr)
	, spawned(false)
	, debugRenderDepth(-1)
{
	settings.stressSolverEnabled = false;
	settings.stressDamageEnabled = false;
	settings.damageAcceleratorEnabled = true;
}

void physics::px_blast_family::initialize(const px_blast_asset::actor_desc& desc)
{
	ExtPxFamilyDesc familyDesc{};
	familyDesc.actorDesc = nullptr; // if you use it one day, consider changing code which needs getBondHealthMax() from BlastAsset.
	familyDesc.group = desc.group;
	familyDesc.pxAsset = blastAsset.getPxAsset();
	pxFamily = pxManager.createFamily(familyDesc);
	pxFamily->setMaterial(&settings.material);

	tkFamily = &pxFamily->getTkFamily();
	tkFamily->setID(desc.id);

	refreshDamageAcceleratorSettings();

	familySize = NvBlastFamilyGetSize(tkFamily->getFamilyLL(), nullptr);

	pxFamily->subscribe(listener);

	initialTransform = desc.transform;
}

void physics::px_blast_family::processActorCreated(ExtPxFamily&, ExtPxActor& actor)
{
	totalVisibleChunkCount += actor.getChunkCount();
	actors.emplace(&actor);

	onActorCreated(actor);
	onActorHealthUpdate(actor);
}

void physics::px_blast_family::processActorDestroyed(ExtPxFamily&, ExtPxActor& actor)
{
	totalVisibleChunkCount -= actor.getChunkCount();
	// TODO: notify

	onActorDestroyed(actor);

	actors.erase(actors.find(&actor));
}

physics::px_blast_family_boxes::px_blast_family_boxes(ExtPxManager& pxManager, main_renderer& rend, const px_blast_asset_boxes& blastAsset, const px_blast_asset::actor_desc& desc)
	: px_blast_family(pxManager, blastAsset), renderer(rend)
{
	const ExtPxAsset* pxAsset = blastAsset.getPxAsset();
	const uint32_t chunkCount = pxAsset->getChunkCount();
	const ExtPxChunk* chunks = pxAsset->getChunks();
	const ExtPxSubchunk* subChunks = pxAsset->getSubchunks();
	chunkRenderables.resize(chunkCount);

	auto defaultmat = createPBRMaterialAsync({ "", "" });

	auto enttScene = physics::physics_holder::physicsRef->app.getCurrentScene();

	mesh_builder builder;

	for (uint32_t i = 0; i < chunkCount; i++)
	{
		auto& geom = subChunks[chunks[i].firstSubchunkIndex].geometry;

		builder.pushBox({ vec3(0.f), vec3(geom.scale.scale.x, geom.scale.scale.y, geom.scale.scale.z) });

		auto mm = make_ref<multi_mesh>();

		mm->submeshes.push_back({ builder.endSubmesh(), {}, trs::identity, defaultmat });

		eentity entt = enttScene->
			createEntity(("BlastEntity_" + std::to_string(::id++)).c_str())
			.addComponent<transform_component>(vec3(0.0f), quat::identity, vec3(1.f))
			.addComponent<physics::px_shape_holder_component>()
			.addComponent<mesh_component>(mm, true);

		chunkRenderables[i] = entt.handle;

		mm->mesh = builder.createDXMesh();
	}

	// initialize in position
	initialize(desc);
}

physics::px_blast_family_boxes::~px_blast_family_boxes()
{
}

void physics::px_blast_family_boxes::onActorCreated(const ExtPxActor& actor)
{
	const uint32_t* chunkIndices = actor.getChunkIndices();
	uint32_t chunkCount = actor.getChunkCount();

	auto enttScene = physics::physics_holder::physicsRef->app.getCurrentScene();

	for (uint32_t i = 0; i < chunkCount; i++)
	{
		const uint32_t chunkIndex = chunkIndices[i];
		enttScene->registry.get<mesh_component>(chunkRenderables[chunkIndex]).isHidden = false;
	}
}

void physics::px_blast_family_boxes::onActorUpdate(const ExtPxActor& actor)
{
	if (!&actor.getPhysXActor())
		return;

	physics::physics_holder::physicsRef->lockRead();

	const ExtPxChunk* chunks = blastAsset.getPxAsset()->getChunks();
	const ExtPxSubchunk* subChunks = blastAsset.getPxAsset()->getSubchunks();
	const uint32_t* chunkIndices = actor.getChunkIndices();
	uint32_t chunkCount = actor.getChunkCount();

	auto enttScene = physics::physics_holder::physicsRef->app.getCurrentScene();

	for (uint32_t i = 0; i < chunkCount; i++)
	{
		const uint32_t chunkIndex = chunkIndices[i];

		eentity renderEntity{ chunkRenderables[chunkIndex], &enttScene->registry };

		auto pose = actor.getPhysXActor().getGlobalPose() * subChunks[chunks[chunkIndex].firstSubchunkIndex].transform;

		renderEntity.getComponent<transform_component>().position = createVec3(pose.p);
		renderEntity.getComponent<transform_component>().rotation = createQuat(pose.q);
	}
	physics::physics_holder::physicsRef->unlockRead();
}

void physics::px_blast_family_boxes::onActorDestroyed(const ExtPxActor& actor)
{
	const uint32_t* chunkIndices = actor.getChunkIndices();
	uint32_t chunkCount = actor.getChunkCount();

	auto enttScene = physics::physics_holder::physicsRef->app.getCurrentScene();

	for (uint32_t i = 0; i < chunkCount; i++)
	{
		const uint32_t chunkIndex = chunkIndices[i];
		enttScene->registry.get<mesh_component>(chunkRenderables[chunkIndex]).isHidden = true;
	}
}

physics::px_blast_replay::px_blast_replay()
{
	sync = ExtSync::create();
	reset();
}

physics::px_blast_replay::~px_blast_replay()
{
	sync->release();
	clearBuffer();
}

void physics::px_blast_replay::addFamily(TkFamily* family) const
{
	family->addListener(*sync);
}

void physics::px_blast_replay::removeFamily(TkFamily* family) const
{
	family->removeListener(*sync);
}

void physics::px_blast_replay::startRecording(ExtPxManager& manager, bool syncFamily, bool syncPhysics)
{
	if (isRecording)
		return;

	sync->releaseSyncBuffer();

	if (syncFamily || syncPhysics)
	{
		std::vector<ExtPxFamily*> families(manager.getFamilyCount());
		manager.getFamilies(families.data(), (uint32_t)families.size());
		for (ExtPxFamily* family : families)
		{
			if (syncPhysics)
			{
				sync->syncFamily(*family);
			}
			else if (syncFamily)
			{
				sync->syncFamily(family->getTkFamily());
			}
		}
	}

	isRecording = true;
}

void physics::px_blast_replay::stopRecording()
{
	if (!isRecording)
		return;

	const ExtSyncEvent* const* buff;
	uint32_t size;
	sync->acquireSyncBuffer(buff, size);

	clearBuffer();
	buffer.resize(size);
	for (uint32_t i = 0; i < size; ++i)
	{
		buffer[i] = buff[i]->clone();
	}

	// TODO: sort by ts ? make sure?
	//buffer.sort

	sync->releaseSyncBuffer();

	isRecording = false;
}

void physics::px_blast_replay::startPlayback(ExtPxManager& manager, TkGroup* group)
{
	if (isPlaying || !hasRecord())
		return;

	isPlaying = true;
	startTime = steady_clock::now();
	nextEventIndex = 0;
	firstEventTs = buffer[0]->timestamp;
	pxManager = &manager;
	group = group;
}

void physics::px_blast_replay::stopPlayback()
{
	if (!isPlaying)
		return;

	isPlaying = false;
	pxManager = nullptr;
	group = nullptr;
}

void physics::px_blast_replay::update()
{
	if (isPlaying)
	{
		auto now = steady_clock::now();
		auto mil = duration_cast<milliseconds>((now - startTime));
		bool stop = true;
		while (nextEventIndex < buffer.size())
		{
			const ExtSyncEvent* e = buffer[nextEventIndex];
			auto t = e->timestamp - firstEventTs;
			if (t < (uint64_t)mil.count())
			{
				sync->applySyncBuffer(pxManager->getFramework(), &e, 1, group, pxManager);
				nextEventIndex++;
			}
			else
			{
				stop = false;
				break;
			}
		}

		if (stop)
			stopPlayback();
	}
}

void physics::px_blast_replay::reset()
{
	isPlaying = false;
	isRecording = false;
	sync->releaseSyncBuffer();
}

void physics::px_blast_replay::clearBuffer()
{
	for (auto e : buffer)
	{
		e->release();
	}
	buffer.clear();
}



physics::px_blast_asset_model::px_blast_asset_model(TkFramework& framework, PxPhysics& physics, ExtSerialization& serialization, main_renderer& rndr, const char* modelName)
	: px_blast_asset(rndr)
{
	const float unitConversion = 1.f;

	const NvcVec3 inputScale = { unitConversion, unitConversion, unitConversion };

	std::string path;

	// load obj file
	std::ostringstream objFileName;
	std::ostringstream mtlFileName;
	objFileName << "assets\\obj\\" << modelName << ".obj";
	mtlFileName << "assets\\obj\\" << modelName << ".mtl";

	model = px_blast_model::loadFromTinyLoader(objFileName.str().c_str(), mtlFileName.str().c_str());
	if (!model)
	{
		LOG_ERROR("Blast> Obj load failed.");
	}

	for (auto& chunk : model->chunks)
	{
		for (auto& mesh : chunk.meshes)
		{
			px_simple_mesh& smesh = const_cast<px_simple_mesh&>(mesh.mesh);
			smesh.center *= unitConversion;
			smesh.extents *= unitConversion;
			for (auto& vertex : smesh.vertices)
			{
				vertex.position *= unitConversion;
			}
		}
	}

	std::ostringstream blastFileName;
	blastFileName << "assets/obj/" << modelName << ".blast";
	{
		std::fstream stream(blastFileName.str().c_str(), std::ios::binary | std::ios::in);
		std::streampos size = stream.tellg();
		stream.seekg(0, std::ios::end);
		size = stream.tellg() - size;
		stream.seekg(0, std::ios::beg);
		std::vector<char> buffer(size);
		stream.read(buffer.data(), buffer.size());
		stream.close();
		uint32_t objectTypeID;
		void* asset = serialization.deserializeFromBuffer(buffer.data(), buffer.size(), &objectTypeID);
		if (asset == nullptr)
		{
			LOG_ERROR("Blast> Can't load .blast file.");
		}

		if (objectTypeID == Nv::Blast::ExtPxObjectTypeID::Asset)
		{
			pxAsset = reinterpret_cast<ExtPxAsset*>(asset);
			const TkAsset& tkAsset = pxAsset->getTkAsset();
			NvBlastAsset* llasset = const_cast<NvBlastAsset*>(tkAsset.getAssetLL());
			NvBlastExtAssetTransformInPlace(llasset, &inputScale, nullptr, nullptr);
			ExtPxSubchunk* subchunks = const_cast<ExtPxSubchunk*>(pxAsset->getSubchunks());
			for (uint32_t i = 0; i < pxAsset->getSubchunkCount(); ++i)
			{
				subchunks[i].geometry.scale.scale = PxVec3(unitConversion);
			}
		}
		else
		{
			TkAsset* tkAsset = nullptr;
			if (objectTypeID == Nv::Blast::TkObjectTypeID::Asset)
			{
				tkAsset = reinterpret_cast<TkAsset*>(asset);
				NvBlastAsset* llasset = const_cast<NvBlastAsset*>(tkAsset->getAssetLL());
				NvBlastExtAssetTransformInPlace(llasset, &inputScale, nullptr, nullptr);
			}
			else if (objectTypeID == Nv::Blast::LlObjectTypeID::Asset)
			{
				NvBlastAsset* llasset = reinterpret_cast<NvBlastAsset*>(asset);
				NvBlastExtAssetTransformInPlace(llasset, &inputScale, nullptr, nullptr);
				tkAsset = framework.createAsset(llasset, nullptr, 0, true);
			}
			else
			{
				LOG_ERROR("Blast> .blast file contains unknown object.");
			}

			if (tkAsset != nullptr)
			{
				std::vector<ExtPxAssetDesc::ChunkDesc> physicsChunks;
				std::vector<std::vector<ExtPxAssetDesc::SubchunkDesc> > physicsSubchunks;

				objFileName.str("");
				objFileName << "assets\\obj\\" << modelName << ".fbx";
				{
					std::shared_ptr<IFbxFileReader> rdr(NvBlastExtExporterCreateFbxFileReader(),
						[](IFbxFileReader* p) { p->release(); });
					rdr->loadFromFile(objFileName.str().c_str());
					if (rdr->isCollisionLoaded() == 0)
					{
						LOG_ERROR("Blast> fbx doesn't contain collision geometry");
					}
					uint32_t* hullsOffsets = nullptr;
					CollisionHull** hulls = nullptr;
					uint32_t meshCount = rdr->getCollision(hullsOffsets, hulls);

					physicsChunks.resize(meshCount);
					physicsSubchunks.resize(meshCount);

					std::shared_ptr<ExtPxCollisionBuilder> collisionBuilder(
						ExtPxManager::createCollisionBuilder(physics),
						[](Nv::Blast::ExtPxCollisionBuilder* cmb) { cmb->release(); });

					for (uint32_t i = 0; i < meshCount; ++i)
					{
						for (uint32_t sbHulls = hullsOffsets[i]; sbHulls < hullsOffsets[i + 1]; ++sbHulls)
						{
							PxConvexMeshGeometry temp =
								physx::PxConvexMeshGeometry(collisionBuilder.get()->buildConvexMesh(*hulls[sbHulls]));
							if (temp.isValid())
							{
								physicsSubchunks[i].push_back(ExtPxAssetDesc::SubchunkDesc());
								physicsSubchunks[i].back().geometry = temp;
								physicsSubchunks[i].back().transform = physx::PxTransform(physx::PxIdentity);
							}
						}
					}
					for (uint32_t i = 0; i < meshCount; ++i)
					{
						physicsChunks[i].isStatic = false;
						physicsChunks[i].subchunkCount = (uint32_t)physicsSubchunks[i].size();
						physicsChunks[i].subchunks = physicsSubchunks[i].data();
					}
					if (hulls && hullsOffsets)
					{
						for (uint32_t h = 0; h < hullsOffsets[meshCount]; h++)
						{
							collisionBuilder->releaseCollisionHull(hulls[h]);
						}
						NVBLAST_FREE(hulls);
						NVBLAST_FREE(hullsOffsets);
					}
				}
				pxAsset = ExtPxAsset::create(tkAsset, physicsChunks.data(), (uint32_t)physicsChunks.size());
			}
		}
	}

	initialize();
}

physics::px_blast_asset_model::~px_blast_asset_model()
{
	pxAsset->release();
}

ref<physics::px_blast_family> physics::px_blast_asset_model::createFamily(ExtPxManager& pxManager, const actor_desc& desc)
{
	return ref<px_blast_family>(new px_blast_family_simple_mesh(pxManager, renderer, *this, desc));
}

void physics::px_blast_single_scene_asset::spawn(PxVec3 shift)
{
	load();

	trs t0 = trs(vec3(0.f, 0.0f, 0.0f), quat::identity, vec3(1.f));

	physics_holder::physicsRef->app.getCurrentScene()->createEntity("blast_entity")
		.addComponent<transform_component>(vec3(0.f), quat::identity, vec3(1.f))
		.addComponent<px_blast_rigidbody_component>(std::string("blast_entity"), this);

	trs t1 = trs(vec3(50.f, 0.0f, 0.0f), quat::identity, vec3(1.f));
	physics_holder::physicsRef->app.getCurrentScene()->createEntity("blast_entity1")
		.addComponent<transform_component>(vec3(5.0f, 0.0f, 0.0f), quat::identity, vec3(1.f))
		.addComponent<px_blast_rigidbody_component>(std::string("blast_entity1"), this);
}

void physics::px_blast_single_scene_asset::spawnOnObject(PxVec3 shift, eentity& entity)
{
	load();
	auto name = entity.getComponent<tag_component>().name;
	entity.addComponent<px_blast_rigidbody_component>(std::string(name), this);
}

static void loadMeshes(std::vector<physics::px_blast_model::chunk::mesh>& meshes, tinyobj::mesh_t mesh)
{
	// Check if all faces are triangles
	bool allTriangles = true;
	for (uint32_t i = 0; i < mesh.num_vertices.size(); ++i)
	{
		if (mesh.num_vertices[i] != 3)
		{
			allTriangles = false;
			break;
		}
	}
	if (!allTriangles) return;

	std::map<int32_t, uint32_t> matIdToMesh;
	for (int32_t mt : mesh.material_ids)
	{
		auto it = matIdToMesh.find(mt);
		if (it == matIdToMesh.end())
		{
			meshes.push_back(physics::px_blast_model::chunk::mesh());
			matIdToMesh[mt] = uint32_t(meshes.size()) - 1;
		}
	}

	std::vector<physics::px_simple_mesh::vertex> oldVertices(mesh.positions.size() / 3);
	std::vector<uint32_t> oldIndexToNew;

	for (uint32_t i = 0; i < oldVertices.size(); ++i)
	{
		oldVertices[i].position.x = mesh.positions[i * 3];
		oldVertices[i].position.y = mesh.positions[i * 3 + 1];
		oldVertices[i].position.z = mesh.positions[i * 3 + 2];

		oldVertices[i].normal.x = mesh.normals[i * 3];
		oldVertices[i].normal.y = mesh.normals[i * 3 + 1];
		oldVertices[i].normal.z = mesh.normals[i * 3 + 2];

		oldVertices[i].uv.x = mesh.texcoords[i * 2];
		oldVertices[i].uv.y = mesh.texcoords[i * 2 + 1];
	}

	for (auto& matmapping : matIdToMesh)
	{
		int32_t mid = matmapping.first;
		auto sampleMesh = &meshes[matmapping.second];
		sampleMesh->materialIndex = (mid >= 0) ? mid : 0;
		oldIndexToNew.assign(oldVertices.size(), -1);

		physx::PxVec3 emin(FLT_MAX, FLT_MAX, FLT_MAX);
		physx::PxVec3 emax(FLT_MIN, FLT_MIN, FLT_MIN);

		for (uint32_t i = 0; i < mesh.indices.size() / 3; i++)
		{
			if (mesh.material_ids[i] != mid) continue;
			for (int32_t vi = 2; vi >= 0; --vi)
			{
				int32_t idx = mesh.indices[i * 3 + vi];
				if (oldIndexToNew[idx] == -1)
				{
					oldIndexToNew[idx] = (uint32_t)sampleMesh->mesh.vertices.size();
					sampleMesh->mesh.vertices.push_back(oldVertices[idx]);

					emin = emin.minimum(sampleMesh->mesh.vertices.back().position);
					emax = emax.maximum(sampleMesh->mesh.vertices.back().position);
				}
				sampleMesh->mesh.indices.push_back(oldIndexToNew[idx]);

				// assign extents
				sampleMesh->mesh.extents = (emax - emin) * 0.5f;

				// get the center
				sampleMesh->mesh.center = emin + sampleMesh->mesh.extents;
			}
		}
	}
}

ref<physics::px_blast_model> physics::px_blast_model::loadFromTinyLoader(const char* path, const char* pathMtl)
{
	ref<px_blast_model> model = make_ref<px_blast_model>();

	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> mats;
	std::string err;
	std::string mtlPath = pathMtl;

	/*for(size_t i = strnlen(path, 255) - 1; i >= 0; --i)
	{
		if (path[i] == '\\')
		{
			mtlPath.resize(i + 2, 0);
			strncpy(&mtlPath[0], path, i + 1);
			break;
		}
	}*/

	bool ret = tinyobj::LoadObj(shapes, mats, err, path, mtlPath.data());

	// can't load?
	if (!ret)
		return nullptr;

	// one submodel per material
	uint32_t materialsCount = (uint32_t)mats.size();
	model->materials.resize(materialsCount);

	// fill submodel materials
	for (uint32_t i = 0; i < materialsCount; i++)
	{
		tinyobj::material_t* pMaterial = &mats[i];

		if (!pMaterial->diffuse_texname.empty())
		{
			model->materials[i].diffuseTexture = pMaterial->diffuse_texname;
		}
	}

	// estimate
	model->chunks.reserve(shapes.size() / materialsCount + 1);
	uint32_t chunkIndex = 1;
	if (shapes.size() > 0)
	{
		uint32_t meshIndex = 0;
		for (uint32_t m = 0; m < shapes.size(); m++)
		{
			tinyobj::shape_t& pMesh = shapes[m];
			int32_t materialIndex = 0;	// This is actually not set
			//uint32_t chunkIndex;
			//int32_t sc = sscanf(pMesh.name.data(), "%d_%d", &chunkIndex, &materialIndex);
			//if (sc == 0)
			//{
			//	return nullptr;
			//}
			if (model->chunks.size() <= chunkIndex)
			{
				model->chunks.resize(chunkIndex);
			}
			loadMeshes(model->chunks.back().meshes, pMesh.mesh);

			chunkIndex++;
		}
	}

	return model;
}

static ref<submesh_asset> createGraphicsMesh(const physics::px_simple_mesh& simpleMesh)
{
	ref<submesh_asset> asset = make_ref<submesh_asset>();
	auto defaultmat = createPBRMaterialAsync({ "", "" });

	std::vector<vec3> positions; positions.reserve(1 << 16);
	std::vector<vec2> uvs; uvs.reserve(1 << 16);
	std::vector<vec3> normals; normals.reserve(1 << 16);

	std::vector<vec3> positionCache; positionCache.reserve(16);
	std::vector<vec2> uvCache; uvCache.reserve(16);
	std::vector<vec3> normalCache; normalCache.reserve(16);

	size_t nbTriangles = simpleMesh.indices.size() / 3;

	for (const auto& pos : simpleMesh.vertices)
	{
		asset->positions.push_back(physx::createVec3(pos.position));
		//asset->normals.push_back(physx::createVec3(pos.normal));
		asset->uvs.push_back(physx::createVec2(pos.uv));
	}

	for (uint32_t i = 0; i < simpleMesh.indices.size(); i += 3)
	{
		asset->triangles.push_back(indexed_triangle16{
			(uint16)simpleMesh.indices[i + 0],
			(uint16)simpleMesh.indices[i + 1],
			(uint16)simpleMesh.indices[i + 2] });
	}

	generateNormalsAndTangents(asset, mesh_flag_default);

	return asset;
}

physics::px_blast_family_simple_mesh::px_blast_family_simple_mesh(ExtPxManager& pxManager, main_renderer& rndr, const px_blast_asset_model& blastAsset, const px_blast_asset::actor_desc& desc)
	: px_blast_family(pxManager, blastAsset), renderer(rndr)
{
	// materials
	auto defaultmat = createPBRMaterialAsync({ "", "" });

	auto enttScene = physics::physics_holder::physicsRef->app.getCurrentScene();

	// model
	const px_blast_model& model = *blastAsset.getModel();

	// create render mesh for every BlastModel::Chunk::Mesh and renderable with it
	const std::vector<px_blast_model::chunk>& modelChunks = model.chunks;
	chunks.resize(modelChunks.size());
	for (uint32_t chunkIndex = 0; chunkIndex < modelChunks.size(); chunkIndex++)
	{
		const std::vector<px_blast_model::chunk::mesh>& meshes = modelChunks[chunkIndex].meshes;
		std::vector<ref<multi_mesh>>& renderMeshes = chunks[chunkIndex].renderMeshes;
		std::vector<entity_handle>& renderables = chunks[chunkIndex].renderables;

		renderMeshes.resize(meshes.size());
		renderables.resize(meshes.size());

		for (uint32_t i = 0; i < meshes.size(); i++)
		{
			mesh_builder builder;

			builder.pushMesh(*createGraphicsMesh(meshes[i].mesh), 1.0f);

			renderMeshes[i] = make_ref<multi_mesh>();
			renderMeshes[i]->submeshes.push_back({ builder.endSubmesh(), {}, trs::identity, defaultmat });

			eentity entt = enttScene->
				createEntity(("BlastEntity_" + std::to_string(::id++)).c_str())
				.addComponent<transform_component>(vec3(0.0f), quat::identity, vec3(1.f))
				.addComponent<physics::px_shape_holder_component>()
				.addComponent<mesh_component>(renderMeshes[i], true);

			renderables[i] = entt.handle;

			renderMeshes[i]->mesh = builder.createDXMesh();
		}
	}

	// initialize in position
	initialize(desc);
}

physics::px_blast_family_simple_mesh::~px_blast_family_simple_mesh()
{

}

void physics::px_blast_family_simple_mesh::onActorCreated(const ExtPxActor& actor)
{
	const uint32_t* chunkIndices = actor.getChunkIndices();
	uint32_t chunkCount = actor.getChunkCount();
	auto enttScene = physics::physics_holder::physicsRef->app.getCurrentScene();

	for (uint32_t i = 0; i < chunkCount; i++)
	{
		uint32_t chunkIndex = chunkIndices[i];
		std::vector<entity_handle>& renderables = chunks[chunkIndex].renderables;
		for (uint32_t r = 0; r < renderables.size(); r++)
		{
			enttScene->registry.get<mesh_component>(renderables[r]).isHidden = false;
		}
	}
}

void physics::px_blast_family_simple_mesh::onActorUpdate(const ExtPxActor& actor)
{
	const ExtPxChunk* pxchunks = blastAsset.getPxAsset()->getChunks();
	const ExtPxSubchunk* subChunks = blastAsset.getPxAsset()->getSubchunks();
	const uint32_t* chunkIndices = actor.getChunkIndices();
	uint32_t chunkCount = actor.getChunkCount();
	auto enttScene = physics::physics_holder::physicsRef->app.getCurrentScene();

	for (uint32_t i = 0; i < chunkCount; i++)
	{
		uint32_t chunkIndex = chunkIndices[i];
		std::vector<entity_handle>& renderables = chunks[chunkIndex].renderables;
		for (const entity_handle& r : renderables)
		{
			eentity renderEntity{ r, &enttScene->registry };

			auto pose = actor.getPhysXActor().getGlobalPose() * subChunks[pxchunks[chunkIndex].firstSubchunkIndex].transform;

			renderEntity.getComponent<transform_component>().position = createVec3(pose.p);
			renderEntity.getComponent<transform_component>().rotation = createQuat(pose.q);
		}
	}
}

void physics::px_blast_family_simple_mesh::onActorDestroyed(const ExtPxActor& actor)
{
	const uint32_t* chunkIndices = actor.getChunkIndices();
	uint32_t chunkCount = actor.getChunkCount();
	auto enttScene = physics::physics_holder::physicsRef->app.getCurrentScene();

	for (uint32_t i = 0; i < chunkCount; i++)
	{
		uint32_t chunkIndex = chunkIndices[i];
		std::vector<entity_handle>& renderables = chunks[chunkIndex].renderables;
		for (const entity_handle& r : renderables)
		{
			enttScene->registry.get<mesh_component>(r).isHidden = true;
		}
	}
}

void physics::px_blast_family_simple_mesh::onActorHealthUpdate(const ExtPxActor& pxActor)
{
	TkActor& tkActor = pxActor.getTkActor();
	const TkAsset* tkAsset = tkActor.getAsset();

	const float* bondHealths = tkActor.getBondHealths();
	uint32_t nodeCount = tkActor.getGraphNodeCount();
	if (nodeCount == 0) // subsupport chunks don't have graph nodes
		return;

	auto enttScene = physics::physics_holder::physicsRef->app.getCurrentScene();

	std::vector<uint32_t> nodes(tkActor.getGraphNodeCount());
	tkActor.getGraphNodeIndices(nodes.data(), static_cast<uint32_t>(nodes.size()));

	const NvBlastChunk* pxchunks = tkAsset->getChunks();
	const NvBlastBond* bonds = tkAsset->getBonds();

	const NvBlastSupportGraph graph = tkAsset->getGraph();
	const float bondHealthMax = blastAsset.getBondHealthMax();

	std::vector<float> healthBuffer;

	for (uint32_t node0 : nodes)
	{
		uint32_t chunkIndex = graph.chunkIndices[node0];

		if (chunkIndex >= chunks.size())
			continue;

		std::vector<ref<multi_mesh>>& meshes = chunks[chunkIndex].renderMeshes;
		const auto& renderables = chunks[chunkIndex].renderables;
		for (uint32_t i = 0; i < meshes.size(); ++i)
		{
			if (enttScene->registry.get<mesh_component>(renderables[i]).isHidden)
				continue;

			ref<multi_mesh> renderMesh = meshes[i];

			/*const px_simple_mesh* mesh = renderMesh->getMesh();
			healthBuffer.resize(mesh->vertices.size());

			for (uint32_t vertexIndex = 0; vertexIndex < mesh->vertices.size(); vertexIndex++)
			{
				PxVec3 position = mesh->vertices[vertexIndex].position;
				float health = 0.0f;
				float healthDenom = 0.0f;

				for (uint32_t adjacencyIndex = graph.adjacencyPartition[node0]; adjacencyIndex < graph.adjacencyPartition[node0 + 1]; adjacencyIndex++)
				{
					uint32_t node1 = graph.adjacentNodeIndices[adjacencyIndex];
					uint32_t bondIndex = graph.adjacentBondIndices[adjacencyIndex];
					float bondHealth = PxClamp(bondHealths[bondIndex] / bondHealthMax, 0.0f, 1.0f);
					const NvBlastBond& solverBond = bonds[bondIndex];
					const PxVec3& centroid = reinterpret_cast<const PxVec3&>(solverBond.centroid);

					float factor = 1.0f / (centroid - position).magnitudeSquared();

					health += bondHealth * factor;
					healthDenom += factor;
				}

				healthBuffer[vertexIndex] = healthDenom > 0.0f ? health / healthDenom : 1.0f;
			}*/

			//renderMesh->updateHealths(healthBuffer);
		}
	}
}

#endif