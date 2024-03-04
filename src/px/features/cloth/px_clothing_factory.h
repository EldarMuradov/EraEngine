#pragma once
#include <core/math.h>
#include <px/core/px_physics_engine.h>

struct px_cloth_system
{
	px_cloth_system(PxU32 numX, PxU32 numZ, const PxVec3& position = PxVec3(0, 0, 0), PxReal particleSpacing = 0.2f, PxReal totalClothMass = 10.f) noexcept
	{
		const auto cudaCM = px_physics_engine::get()->getPhysicsAdapter()->cudaContextManager;

		const PxU32 numParticles = numX * numZ;
		const PxU32 numSprings = (numX - 1) * (numZ - 1) * 4 + (numX - 1) + (numZ - 1);
		const PxU32 numTriangles = (numX - 1) * (numZ - 1) * 2;

		const PxReal restOffset = particleSpacing;

		const PxReal stretchStiffness = 10000.f;
		const PxReal shearStiffness = 100.f;
		const PxReal springDamping = 0.001f;

		material = px_physics_engine::getPhysics()->
			createPBDMaterial(0.8f, 0.05f, 1e+6f, 0.001f, 0.5f, 0.005f, 0.05f, 0.f, 0.f);

		particleSystem = px_physics_engine::getPhysics()->
			createPBDParticleSystem(*cudaCM);

		const PxReal particleMass = totalClothMass / numParticles;
		particleSystem->setRestOffset(restOffset);
		particleSystem->setContactOffset(restOffset + 0.02f);
		particleSystem->setParticleContactOffset(restOffset + 0.02f);
		particleSystem->setSolidRestOffset(restOffset);
		particleSystem->setFluidRestOffset(0.0f);

		px_physics_engine::get()->getPhysicsAdapter()->scene->addActor(*particleSystem);

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

		clothBuffer = physx::ExtGpu::PxCreateAndPopulateParticleClothBuffer(bufferDesc, clothDesc, output, cudaCM);
		particleSystem->addParticleBuffer(clothBuffer);

		clothBuffers->release();

		cudaCM->freePinnedHostBuffer(positionInvMass);
		cudaCM->freePinnedHostBuffer(velocity);
		cudaCM->freePinnedHostBuffer(phase);
	}

	~px_cloth_system()
	{
		px_physics_engine::get()->getPhysicsAdapter()->scene->removeActor(*particleSystem);
		particleSystem->removeParticleBuffer(clothBuffer);

		PX_RELEASE(particleSystem)
		PX_RELEASE(material)
		PX_RELEASE(clothBuffer)
	}

	PxPBDMaterial* material = nullptr;
	PxPBDParticleSystem* particleSystem = nullptr;
	PxParticleClothBuffer* clothBuffer = nullptr;
};