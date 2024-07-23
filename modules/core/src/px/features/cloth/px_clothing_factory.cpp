// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "px/features/cloth/px_clothing_factory.h"

#include "animation/skinning.h"

#include "application.h"

namespace era_engine::physics
{
	NODISCARD std::tuple<dx_vertex_buffer_group_view, dx_vertex_buffer_group_view, dx_index_buffer_view, submesh_info> px_cloth_render_component::getRenderData(const px_cloth_component& cloth)
	{
		CPU_PROFILE_BLOCK("Get cloth render data");

		uint32 numVertices = cloth.numX * cloth.numZ;
		uint32 numTriangles = (cloth.numX - 1) * (cloth.numZ - 1) * 2;

		auto [positionVertexBuffer, positionPtr] = dxContext.createDynamicVertexBuffer(sizeof(vec3), numVertices);
		memcpy(positionPtr, cloth.getPositions(), numVertices * sizeof(vec3));

		dx_vertex_buffer_group_view vb = era_engine::animation::skinCloth(positionVertexBuffer, cloth.numX, cloth.numZ);
		submesh_info sm{};
		sm.baseVertex = 0;
		sm.firstIndex = 0;
		sm.numIndices = numTriangles * 3;
		sm.numVertices = numVertices;

		dx_vertex_buffer_group_view prev = prevFrameVB;
		prevFrameVB = vb;

		if (!indexBuffer)
		{
			::std::vector<indexed_triangle16> triangles;
			triangles.reserve(numTriangles);
			for (uint32 y = 0; y < cloth.numZ - 1; ++y)
			{
				for (uint32 x = 0; x < cloth.numX - 1; ++x)
				{
					uint16 tlIndex = y * cloth.numX + x;
					uint16 trIndex = tlIndex + 1;
					uint16 blIndex = tlIndex + cloth.numX;
					uint16 brIndex = blIndex + 1;

					triangles.push_back({ tlIndex, blIndex, brIndex });
					triangles.push_back({ tlIndex, brIndex, trIndex });
				}
			}

			indexBuffer = createIndexBuffer(sizeof(uint16), (uint32)triangles.size() * 3, triangles.data());
		}

		return { vb, prev, indexBuffer, sm };
	}

	px_cloth_component::px_cloth_component(uint32 handle, int nX, int nZ, const vec3& position) : px_physics_component_base(handle), numX(nX), numZ(nZ)
	{
		clothSystem = make_ref<px_cloth_system>(numX, numZ, physx::createPxVec3(position));
		positions = (vec3*)malloc(numX * numZ * sizeof(vec3));
		//clothSystem->translate(PxVec4(position.x, position.y, position.z, 0));
		eentity entity{ (entity_handle)entityHandle, &globalApp.getCurrentScene()->registry };
		if (!entity.hasComponent<physics::px_cloth_render_component>())
			entity.addComponent<physics::px_cloth_render_component>();
	}

	vec3* px_cloth_component::getPositions() const
	{
		PxVec4* bufferPos = clothSystem->posBuffer;

		for (uint32 i = 0; i < numX * numZ; i += 4)
		{
			positions[i + 0] = vec3(bufferPos[i + 0].x, bufferPos[i + 0].y, bufferPos[i + 0].z);
			positions[i + 1] = vec3(bufferPos[i + 1].x, bufferPos[i + 1].y, bufferPos[i + 1].z);
			positions[i + 2] = vec3(bufferPos[i + 2].x, bufferPos[i + 2].y, bufferPos[i + 2].z);
			positions[i + 3] = vec3(bufferPos[i + 3].x, bufferPos[i + 3].y, bufferPos[i + 3].z);
		}

		return positions;
	}

	void px_cloth_component::release(bool release)
	{
		clothSystem.reset();
		free(positions);
	}

	void px_cloth_component::update(bool visualize, ldr_render_pass* ldrRenderPass) const
	{
		clothSystem->update(visualize, ldrRenderPass);
	}

	void px_cloth_component::translate(const vec3& position)
	{
		clothSystem->translate(position);
	}

	px_cloth_system::px_cloth_system(PxU32 numX, PxU32 numZ, const PxVec3& position, PxReal particleSpacing, PxReal totalClothMass)
	{
		const auto cudaCM = physics_holder::physicsRef->getCudaContextManager();

		numParticles = numX * numZ;
		const PxU32 numSprings = (numX - 1) * (numZ - 1) * 4 + (numX - 1) + (numZ - 1);
		const PxU32 numTriangles = (numX - 1) * (numZ - 1) * 2;

		const PxReal restOffset = particleSpacing;

		const PxReal stretchStiffness = 10000.f;
		const PxReal shearStiffness = 100.f;
		const PxReal springDamping = 0.001f;

		material = physics_holder::physicsRef->getPhysics()->
			createPBDMaterial(0.8f, 0.05f, 1e+6f, 0.001f, 0.5f, 0.005f, 0.05f, 0.f, 0.f);

		particleSystem = physics_holder::physicsRef->getPhysics()->
			createPBDParticleSystem(*cudaCM);

		const PxReal particleMass = totalClothMass / numParticles;
		particleSystem->setRestOffset(restOffset);
		particleSystem->setContactOffset(restOffset + 0.02f);
		particleSystem->setParticleContactOffset(restOffset + 0.02f);
		particleSystem->setSolidRestOffset(restOffset);
		particleSystem->setFluidRestOffset(0.0f);
		particleSystem->enableCCD(true);

		physics_holder::physicsRef->lockWrite();
		physics_holder::physicsRef->getScene()->addActor(*particleSystem);
		physics_holder::physicsRef->unlockWrite();

		const PxU32 particlePhase = particleSystem->createPhase(material, PxParticlePhaseFlags(PxParticlePhaseFlag::eParticlePhaseSelfCollideFilter | PxParticlePhaseFlag::eParticlePhaseSelfCollide));

		ExtGpu::PxParticleClothBufferHelper* clothBuffers = ExtGpu::PxCreateParticleClothBufferHelper(1, numTriangles, numSprings, numParticles, cudaCM);

		PxU32* phase = cudaCM->allocPinnedHostBuffer<PxU32>(numParticles);
		PxVec4* positionInvMass = cudaCM->allocPinnedHostBuffer<PxVec4>(numParticles);
		PxVec4* velocity = cudaCM->allocPinnedHostBuffer<PxVec4>(numParticles);

		PxReal x = position.x;
		PxReal y = position.y;
		PxReal z = position.z;

		PxArray<PxParticleSpring> springs;
		springs.reserve(numSprings);
		PxArray<PxU32> triangles;
		triangles.reserve(numTriangles * 3);

		for (PxU32 i = 0; i < numX; ++i)
		{
			for (PxU32 j = 0; j < numZ; ++j)
			{
				const PxU32 index = i * numZ + j;

				PxVec4 pos(x, y, z, 1.0f / particleMass);
				phase[index] = particlePhase;
				positionInvMass[index] = pos;
				velocity[index] = PxVec4(0.0f);

				if (i > 0)
				{
					PxParticleSpring spring = { id(i - 1, j, numZ), id(i, j, numZ), particleSpacing, stretchStiffness, springDamping, 0 };
					springs.pushBack(spring);
				}
				if (j > 0)
				{
					PxParticleSpring spring = { id(i, j - 1, numZ), id(i, j, numZ), particleSpacing, stretchStiffness, springDamping, 0 };
					springs.pushBack(spring);
				}

				if (i > 0 && j > 0)
				{
					PxParticleSpring spring0 = { id(i - 1, j - 1, numZ), id(i, j, numZ), PxSqrt(2.0f) * particleSpacing, shearStiffness, springDamping, 0 };
					springs.pushBack(spring0);
					PxParticleSpring spring1 = { id(i - 1, j, numZ), id(i, j - 1, numZ), PxSqrt(2.0f) * particleSpacing, shearStiffness, springDamping, 0 };
					springs.pushBack(spring1);

					//Triangles are used to compute approximated aerodynamic forces for cloth falling down
					triangles.pushBack(id(i - 1, j - 1, numZ));
					triangles.pushBack(id(i - 1, j, numZ));
					triangles.pushBack(id(i, j - 1, numZ));

					triangles.pushBack(id(i - 1, j, numZ));
					triangles.pushBack(id(i, j - 1, numZ));
					triangles.pushBack(id(i, j, numZ));
				}

				z += particleSpacing;
			}
			z = position.z;
			x += particleSpacing;
		}

		PX_ASSERT(numSprings == springs.size());
		PX_ASSERT(numTriangles == triangles.size() / 3);

		clothBuffers->addCloth(0.0f, 0.0f, 0.0f, triangles.begin(), numTriangles, springs.begin(), numSprings, positionInvMass, numParticles);

		ExtGpu::PxParticleBufferDesc bufferDesc;
		bufferDesc.maxParticles = numParticles;
		bufferDesc.numActiveParticles = numParticles;
		bufferDesc.positions = positionInvMass;
		bufferDesc.velocities = velocity;
		bufferDesc.phases = phase;

		const PxParticleClothDesc& clothDesc = clothBuffers->getParticleClothDesc();
		PxParticleClothPreProcessor* clothPreProcessor = PxCreateParticleClothPreProcessor(cudaCM);

		PxPartitionedParticleCloth output;
		clothPreProcessor->partitionSprings(clothDesc, output);
		clothPreProcessor->release();

		clothBuffer = ExtGpu::PxCreateAndPopulateParticleClothBuffer(bufferDesc, clothDesc, output, cudaCM);

		physics_holder::physicsRef->lockWrite();
		particleSystem->addParticleBuffer(clothBuffer);
		physics_holder::physicsRef->unlockWrite();

		clothBuffers->release();

		cudaCM->freePinnedHostBuffer(positionInvMass);
		cudaCM->freePinnedHostBuffer(velocity);
		cudaCM->freePinnedHostBuffer(phase);

#if PX_PARTICLE_USE_ALLOCATOR

		uint32 size = numParticles * sizeof(PxVec4);
		allocator.initialize(0, size + sizeof(PxVec4));
		posBuffer = allocator.allocate<PxVec4>(numParticles, true);

#else

		posBuffer = (PxVec4*)malloc(numParticles * sizeof(PxVec4));

#endif
	}

	px_cloth_system::~px_cloth_system()
	{
		physics_holder::physicsRef->lockWrite();
		physics_holder::physicsRef->getScene()->removeActor(*particleSystem);
		particleSystem->removeParticleBuffer(clothBuffer);
		physics_holder::physicsRef->unlockWrite();

		PX_RELEASE(particleSystem)
			PX_RELEASE(material)
			PX_RELEASE(clothBuffer)

#if PX_PARTICLE_USE_ALLOCATOR

			allocator.reset(true);

#else

			free(posBuffer);

#endif
	}

	void px_cloth_system::setWind(const PxVec3& wind)
	{
		particleSystem->setWind(wind);
	}

	PxVec3 px_cloth_system::getWind() const
	{
		return particleSystem->getWind();
	}

	void px_cloth_system::syncPositionBuffer()
	{
		static const auto cudaCM = physics_holder::physicsRef->getCudaContextManager();

		PxVec4* positions = clothBuffer->getPositionInvMasses();

		const PxU32 numParticles = clothBuffer->getNbActiveParticles();

		cudaCM->acquireContext();

		PxCudaContext* cudaContext = cudaCM->getCudaContext();
		cudaContext->memcpyDtoH(posBuffer, CUdeviceptr(positions), sizeof(PxVec4) * numParticles);

		cudaCM->releaseContext();
	}

	void px_cloth_system::translate(const PxVec4& position)
	{
		{
			static const auto cudaCM = physics_holder::physicsRef->getCudaContextManager();
			PxVec4* bufferPos = clothBuffer->getPositionInvMasses();
			const PxU32 numParticles = clothBuffer->getNbActiveParticles();

			cudaCM->acquireContext();

			PxCudaContext* cudaContext = cudaCM->getCudaContext();

			PxVec4* hostBuffer = nullptr;
			cudaCM->allocPinnedHostBuffer(hostBuffer, numParticles * sizeof(PxVec4));

			cudaContext->memcpyDtoH(hostBuffer, CUdeviceptr(bufferPos), numParticles * sizeof(PxVec4));

			for (size_t i = 0; i < numParticles; i += 4)
			{
				hostBuffer[i + 0] = hostBuffer[i + 0] + position;
				hostBuffer[i + 1] = hostBuffer[i + 1] + position;
				hostBuffer[i + 2] = hostBuffer[i + 2] + position;
				hostBuffer[i + 3] = hostBuffer[i + 3] + position;
			}

			cudaContext->memcpyHtoD(CUdeviceptr(bufferPos), hostBuffer, numParticles * sizeof(PxVec4));

			cudaCM->releaseContext();

			clothBuffer->raiseFlags(PxParticleBufferFlag::eUPDATE_POSITION);

			cudaCM->freePinnedHostBuffer(hostBuffer);
		}
	}

	void px_cloth_system::translate(const vec3& position)
	{
		translate(createPxVec4(position));
	}

	void px_cloth_system::update(bool visualize, ldr_render_pass* ldrRenderPass)
	{
		syncPositionBuffer();
		if (visualize && ldrRenderPass)
			debugVisualize(ldrRenderPass);
	}

	void px_cloth_system::debugVisualize(ldr_render_pass* ldrRenderPass) const
	{
		for (size_t i = 0; i < numParticles; i++)
		{
			PxVec4 p_i = (PxVec4)posBuffer[i];
			vec3 pos_i = vec3(p_i.x, p_i.y, p_i.z);
			renderPoint(pos_i, vec4(0.107f, 1.0f, 0.0f, 1.0f), ldrRenderPass, false);
		}
	}

}

