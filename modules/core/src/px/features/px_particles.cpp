// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "px/features/px_particles.h"

#include "application.h"

namespace era_engine::physics
{
	px_particles_component::px_particles_component(uint32 handle, const vec3& position, int nX, int nY, int nZ, bool fluid) : px_physics_component_base(handle), numX(nX), numY(nY), numZ(nZ)
	{
		particleSystem = make_ref<px_particle_system>(numX, numY, numZ, fluid, physx::createPxVec3(position));
		//particleSystem->translate(PxVec4(position.x, position.y, position.z, 0));
		eentity entity{ (entity_handle)entityHandle, &globalApp.getCurrentScene()->registry };
		if (!entity.hasComponent<physics::px_particles_render_component>())
			entity.addComponent<physics::px_particles_render_component>();
	}

	void px_particles_component::release(bool release)
	{
		particleSystem.reset();
	}

	void px_particles_component::update(bool visualize, ldr_render_pass* ldrRenderPass) const
	{
		particleSystem->update(visualize, ldrRenderPass);
	}

	px_particle_system::px_particle_system(PxU32 numX, PxU32 numY, PxU32 numZ, bool fluid, const PxVec3& position, PxU32 maxVols, PxU32 maxNh)
		: maxNeighborhood(maxNh), maxParticles(numX* numY* numZ), maxVolumes(maxVols)
	{
		maxParticles = numX * numY * numZ;
		const PxU32 maxDiffuseParticles = maxParticles;

		if (fluid)
		{
			// Setup PBF
			bool useLargeFluid = false;
			const PxReal particleSpacing = 0.2f;

			const PxReal fluidDensity = 1000.0f;

			PxCudaContextManager* cudaContextManager = physics_holder::physicsRef->getCudaContextManager();

			if (cudaContextManager == nullptr)
				return;

			const PxReal restOffset = 0.5f * particleSpacing / 0.6f;

			// Material setup
			material = physics_holder::physicsRef->getPhysics()->createPBDMaterial(0.05f, 0.05f, 0.f, 0.001f, 0.5f, 0.005f, 0.01f, 0.f, 0.f);

			material->setViscosity(0.001f);
			material->setSurfaceTension(0.00704f);
			material->setCohesion(0.0704f);
			material->setVorticityConfinement(10.f);

			particleSystem = physics_holder::physicsRef->getPhysics()->createPBDParticleSystem(*cudaContextManager, 96);

			// General particle system setting
			const PxReal solidRestOffset = restOffset;
			const PxReal fluidRestOffset = restOffset * 0.6f;
			const PxReal particleMass = fluidDensity * 1.333f * 3.14159f * particleSpacing * particleSpacing * particleSpacing;
			particleSystem->setRestOffset(restOffset);
			particleSystem->setContactOffset(restOffset + 0.01f);
			particleSystem->setParticleContactOffset(fluidRestOffset / 0.6f);
			particleSystem->setSolidRestOffset(solidRestOffset);
			particleSystem->setFluidRestOffset(fluidRestOffset);
			particleSystem->enableCCD(false);
			particleSystem->setMaxVelocity(solidRestOffset * 100.f);

			physics_holder::physicsRef->lockWrite();
			physics_holder::physicsRef->getScene()->addActor(*particleSystem);
			physics_holder::physicsRef->unlockWrite();

			// Diffuse particles setting
			PxDiffuseParticleParams dpParams;
			dpParams.threshold = 300.0f;
			dpParams.bubbleDrag = 0.9f;
			dpParams.buoyancy = 0.9f;
			dpParams.airDrag = 0.0f;
			dpParams.kineticEnergyWeight = 0.01f;
			dpParams.pressureWeight = 1.0f;
			dpParams.divergenceWeight = 10.f;
			dpParams.lifetime = 1.0f;
			dpParams.useAccurateVelocity = false;

			//maxDiffuseParticles = maxDiffuseParticles;

			// Create particles and add them to the particle system
			const PxU32 particlePhase = particleSystem->createPhase(material, PxParticlePhaseFlags(PxParticlePhaseFlag::eParticlePhaseFluid | PxParticlePhaseFlag::eParticlePhaseSelfCollide));

			PxU32* phase = cudaContextManager->allocPinnedHostBuffer<PxU32>(maxParticles);
			PxVec4* positionInvMass = cudaContextManager->allocPinnedHostBuffer<PxVec4>(maxParticles);
			PxVec4* velocity = cudaContextManager->allocPinnedHostBuffer<PxVec4>(maxParticles);

			PxReal x = position.x;
			PxReal y = position.y;
			PxReal z = position.z;

			for (PxU32 i = 0; i < numX; ++i)
			{
				for (PxU32 j = 0; j < numY; ++j)
				{
					for (PxU32 k = 0; k < numZ; ++k)
					{
						const PxU32 index = i * (numY * numZ) + j * numZ + k;

						PxVec4 pos(x, y, z, 1.0f / particleMass);
						phase[index] = particlePhase;
						positionInvMass[index] = pos;
						velocity[index] = PxVec4(0.0f);

						z += particleSpacing;
					}
					z = position.z;
					y += particleSpacing;
				}
				y = position.y;
				x += particleSpacing;
			}


			ExtGpu::PxParticleAndDiffuseBufferDesc bufferDesc;
			bufferDesc.maxParticles = maxParticles;
			bufferDesc.numActiveParticles = maxParticles;
			bufferDesc.maxDiffuseParticles = maxDiffuseParticles;
			bufferDesc.maxActiveDiffuseParticles = maxDiffuseParticles;
			bufferDesc.diffuseParams = dpParams;

			bufferDesc.positions = positionInvMass;
			bufferDesc.velocities = velocity;
			bufferDesc.phases = phase;

			particleBuffer = physx::ExtGpu::PxCreateAndPopulateParticleAndDiffuseBuffer(bufferDesc, cudaContextManager);

			physics_holder::physicsRef->lockWrite();
			particleSystem->addParticleBuffer(particleBuffer);
			physics_holder::physicsRef->unlockWrite();

			cudaContextManager->freePinnedHostBuffer(positionInvMass);
			cudaContextManager->freePinnedHostBuffer(velocity);
			cudaContextManager->freePinnedHostBuffer(phase);
		}
		else
		{
			static const auto cudaCM = physics_holder::physicsRef->getCudaContextManager();

			particleSystem = physics_holder::physicsRef->getPhysics()->
				createPBDParticleSystem(*cudaCM, maxNeighborhood);

			const PxReal particleSpacing = 0.2f;
			const PxReal fluidDensity = 1000.f;
			const PxReal restOffset = 0.5f * particleSpacing / 0.6f;
			const PxReal solidRestOffset = restOffset;
			const PxReal fluidRestOffset = restOffset * 0.6f;
			const PxReal renderRadius = fluidRestOffset;
			const PxReal particleMass = fluidDensity * 1.333f * M_PI * renderRadius * renderRadius * renderRadius;

			PxU32 maxDiffuseParticles = maxParticles;

			material = physics_holder::physicsRef->getPhysics()->
				createPBDMaterial(0.05f, 0.05f, 0.f, 0.001f, 0.5f, 0.005f, 0.01f, 0.f, 0.f);

			material->setViscosity(0.001f);
			material->setSurfaceTension(0.00704f);
			material->setCohesion(0.0704f);
			material->setVorticityConfinement(10.f);

			particleSystem->setRestOffset(restOffset);
			particleSystem->setContactOffset(restOffset + 0.01f);
			particleSystem->setParticleContactOffset(PxMax(solidRestOffset + 0.01f, fluidRestOffset / 0.6f));
			particleSystem->setSolidRestOffset(solidRestOffset);
			particleSystem->setFluidRestOffset(fluidRestOffset);
			particleSystem->setMaxVelocity(solidRestOffset * 100.f);
			particleSystem->enableCCD(true);

			PxFilterData filterData;
			filterData.word0 = -1; // word0 = own ID
			filterData.word1 = -1;  // word1 = ID mask to filter pairs that trigger a contact callback
			particleSystem->setSimulationFilterData(filterData);

			physics_holder::physicsRef->getScene()->addActor(*particleSystem);

			PxDiffuseParticleParams dpParams;
			dpParams.threshold = 300.0f;
			dpParams.bubbleDrag = 0.9f;
			dpParams.buoyancy = 0.9f;
			dpParams.airDrag = 0.0f;
			dpParams.kineticEnergyWeight = 0.01f;
			dpParams.pressureWeight = 1.0f;
			dpParams.divergenceWeight = 10.f;
			dpParams.lifetime = 1.0f;
			dpParams.useAccurateVelocity = false;

			const PxU32 particlePhase = particleSystem->createPhase(material, PxParticlePhaseFlags(PxParticlePhaseFlag::eParticlePhaseFluid | PxParticlePhaseFlag::eParticlePhaseSelfCollide));

			PxU32* phase = cudaCM->allocPinnedHostBuffer<PxU32>(maxParticles);
			PxVec4* positionInvMass = cudaCM->allocPinnedHostBuffer<PxVec4>(maxParticles);
			PxVec4* velocity = cudaCM->allocPinnedHostBuffer<PxVec4>(maxParticles);

			PxReal x = position.x;
			PxReal y = position.y;
			PxReal z = position.z;

			for (PxU32 i = 0; i < numX; ++i)
			{
				for (PxU32 j = 0; j < numY; ++j)
				{
					for (PxU32 k = 0; k < numZ; ++k)
					{
						const PxU32 index = i * (numY * numZ) + j * numZ + k;

						PxVec4 pos(x, y, z, 1.0f / particleMass);
						phase[index] = particlePhase;
						positionInvMass[index] = pos;
						velocity[index] = PxVec4(0.0f);

						z += particleSpacing;
					}
					z = position.z;
					y += particleSpacing;
				}
				y = position.y;
				x += particleSpacing;
			}

			ExtGpu::PxParticleAndDiffuseBufferDesc bufferDesc;
			bufferDesc.maxParticles = maxParticles;
			bufferDesc.numActiveParticles = maxParticles;
			bufferDesc.maxDiffuseParticles = maxDiffuseParticles;
			bufferDesc.maxActiveDiffuseParticles = maxDiffuseParticles;
			bufferDesc.diffuseParams = dpParams;

			bufferDesc.positions = positionInvMass;
			bufferDesc.velocities = velocity;
			bufferDesc.phases = phase;

			particleBuffer = ExtGpu::PxCreateAndPopulateParticleAndDiffuseBuffer(bufferDesc, cudaCM);
			particleSystem->addParticleBuffer(particleBuffer);

			cudaCM->freePinnedHostBuffer(positionInvMass);
			cudaCM->freePinnedHostBuffer(velocity);
			cudaCM->freePinnedHostBuffer(phase);
		}

#if PX_PARTICLE_USE_ALLOCATOR

		allocator.initialize(0, maxParticles * sizeof(PxVec4) + maxDiffuseParticles * sizeof(PxVec4) + sizeof(PxVec4));

		diffuseLifeBuffer = allocator.allocate<PxVec4>(maxDiffuseParticles, true);
		posBuffer = allocator.allocate<PxVec4>(maxParticles, true);

#else

		diffuseLifeBuffer = (PxVec4*)malloc(maxDiffuseParticles * sizeof(PxVec4));
		posBuffer = (PxVec4*)malloc(maxParticles * sizeof(PxVec4));

#endif
		setWind(PxVec3(1.0f, 0.f, 0.f));
	}

	px_particle_system::~px_particle_system()
	{
		physics_holder::physicsRef->lockWrite();
		physics_holder::physicsRef->getScene()->removeActor(*particleSystem);
		particleSystem->removeParticleBuffer(particleBuffer);
		physics_holder::physicsRef->lockWrite();

		PX_RELEASE(particleSystem)
			PX_RELEASE(material)
			PX_RELEASE(particleBuffer)

#if PX_PARTICLE_USE_ALLOCATOR

			allocator.reset(true);

#else

			free(diffuseLifeBuffer);
		free(posBuffer);

#endif
	}

	void px_particle_system::setWind(const PxVec3& wind)
	{
		particleSystem->setWind(wind);
	}

	PxVec3 px_particle_system::getWind() const
	{
		return particleSystem->getWind();
	}

	void px_particle_system::translate(const vec3& position)
	{
		translate(physx::createPxVec4(position));
	}

	void px_particle_system::translate(const PxVec4& position)
	{
		static const auto cudaCM = physics_holder::physicsRef->getCudaContextManager();
		PxVec4* bufferPos = particleBuffer->getPositionInvMasses();
		PxVec4* bufferDiffPos = particleBuffer->getDiffusePositionLifeTime();
		const PxU32 numParticles = particleBuffer->getNbActiveParticles();

		cudaCM->acquireContext();

		PxCudaContext* cudaContext = cudaCM->getCudaContext();

		PxVec4* hostBuffer = nullptr;
		cudaCM->allocPinnedHostBuffer(hostBuffer, numParticles * sizeof(PxVec4));

		const PxU32 numActiveDiffuseParticles = particleBuffer->getNbActiveDiffuseParticles();
		PxVec4* hostDiffBuffer = nullptr;
		cudaCM->allocPinnedHostBuffer(hostDiffBuffer, numActiveDiffuseParticles * sizeof(PxVec4));

		cudaContext->memcpyDtoH(hostBuffer, CUdeviceptr(bufferPos), numParticles * sizeof(PxVec4));
		cudaContext->memcpyDtoH(hostDiffBuffer, CUdeviceptr(bufferDiffPos), numActiveDiffuseParticles * sizeof(PxVec4));

		for (size_t i = 0; i < numParticles; i += 4)
		{
			hostBuffer[i + 0] = hostBuffer[i + 0] + position;
			hostBuffer[i + 1] = hostBuffer[i + 1] + position;
			hostBuffer[i + 2] = hostBuffer[i + 2] + position;
			hostBuffer[i + 3] = hostBuffer[i + 3] + position;
		}

		for (size_t i = 0; i < numActiveDiffuseParticles; i += 4)
		{
			hostDiffBuffer[i + 0] = hostDiffBuffer[i + 0] + position;
			hostDiffBuffer[i + 1] = hostDiffBuffer[i + 1] + position;
			hostDiffBuffer[i + 2] = hostDiffBuffer[i + 2] + position;
			hostDiffBuffer[i + 3] = hostDiffBuffer[i + 3] + position;
		}

		cudaContext->memcpyHtoD(CUdeviceptr(bufferPos), hostBuffer, numParticles * sizeof(PxVec4));
		cudaContext->memcpyHtoD(CUdeviceptr(bufferDiffPos), hostBuffer, numActiveDiffuseParticles * sizeof(PxVec4));

		cudaCM->releaseContext();

		particleBuffer->raiseFlags(PxParticleBufferFlag::eUPDATE_POSITION);

		cudaCM->freePinnedHostBuffer(hostBuffer);
		cudaCM->freePinnedHostBuffer(hostDiffBuffer);
	}

	void px_particle_system::syncPositionBuffer()
	{
		static const auto cudaCM = physics_holder::physicsRef->getCudaContextManager();

		PxVec4* positions = particleBuffer->getPositionInvMasses();
		PxVec4* diffusePositions = particleBuffer->getDiffusePositionLifeTime();

		const PxU32 numParticles = particleBuffer->getNbActiveParticles();
		const PxU32 numDiffuseParticles = particleBuffer->getNbActiveDiffuseParticles();

		cudaCM->acquireContext();

		PxCudaContext* cudaContext = cudaCM->getCudaContext();
		cudaContext->memcpyDtoH(posBuffer, CUdeviceptr(positions), sizeof(PxVec4) * numParticles);
		cudaContext->memcpyDtoH(diffuseLifeBuffer, CUdeviceptr(diffusePositions), sizeof(PxVec4) * numDiffuseParticles);

		cudaCM->releaseContext();
	}

	void px_particle_system::debugVisualize(ldr_render_pass* ldrRenderPass)
	{
		for (size_t i = 0; i < maxParticles; i++)
		{
			PxVec4 p_i = (PxVec4)posBuffer[i];
			vec3 pos_i = vec3(p_i.x, p_i.y, p_i.z);
			renderPoint(pos_i, vec4(0.107f, 1.0f, 0.0f, 1.0f), ldrRenderPass, false);
		}

		const PxU32 numActiveDiffuseParticles = particleBuffer->getNbActiveDiffuseParticles();

		static PxVec3 colorDiffuseParticles(1, 1, 1);

		if (numActiveDiffuseParticles > 0)
		{
			for (size_t i = 0; i < numActiveDiffuseParticles; i++)
			{
				PxVec4 p_i = (PxVec4)diffuseLifeBuffer[i];
				vec3 pos_i = vec3(p_i.x, p_i.y, p_i.z);
				renderPoint(pos_i, vec4(1, 0, 0, 1), ldrRenderPass, false);
			}
		}
	}


}