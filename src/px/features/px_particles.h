#pragma once
#include <core/math.h>
#include <px/core/px_physics_engine.h>

struct px_particle_system
{
	px_particle_system() = default;
	px_particle_system(PxU32 numX, PxU32 numY, PxU32 numZ, const PxVec3& position = PxVec3(0, 0, 0), PxU32 maxVols = 1, PxU32 maxNh = 96) noexcept
		: maxNeighborhood(maxNh), maxParticles(numX * numY * numZ), maxVolumes(maxVols)
	{
		const auto cudaCM = px_physics_engine::get()->getPhysicsAdapter()->cudaContextManager;

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
	}

	~px_particle_system()
	{
		px_physics_engine::get()->getPhysicsAdapter()->scene->removeActor(*particleSystem);
		particleSystem->removeParticleBuffer(particleBuffer);

		PX_RELEASE(particleSystem)
		PX_RELEASE(material)
		PX_RELEASE(particleBuffer)
	}

	void setPositions(PxVec4* positionsHost) noexcept
	{
		const auto cudaCM = px_physics_engine::get()->getPhysicsAdapter()->cudaContextManager;
		PxVec4* bufferPos = particleBuffer->getPositionInvMasses();
		cudaCM->copyHToDAsync(bufferPos, positionsHost, maxParticles * sizeof(PxVec4), 0);
		particleBuffer->raiseFlags(PxParticleBufferFlag::eUPDATE_POSITION);
	}

	PxPBDParticleSystem* particleSystem = nullptr;
	PxParticleAndDiffuseBuffer* particleBuffer = nullptr;
	PxPBDMaterial* material = nullptr;

	PxU32 maxNeighborhood{};
	PxU32 maxVolumes{};
	PxU32 maxParticles{};
};