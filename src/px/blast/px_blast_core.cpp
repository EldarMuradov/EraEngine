#include <pch.h>
#include <px/blast/px_blast_core.h>
#include <../ext/PhysX/blast/sdk/extensions/serialization/include/NvBlastExtPxSerialization.h>
#include <numeric>
#include <cooking/PxCooking.h>

static const float RIGIDBODY_DENSITY = 2000.0f;

physics::px_blast::px_blast()
	: eventCallback(nullptr)
	, debugRenderMode(px_blast_family::DEBUG_RENDER_DISABLED)
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
	impactDamageToStressFactor = 0.01f;
	draggingToStressFactor = 100.0f;
}

void physics::px_blast::reinitialize()
{
	onSampleStop();
	onSampleStart();
}

void physics::px_blast::onSampleStart()
{
	framework = NvBlastTkFrameworkCreate();

	replay = new px_blast_replay();
	PxScene* scene[1];
	PxGetPhysics().getScenes(scene, 1);

	taskManager = PxTaskManager::createTaskManager(errorReporter, scene[0]->getCpuDispatcher());

	TkGroupDesc gdesc{};
	gdesc.workerCount = taskManager->getCpuDispatcher()->getWorkerCount();
	tkGroup = framework->createGroup(gdesc);

	extPxManager = ExtPxManager::create(PxGetPhysics(), *framework, createPxJointCallback);
	extPxManager->setActorCountLimit(rigidBodyLimitEnabled ? rigidBodyLimit : 0);
	extImpactDamageManager = ExtImpactDamageManager::create(extPxManager, extImpactDamageManagerSettings);
	eventCallback = new px_blast_event_callback(extImpactDamageManager);

	extGroupTaskManager = ExtGroupTaskManager::create(*taskManager);
	extGroupTaskManager->setGroup(tkGroup);

	setImpactDamageEnabled(impactDamageEnabled, true);

	extSerialization = NvBlastExtSerializationCreate();
	if (extSerialization != nullptr)
	{
		NvBlastExtTkSerializerLoadSet(*framework, *extSerialization);

		// TODO: Switch serialization to PhysX 5.3.1
		/*NvBlastExtPxSerializerLoadSet(*framework, PxGetPhysics(),
			, *extSerialization);*/
	}
}

void physics::px_blast::onSampleStop()
{
	removeAllFamilies();

	PX_RELEASE(extImpactDamageManager);
	RELEASE_PTR(extPxManager);
	RELEASE_PTR(eventCallback);
	PX_RELEASE(tkGroup);
	RELEASE_PTR(replay);
	PX_RELEASE(framework);
	PX_RELEASE(extGroupTaskManager);
	PX_RELEASE(taskManager);
	PX_RELEASE(extSerialization);
}

void physics::px_blast::animate(double dt)
{
}

void physics::px_blast::drawUI()
{
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
		physics_holder::physicsRef->getScene()->setSimulationEventCallback(impactDamageEnabled ? eventCallback :
			nullptr);
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

physics::px_blast_asset_boxes::px_blast_asset_boxes(TkFramework& framework, PxPhysics& physics, PxCooking& cooking, main_renderer& renderer, const desc& desc)
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
	const auto& actorDesc = pxAsset->getDefaultActorDesc();
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
}

void physics::px_blast_family::drawStatsUI()
{
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
}

physics::px_blast_family_boxes::~px_blast_family_boxes()
{
}

void physics::px_blast_family_boxes::onActorCreated(const ExtPxActor& actor)
{
}

void physics::px_blast_family_boxes::onActorUpdate(const ExtPxActor& actor)
{
}

void physics::px_blast_family_boxes::onActorDestroyed(const ExtPxActor& actor)
{
}

physics::px_blast_replay::px_blast_replay()
{
}

physics::px_blast_replay::~px_blast_replay()
{
}

void physics::px_blast_replay::addFamily(TkFamily* family)
{
}

void physics::px_blast_replay::removeFamily(TkFamily* family)
{
}

void physics::px_blast_replay::startRecording(ExtPxManager& manager, bool syncFamily, bool syncPhysics)
{
}

void physics::px_blast_replay::stopRecording()
{
}

void physics::px_blast_replay::startPlayback(ExtPxManager& manager, TkGroup* group)
{
}

void physics::px_blast_replay::stopPlayback()
{
}

void physics::px_blast_replay::update()
{
}

void physics::px_blast_replay::reset()
{
}

void physics::px_blast_replay::clearBuffer()
{
}
