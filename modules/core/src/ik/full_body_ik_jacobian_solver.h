#pragma once

#include "core_api.h"

#include "ik/full_body_ik_solver.h"

#include "core/dense_matrix.h"

namespace era_engine
{
	// Jacobian solver (DLS).
	class ERA_CORE_API FBIKJacobianSolver final : public FBIKSolver
	{
	protected:
		void on_init(const std::vector<IKEffector>& effectors) override;
		void solve_nodes(const std::vector<IKEffector>& effectors, const FBIKSettings& settings) override;

	private:
		struct Column
		{
			uint32 node = INVALID_JOINT;
			vec3 frame_axis = vec3(0.0f);
			float scale = 1.0f; // 1 - stiffness.
			bool is_linear = false;
		};

		struct EffectorChain
		{
			uint32 node = INVALID_JOINT;
			std::vector<uint8> nodes; // 1 for the nodes the effector moves.
		};

		struct Row
		{
			uint32 effector = INVALID_JOINT;
			uint32 node = INVALID_JOINT;
			vec3 goal = vec3(0.0f);
			bool is_rotation = false;
		};

		bool solve_step(const std::vector<IKEffector>& effectors, const FBIKSettings& settings);
		void apply_step();
		void apply_constraints();
		void solve_limbs(const std::vector<IKEffector>& effectors);

		std::vector<Column> columns;
		std::vector<EffectorChain> chains;
		std::vector<float> chain_fractions; // [0, 1] from the chain root to the effector.

		std::vector<vec3> column_world_axes;
		std::vector<float> motion_strengths;

		std::vector<Row> rows;

		MatrixX jacobian;
		MatrixX system_matrix;
		VectorX target_vector;
		VectorX deltas;
	};
}
