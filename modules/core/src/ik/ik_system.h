#pragma once

#include "ecs/system.h"

#include "core/math.h"

namespace era_engine
{
	namespace animation
	{
		class Skeleton;
		class SkeletonPose;
	}

	struct IKChain;

	class RendererHolderRootComponent;

	class IKSystem final : public System
	{
	public:
		IKSystem(World* _world);
		~IKSystem();

		void init() override;
		void update(float dt) override;

		void debug_draw(float dt);

		void solve_chain(IKChain& chain,
			const animation::Skeleton& skeleton,
			animation::SkeletonPose& pose,
			const trs& world_transform);

		void solve_fabrik(IKChain& chain,
			const animation::Skeleton& skeleton,
			const animation::SkeletonPose& pose,
			const trs& world_transform);

		void write_back(IKChain& chain,
			const animation::Skeleton& skeleton,
			animation::SkeletonPose& pose, 
			const trs& world_transform);

		void apply_pole_vector(IKChain& chain, 
			uint32 joint_idx,
			const trs& world_transform);

		ERA_VIRTUAL_REFLECT(System)

	private:
		RendererHolderRootComponent* renderer_holder_rc = nullptr;
	};
}