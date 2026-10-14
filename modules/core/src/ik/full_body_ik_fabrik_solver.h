#pragma once

#include "core_api.h"

#include "ik/full_body_ik_solver.h"

namespace era_engine
{
	// Multi-effector FABRIK with joint rotations.
	class ERA_CORE_API FBIKFabrikSolver final : public FBIKSolver
	{
	protected:
		void solve_nodes(const std::vector<IKEffector>& effectors, const FBIKSettings& settings) override;

	private:
		void backward_pass();
		void forward_pass(const std::vector<IKEffector>& effectors);

		std::vector<vec3> backward_positions;
		std::vector<uint8> position_goals; // Position effectors and their ancestors.

		std::vector<vec3> fit_from;
		std::vector<vec3> fit_to;
	};
}
