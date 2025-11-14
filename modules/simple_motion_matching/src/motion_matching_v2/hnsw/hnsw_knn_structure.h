#pragma once 

#include "motion_matching_api.h"

#include "motion_matching_v2/motion_matching_knn_structure.h"

#include <hnswlib/hnsw_all.h>

namespace era_engine 
{
	class ERA_MOTION_MATCHING_API HnswKnnStructure : public KnnStructure
	{
	public:
		~HnswKnnStructure() override;

		void build_structure(const MotionMatchingDatabase& database) override;

		std::vector<std::shared_ptr<MotionMatchingDatabase::Sample>> search_knn(float* query, uint32 query_size, uint32 max_candidates, const MotionMatchingDatabase& database) override;

		bool serialize(std::ostream& os, const MotionMatchingDatabase& database) const override;
		bool deserialize(std::istream& is, const MotionMatchingDatabase& database) override;

	public:
		uint32 max_edges_per_vertex = 10;
		uint32 construction_exploration_factor = 50;

	private:
		hnswlib::L2Space hnsw_l2_space = hnswlib::L2Space(0);
		std::shared_ptr<hnswlib::HierarchicalNSW<float>> hnsw;
	};
}