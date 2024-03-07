#pragma once
#include <core/math.h>
#include <px/core/px_physics_engine.h>
#include <core/memory.h>
#include <rendering/debug_visualization.h>

struct px_particle_system
{
	px_particle_system() = default;
	px_particle_system(PxU32 numX, PxU32 numY, PxU32 numZ, const PxVec3& position = PxVec3(0, 0, 0), PxU32 maxVols = 1, PxU32 maxNh = 96) noexcept
		: maxNeighborhood(maxNh), maxParticles(numX * numY * numZ), maxVolumes(maxVols)
	{
		static const auto cudaCM = px_physics_engine::get()->getPhysicsAdapter()->cudaContextManager;

		particleSystem = px_physics_engine::getPhysics()->
			createPBDParticleSystem(*cudaCM, maxNeighborhood);

		const PxReal particleSpacing = 0.2f;
		const PxReal fluidDensity = 1000.f;
		const PxReal restOffset = 0.5f * particleSpacing / 0.6f;
		const PxReal solidRestOffset = restOffset;
		const PxReal fluidRestOffset = restOffset * 0.6f;
		const PxReal renderRadius = fluidRestOffset;
		const PxReal particleMass = fluidDensity * 1.333f * M_PI * renderRadius * renderRadius * renderRadius;

		PxU32 maxDiffuseParticles = maxParticles;

		material = px_physics_engine::getPhysics()->
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

		px_physics_engine::get()->getPhysicsAdapter()->scene->addActor(*particleSystem);

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

		particleBuffer = physx::ExtGpu::PxCreateAndPopulateParticleAndDiffuseBuffer(bufferDesc, cudaCM);
		particleSystem->addParticleBuffer(particleBuffer);

		cudaCM->freePinnedHostBuffer(positionInvMass);
		cudaCM->freePinnedHostBuffer(velocity);
		cudaCM->freePinnedHostBuffer(phase);

#if PX_PARTICLE_USE_ALLOCATOR

		allocator.initialize(0, maxParticles * sizeof(PxVec4) + maxDiffuseParticles * sizeof(PxVec4) + sizeof(PxVec4));

		diffuseLifeBuffer = allocator.allocate<PxVec4>(maxDiffuseParticles, true);
		posBuffer = allocator.allocate<PxVec4>(maxParticles, true);

#else

		diffuseLifeBuffer = (PxVec4*)malloc(maxDiffuseParticles * sizeof(PxVec4));
		posBuffer = (PxVec4*)malloc(maxParticles * sizeof(PxVec4));

#endif

	}

	~px_particle_system()
	{
		px_physics_engine::get()->getPhysicsAdapter()->scene->removeActor(*particleSystem);
		particleSystem->removeParticleBuffer(particleBuffer);
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

	void setWind(const PxVec3& wind) noexcept
	{
		particleSystem->setWind(wind);
	}

	PxVec3 getWind() const noexcept
	{
		return particleSystem->getWind();
	}

	void setPosition(const PxVec4& position) noexcept
	{
		static const auto cudaCM = px_physics_engine::get()->getPhysicsAdapter()->cudaContextManager;
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

	void syncPositionBuffer()
	{
		static const auto cudaCM = px_physics_engine::get()->getPhysicsAdapter()->cudaContextManager;

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

	void update(bool visualize = false, ldr_render_pass* ldrRenderPass = nullptr)
	{
		syncPositionBuffer();
		if (visualize && ldrRenderPass)
			debugVisualize(ldrRenderPass);
	}

	void debugVisualize(ldr_render_pass* ldrRenderPass) noexcept
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

	constexpr uint32 getMaxVolumes() const noexcept { return maxVolumes; }
	constexpr uint32 getMaxNeighborhood() const noexcept { return maxNeighborhood; }
	constexpr uint32 getMaxParticles() const noexcept { return maxParticles; }

	PxVec4* diffuseLifeBuffer = nullptr;
	PxVec4* posBuffer = nullptr;

private:

#if PX_PARTICLE_USE_ALLOCATOR

	eallocator allocator;

#endif

	PxPBDParticleSystem* particleSystem = nullptr;
	PxParticleAndDiffuseBuffer* particleBuffer = nullptr;
	PxPBDMaterial* material = nullptr;

	PxU32 maxNeighborhood{};
	PxU32 maxVolumes{};
	PxU32 maxParticles{};
};

struct px_particles_component
{
	px_particles_component() = default;
	px_particles_component(int nX, int nY, int nZ, const vec3& position = vec3(0, 0, 0)) noexcept : numX(nX), numY(nY), numZ(nZ)
	{
		particleSystem = make_ref<px_particle_system>(numX, numY, numZ, physx::createPxVec3(position));
		particleSystem->setPosition(PxVec4(position.x, position.y, position.z, 0));
	}

	~px_particles_component() {}

	void release() noexcept { particleSystem.reset(); }

	uint32 numX{};
	uint32 numY{};
	uint32 numZ{};

	ref<px_particle_system> particleSystem;
};