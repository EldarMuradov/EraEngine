#include "motion_matching/motion_matching_database.h"
#include "motion_matching/motion_matching_knn_structure.h"
#include "motion_matching/features/motion_matching_feature.h"
#include "motion_matching/motion_matching_anim_marker.h"
#include "motion_matching/common.h"
#include "motion_matching/hnsw/hnsw_knn_structure.h"

#include <animation/animation_clip.h>
#include <animation/animation_pose_sampler.h>
#include <animation/skeleton.h>

#include <asset/file_registry.h>

#include <asset/io.h>
#include <core/io/binary_data.h>
#include <core/io/binary_data_archive.h>

#include <core/math.h>
#include <core/log.h>

#include <core/traits.h>

namespace era_engine
{
	// File format: magic + version, bump the version on any layout change.
	static constexpr uint32 MOTION_DATABASE_MAGIC = 0x444D4D45; // 'EMMD'
	static constexpr uint32 MOTION_DATABASE_VERSION = 4;

	static float square_distance(const std::vector<float>& lhs, const std::vector<float>& rhs)
	{
		ASSERT(lhs.size() == rhs.size());

		float result = 0.0f;
		for (size_t index = 0; index < lhs.size(); ++index)
		{
			const float diff = lhs[index] - rhs[index];
			result += squaref(diff);
		}

		return result;
	}

	// Cyclic Jacobi eigen decomposition of a symmetric n x n matrix (row-major, double precision).
	// On return the diagonal of `a` holds the eigenvalues and the columns of `v` the eigenvectors.
	static void jacobi_eigen_decomposition(std::vector<double>& a, std::vector<double>& v, uint32 n, uint32 max_sweeps = 100)
	{
		auto at = [n](std::vector<double>& m, uint32 row, uint32 column) -> double&
			{
				return m[size_t(row) * n + column];
			};

		v.assign(size_t(n) * n, 0.0);
		for (uint32 i = 0; i < n; ++i)
		{
			at(v, i, i) = 1.0;
		}

		for (uint32 sweep = 0; sweep < max_sweeps; ++sweep)
		{
			double off_diagonal_norm = 0.0;
			double total_norm = 0.0;
			for (uint32 i = 0; i < n; ++i)
			{
				for (uint32 j = 0; j < n; ++j)
				{
					const double value = at(a, i, j) * at(a, i, j);
					total_norm += value;
					if (i != j)
					{
						off_diagonal_norm += value;
					}
				}
			}

			if (total_norm == 0.0 || off_diagonal_norm <= 1e-24 * total_norm)
			{
				break;
			}

			for (uint32 p = 0; p < n; ++p)
			{
				for (uint32 q = p + 1; q < n; ++q)
				{
					const double apq = at(a, p, q);
					if (apq == 0.0)
					{
						continue;
					}

					const double app = at(a, p, p);
					const double aqq = at(a, q, q);

					// Rotation J in the (p, q) plane that zeroes a(p, q) in J^T * A * J.
					const double theta = 0.5 * std::atan2(2.0 * apq, aqq - app);
					const double c = std::cos(theta);
					const double s = std::sin(theta);

					for (uint32 k = 0; k < n; ++k)
					{
						const double akp = at(a, k, p);
						const double akq = at(a, k, q);
						at(a, k, p) = c * akp - s * akq;
						at(a, k, q) = s * akp + c * akq;
					}

					for (uint32 k = 0; k < n; ++k)
					{
						const double apk = at(a, p, k);
						const double aqk = at(a, q, k);
						at(a, p, k) = c * apk - s * aqk;
						at(a, q, k) = s * apk + c * aqk;
					}

					at(a, p, q) = 0.0;
					at(a, q, p) = 0.0;

					for (uint32 k = 0; k < n; ++k)
					{
						const double vkp = at(v, k, p);
						const double vkq = at(v, k, q);
						at(v, k, p) = c * vkp - s * vkq;
						at(v, k, q) = s * vkp + c * vkq;
					}
				}
			}
		}
	}

	// PCA of mean-centered row samples (rows = samples, cols = features).
	// Returns a (num_components x features) matrix whose rows are the principal axes, sorted by descending variance.
	static MatrixX build_pca_projection(const MatrixX& centered_data, uint32 num_components)
	{
		const uint32 num_samples = centered_data.rows();
		const uint32 num_features = centered_data.cols();

		ASSERT(num_components > 0 && num_components <= num_features);

		std::vector<double> covariance(size_t(num_features) * num_features, 0.0);
		for (uint32 sample = 0; sample < num_samples; ++sample)
		{
			for (uint32 i = 0; i < num_features; ++i)
			{
				const double xi = centered_data(sample, i);
				if (xi == 0.0)
				{
					continue;
				}

				for (uint32 j = i; j < num_features; ++j)
				{
					covariance[size_t(i) * num_features + j] += xi * double(centered_data(sample, j));
				}
			}
		}

		const double denominator = num_samples > 1 ? double(num_samples - 1) : 1.0;
		for (uint32 i = 0; i < num_features; ++i)
		{
			for (uint32 j = i; j < num_features; ++j)
			{
				const double value = covariance[size_t(i) * num_features + j] / denominator;
				covariance[size_t(i) * num_features + j] = value;
				covariance[size_t(j) * num_features + i] = value;
			}
		}

		std::vector<double> eigenvectors;
		jacobi_eigen_decomposition(covariance, eigenvectors, num_features);

		std::vector<uint32> indices(num_features);
		std::iota(indices.begin(), indices.end(), 0u);
		std::sort(indices.begin(), indices.end(), [&](uint32 a, uint32 b)
			{
				return covariance[size_t(a) * num_features + a] > covariance[size_t(b) * num_features + b];
			});

		MatrixX proj(num_components, num_features);
		for (uint32 component = 0; component < num_components; ++component)
		{
			const uint32 eigen_index = indices[component];
			for (uint32 feature = 0; feature < num_features; ++feature)
			{
				proj(component, feature) = float(eigenvectors[size_t(feature) * num_features + eigen_index]);
			}
		}

		return proj;
	}

	static std::vector<float> build_column_means(const MatrixX& mat)
	{
		std::vector<float> result(mat.cols(), 0.0f);
		if (mat.rows() == 0)
		{
			return result;
		}

		for (uint32 column = 0; column < mat.cols(); ++column)
		{
			double sum = 0.0;
			for (uint32 row = 0; row < mat.rows(); ++row)
			{
				sum += mat(row, column);
			}

			result[column] = float(sum / double(mat.rows()));
		}

		return result;
	}

	void MotionMatchingDatabase::fill_normalize_factors()
	{
		normalize_factors.clear();
		normalize_factors.reserve(max_values.size());
		for (size_t index = 0; index < max_values.size(); ++index)
		{
			if (fuzzy_equals(max_values[index], min_values[index], 1e-3f))
			{
				normalize_factors.push_back(0.0f);
			}
			else
			{
				normalize_factors.push_back(weights[index] / (max_values[index] - min_values[index]));
			}
		}
	}

	SearchResult MotionMatchingDatabase::narrow_phase_search(const std::vector<std::shared_ptr<Sample>>& broadphase_candidates,
		const std::vector<float>& normalized_query,
		const SearchParams& params) const
	{
		std::vector<std::shared_ptr<Sample>> resulted_candidates = broadphase_candidates;

		if (has_flag(narrow_phase_params.flags, NarrowPhaseFlags::STATIC_THRESHOLDS_CHECK))
		{
			ASSERT(static_thresholds.size() == params.query.size());

			for (auto iter = resulted_candidates.begin(); iter != resulted_candidates.end();)
			{
				const std::vector<float>& candidates_features = iter->get()->features;

				if (candidates_features.size() != params.query.size())
				{
					ASSERT(false);
					iter = resulted_candidates.erase(iter);
					continue;
				}

				bool is_valid_by_threshold = true;
				for (uint32 i = 0; i < params.query.size(); ++i)
				{
					const StaticThreshold& threshold = static_thresholds[i];
					if (!threshold.enabled)
					{
						continue;
					}

					const float diff = params.query[i] - candidates_features[i];

					if (diff > threshold.max_diff ||
						diff < threshold.min_diff)
					{
						is_valid_by_threshold = false;
						break;
					}
				}

				if (!is_valid_by_threshold)
				{
					iter = resulted_candidates.erase(iter);
					continue;
				}

				++iter;
			}
		}

		for (auto iter = resulted_candidates.begin(); iter != resulted_candidates.end();)
		{
			const ref<animation::AnimationAssetClip>& sample_animation = animations[(*iter)->anim_index];

			if ((*iter)->anim_position + narrow_phase_params.animation_end_ignore_window >= sample_animation->get_duration())
			{
				iter = resulted_candidates.erase(iter);
				continue;
			}

			++iter;
		}

		if (resulted_candidates.empty())
		{
			return SearchResult();
		}

		if (has_flag(narrow_phase_params.flags, NarrowPhaseFlags::EUCLIDIAN_DISTANCE_CHECK))
		{
			const float max_euclidean_distance_sqr = squaref(narrow_phase_params.max_euclidean_distance);

			for (auto iter = resulted_candidates.begin(); iter != resulted_candidates.end();)
			{
				const float result_candidate_to_query_distance_sqr = square_distance(normalized_query, normalize_query(iter->get()->features));

				if (result_candidate_to_query_distance_sqr >= max_euclidean_distance_sqr)
				{
					iter = resulted_candidates.erase(iter);
					continue;
				}

				++iter;
			}
		}

		if (resulted_candidates.empty())
		{
			return SearchResult();
		}

		const std::shared_ptr<MotionMatchingDatabase::Sample>& result_candidate = resulted_candidates.front();
		const ref<animation::AnimationAssetClip>& result_animation = animations[result_candidate->anim_index];

		SearchResultType result_type = SearchResultType::NEW_ANIMATION;

		const bool is_same_animation = params.current_animation != nullptr &&
			result_animation != nullptr &&
			(params.current_animation == result_animation ||
				(params.current_animation->handle.value != 0 && params.current_animation->handle == result_animation->handle));

		if (has_flag(narrow_phase_params.flags, NarrowPhaseFlags::SAME_FRAME_CHECK) && is_same_animation)
		{
			const float anim_position_diff = result_candidate->anim_position - params.current_anim_position;

			if (abs(anim_position_diff) < narrow_phase_params.same_frame_time_threshold)
			{
				return SearchResult(params.current_animation, database_id, params.query, params.current_anim_position, SearchResultType::SAME_FRAME);
			}

			result_type = SearchResultType::SAME_ANIMATION;

			LOG_MESSAGE(std::to_string(anim_position_diff).c_str());
		}

		return SearchResult(result_animation, database_id, result_candidate->features, result_candidate->anim_position, result_type);
	}

	MotionMatchingDatabase::MotionMatchingDatabase(KnnStructureType _knn_type /*= KnnStructureType::DEFAULT*/)
	{
		knn_type = _knn_type;
		if (knn_type == KnnStructureType::HNSW)
		{
			knn_structure = ref<KnnStructure>(new HnswKnnStructure());
		}
		else
		{
			ASSERT(false);
		}
	}

	MotionMatchingDatabase::~MotionMatchingDatabase()
	{
		for (MotionMatchingFeature* feature : features)
		{
			delete feature;
		}
	}

	void MotionMatchingDatabase::generate_initial_data()
	{
		animations_asset_handles.clear();
		animations_asset_handles.reserve(animations.size());
		for (const ref<animation::AnimationAssetClip>& animation : animations)
		{
			animations_asset_handles.emplace_back(animation->handle.value);
		}

		total_features_per_sample = 0;
		for (MotionMatchingFeature* feature : features)
		{
			total_features_per_sample += feature->get_feature_size();
		}
	}

	void MotionMatchingDatabase::add_animation(ref<animation::AnimationAssetClip> animation)
	{
		animations.emplace_back(std::move(animation));
	}

	void MotionMatchingDatabase::bake(const animation::Skeleton* skeleton)
	{
		using namespace animation;

		const uint32 total_features_size = total_features_per_sample;
		ASSERT(total_features_size > 0);

		if (weights.size() != total_features_size)
		{
		    LOG_WARNING("Motion matching> Database '%s': weights size mismatch, missing weights are set to 1.", database_id.c_str());
		    weights.resize(total_features_size, 1.0f);
		}

		if (search_dimension == 0 || search_dimension > total_features_size)
		{
		    search_dimension = total_features_size;
		}

		// Sampling
		samples.clear();

		uint32 anim_index = 0;
		const float timestep = 1.0f / sample_rate;

		for (const ref<AnimationAssetClip>& animation : animations)
		{
		    const uint32 num_samples_per_animation = std::lrintf(sample_rate * animation->get_duration());

		    std::vector<ref<MotionMatchingDatabase::Sample>> animation_samples;
		    animation_samples.reserve(num_samples_per_animation);

		    for (uint32 i = 0; i < num_samples_per_animation; ++i)
		    {
		        ref<MotionMatchingDatabase::Sample> sample = make_ref<MotionMatchingDatabase::Sample>();
		        sample->anim_index = anim_index;
		        sample->anim_position = static_cast<float>(i) * timestep;

		        animation_samples.emplace_back(sample);
		    }

		    for (MotionMatchingFeature* feature : features)
		    {
		        const bool status = feature->sample_animation(skeleton, animation, sample_rate, animation_samples);
		        ASSERT(status);
		    }

		    samples.insert(samples.end(), std::make_move_iterator(animation_samples.begin()), std::make_move_iterator(animation_samples.end()));

		    ++anim_index;
		}

		ASSERT(!samples.empty());

		// Statistics
		min_values.assign(total_features_size, std::numeric_limits<float>::max());
		max_values.assign(total_features_size, std::numeric_limits<float>::lowest());

		std::vector<double> feature_sums(total_features_size, 0.0);

		for (const ref<Sample>& motion_sample : samples)
		{
		    ASSERT(motion_sample->features.size() == total_features_size);

		    for (uint32 i = 0; i < total_features_size; i++)
		    {
		        const float value = motion_sample->features[i];
		        min_values[i] = std::min(min_values[i], value);
		        max_values[i] = std::max(max_values[i], value);
		        feature_sums[i] += value;
		    }
		}

		mean_values.resize(total_features_size);
		for (uint32 i = 0; i < total_features_size; ++i)
		{
		    mean_values[i] = float(feature_sums[i] / double(samples.size()));
		}

		fill_normalize_factors();

		MatrixX features_matrix;
		build_feature_matrix(features_matrix);

		// Normalized features are already mean-centered; column means only remove floating point drift.
		transform_column_means = build_column_means(features_matrix);
		substract_column_means(features_matrix);

		transform_matrix = build_pca_projection(features_matrix, search_dimension);

		MatrixX compressed_feature_matrix;
		project_features(features_matrix, compressed_feature_matrix);

		knn_structure->build_structure_from_matrix(*this, compressed_feature_matrix);
	}

	void MotionMatchingDatabase::mark(const animation::SkeletonComponent* skeleton_component)
	{
		MotionMatchingAnimMarker::mark_database(skeleton_component, *this);
	}

	SearchResult MotionMatchingDatabase::search(const SearchParams& params) const
	{
		if (params.query.size() != total_features_per_sample)
		{
			LOG_ERROR("Motion matching> Database '%s': query size %u does not match feature size %u.",
				database_id.c_str(), uint32(params.query.size()), total_features_per_sample);
			return SearchResult();
		}

		std::vector<float> normalized_query_values = normalize_query(params.query);

		VectorX packed_query = pack_query(normalized_query_values);
		ASSERT(packed_query.size() == search_dimension);

		std::vector<std::shared_ptr<Sample>> knn = knn_structure->search_knn(packed_query.data(), packed_query.size(), max_broardphase_candidates, *this);

		return narrow_phase_search(knn, normalized_query_values, params);
	}

	std::vector<float> MotionMatchingDatabase::normalize_query(const std::vector<float>& query) const
	{
		ASSERT(query.size() == mean_values.size());
		ASSERT(query.size() == normalize_factors.size());

		std::vector<float> result;
		result.reserve(query.size());
		for (size_t index = 0; index < query.size(); ++index)
		{
			if (fuzzy_equals(query[index], mean_values[index], 1e-3f))
			{
				result.push_back(0.0f);
			}
			else
			{
				result.push_back((query[index] - mean_values[index]) * normalize_factors[index]);
			}
		}

		return result;
	}

	void MotionMatchingDatabase::build_feature_matrix(MatrixX& out) const
	{
		// Samples are normalized exactly the same way as runtime queries.
		out.resize(uint32(samples.size()), total_features_per_sample);
		for (uint32 row = 0; row < out.rows(); ++row)
		{
			const std::vector<float> normalized_features = normalize_query(samples[row]->features);
			ASSERT(normalized_features.size() == out.cols());

			std::copy(normalized_features.begin(), normalized_features.end(), out.row(row));
		}
	}

	void MotionMatchingDatabase::substract_column_means(MatrixX& matrix) const
	{
		const uint32 rows = matrix.rows();
		const uint32 columns = matrix.cols();

		ASSERT(transform_column_means.size() == columns);

		for (uint32 i = 0; i < rows; ++i)
		{
			for (uint32 j = 0; j < columns; ++j)
			{
				matrix(i, j) = matrix(i, j) - transform_column_means[j];
			}
		}
	}

	void MotionMatchingDatabase::project_features(const MatrixX& features, MatrixX& out) const
	{
		multiply_transposed(features, transform_matrix, out);
	}

	VectorX MotionMatchingDatabase::pack_query(const std::vector<float>& query) const
	{
		ASSERT(query.size() == transform_column_means.size());

		VectorX centered(uint32(query.size()));
		for (uint32 i = 0; i < centered.size(); ++i)
		{
			centered[i] = query[i] - transform_column_means[i];
		}

		VectorX projected;
		multiply(transform_matrix, centered, projected);
		return projected;
	}

	std::string MotionMatchingDatabase::get_asset_type_impl()
	{
		return std::string(".emmdb");
	}

	bool MotionMatchingDatabase::serialize(std::ostream& os) const
	{
		try
		{
			// Header
			{
				IO::write_value(os, MOTION_DATABASE_MAGIC);
				IO::write_value(os, MOTION_DATABASE_VERSION);
			}

			// Base
			{
				IO::write_value(os, BinarySerializer::serialize(database_id));
				IO::write_value(os, total_features_per_sample);
				IO::write_value(os, search_dimension);
				IO::write_value(os, sample_rate);
				IO::write_value(os, max_broardphase_candidates);
			}

			// Weights
			{
				IO::write_vector(os, normalize_factors);
				IO::write_vector(os, mean_values);
				IO::write_vector(os, min_values);
				IO::write_vector(os, max_values);
				IO::write_vector(os, weights);
			}

			// Animations
			{
				IO::write_vector(os, animations_asset_handles);
			}

			// Features
			{
				std::vector<std::string> feature_serialized_names;
				feature_serialized_names.reserve(features.size());
				for (const MotionMatchingFeature* feature : features)
				{
					feature_serialized_names.emplace_back(feature->get_type().get_name().to_string());
				}
				IO::write_value(os, BinarySerializer::serialize(feature_serialized_names));

				for (const MotionMatchingFeature* feature : features)
				{
					const std::vector<ref<FeatureDesc>>& descriptors = feature->get_descriptors();
					IO::write_value(os, static_cast<uint32>(descriptors.size()));

					for (const ref<FeatureDesc>& desc : descriptors)
					{
						IO::write_value(os, BinarySerializer::serialize(desc->get_type().get_name().to_string()));

						FeatureDesc* raw_desc = desc.get();
						BinaryDataArchive desc_archive = BinarySerializer::serialize(*raw_desc);

						IO::write_value(os, desc_archive);
					}
				}
			}

			// Narrow phase
			{
				IO::write_value(os, narrow_phase_params.flags);
				IO::write_value(os, narrow_phase_params.same_frame_time_threshold);
				IO::write_value(os, narrow_phase_params.max_euclidean_distance);
				IO::write_value(os, narrow_phase_params.animation_end_ignore_window);

				IO::write_value(os, BinarySerializer::serialize(static_thresholds));

				IO::write_value(os, BinarySerializer::serialize(samples));
			}

			// Transform
			{
				IO::write_vector(os, transform_column_means);

				const uint32 rows = transform_matrix.rows();
				const uint32 cols = transform_matrix.cols();
				IO::write_value(os, rows);
				IO::write_value(os, cols);
				if (rows * cols > 0)
				{
					// IO::write_data takes the size in bytes.
					IO::write_data(os, transform_matrix.data(), std::streamsize(sizeof(float)) * rows * cols);
				}
			}

			// Knn
			{
				IO::write_value(os, knn_type);

				if (knn_structure->writable.empty())
				{
					knn_structure->rebuild_structure(*this);
				}

				IO::write_value(os, static_cast<uint32>(knn_structure->writable.size()));
				IO::write_data(os, knn_structure->writable.data(), static_cast<std::streamsize>(knn_structure->writable.size()));

				knn_structure->serialize(os, *this);
			}
		}
		catch (...)
		{
			LOG_ERROR("Exception thrown while serializing asset!");
			return false;
		}

		return os.good();
	}

	bool MotionMatchingDatabase::deserialize(std::istream& is)
	{
		try
		{
			// Header
			{
				uint32 magic = 0;
				uint32 version = 0;
				IO::read_value(is, magic);
				IO::read_value(is, version);

				if (magic != MOTION_DATABASE_MAGIC || version != MOTION_DATABASE_VERSION)
				{
					LOG_ERROR("Motion matching> Unsupported motion matching database format (version %u, expected %u). Re-bake the database.",
						magic == MOTION_DATABASE_MAGIC ? version : 0u, MOTION_DATABASE_VERSION);
					return false;
				}
			}

			// Base
			{
				BinaryDataArchive binary_name;
				IO::read_value(is, binary_name);

				BinarySerializer::deserialize(binary_name.raw_data(), binary_name.size(), database_id);

				IO::read_value(is, total_features_per_sample);
				IO::read_value(is, search_dimension);
				IO::read_value(is, sample_rate);
				IO::read_value(is, max_broardphase_candidates);
			}

			// Weights
			{
				IO::read_vector(is, normalize_factors);
				IO::read_vector(is, mean_values);
				IO::read_vector(is, min_values);
				IO::read_vector(is, max_values);
				IO::read_vector(is, weights);
			}

			// Animations
			{
				IO::read_vector(is, animations_asset_handles);

				GameAssetsProvider provider;

				animations.clear();
				animations.reserve(animations_asset_handles.size());
				for (const uint64 handle : animations_asset_handles)
				{
					const fs::path animation_path = get_path_from_asset_handle(AssetHandle(handle));
					ref<animation::AnimationAssetClip> anim_clip = provider.load_game_asset_from_file<animation::AnimationAssetClip>(animation_path);
					if (anim_clip == nullptr)
					{
						LOG_ERROR("Motion matching> Failed to load animation '%s' referenced by the database.", animation_path.string().c_str());
						return false;
					}
					animations.emplace_back(anim_clip);
				}
			}

			// Features
			{
				BinaryDataArchive binary_feature_names;
				IO::read_value(is, binary_feature_names);

				std::vector<std::string> feature_serialized_names;
				BinarySerializer::deserialize(binary_feature_names.raw_data(), binary_feature_names.size(), feature_serialized_names);

				for (MotionMatchingFeature* feature : features)
				{
					delete feature;
				}
				features.clear();

				for (const std::string& param_name : feature_serialized_names)
				{
					rttr::type feature_type = rttr::type::get_by_name(param_name);
					if (!feature_type.is_valid())
					{
						LOG_ERROR("Motion matching> Unknown feature type '%s'.", param_name.c_str());
						return false;
					}
					features.emplace_back(feature_type.create().get_value<MotionMatchingFeature*>());
				}

				for (MotionMatchingFeature* feature : features)
				{
					uint32 descriptors_size = 0;
					IO::read_value(is, descriptors_size);

					feature->descriptors.reserve(descriptors_size);

					for (uint32 i = 0; i < descriptors_size; ++i)
					{
						BinaryDataArchive binary_desc_typename;
						IO::read_value(is, binary_desc_typename);

						std::string desc_typename;
						BinarySerializer::deserialize(binary_desc_typename.raw_data(), binary_desc_typename.size(), desc_typename);

						rttr::type desc_type = rttr::type::get_by_name(desc_typename);
						if (!desc_type.is_valid())
						{
							LOG_ERROR("Motion matching> Unknown feature descriptor type '%s'.", desc_typename.c_str());
							return false;
						}
						FeatureDesc* raw_desc = desc_type.create().get_value<FeatureDesc*>();

						BinaryDataArchive binary_desc;
						IO::read_value(is, binary_desc);
						BinarySerializer::deserialize(binary_desc.raw_data(), binary_desc.size(), *raw_desc);

						ref<FeatureDesc> descriptor = ref<FeatureDesc>(raw_desc);

						feature->descriptors.emplace_back(descriptor);
					}
				}
			}

			// Narrow phase
			{
				IO::read_value(is, narrow_phase_params.flags);
				IO::read_value(is, narrow_phase_params.same_frame_time_threshold);
				IO::read_value(is, narrow_phase_params.max_euclidean_distance);
				IO::read_value(is, narrow_phase_params.animation_end_ignore_window);

				BinaryDataArchive binary_static_thresholds;
				IO::read_value(is, binary_static_thresholds);

				BinarySerializer::deserialize(binary_static_thresholds.raw_data(), binary_static_thresholds.size(), static_thresholds);

				BinaryDataArchive binary_samples_thresholds;
				IO::read_value(is, binary_samples_thresholds);

				BinarySerializer::deserialize(binary_samples_thresholds.raw_data(), binary_samples_thresholds.size(), samples);
			}

			// Transform
			{
				IO::read_vector(is, transform_column_means);

				uint32 rows = 0;
				uint32 cols = 0;
				IO::read_value(is, rows);
				IO::read_value(is, cols);

				transform_matrix.resize(rows, cols);
				if (rows * cols > 0)
				{
					IO::read_data(is, transform_matrix.data(), std::streamsize(sizeof(float)) * rows * cols);
				}
			}

			// Wait for loading
			for (const auto& animation : animations)
			{
				animation->load_job.wait_for_completion();
			}

			// Knn
			{
				IO::read_value(is, knn_type);

				uint32 knn_size = 0;
				IO::read_value(is, knn_size);

				knn_structure->writable.resize(knn_size);
				if (knn_size > 0)
				{
					IO::read_data(is, knn_structure->writable.data(), static_cast<std::streamsize>(knn_size));
				}

				knn_structure->deserialize(is, *this);
			}
		}
		catch (...)
		{
			LOG_ERROR("Exception thrown while deserializing asset!");
			return false;
		}

		return !is.fail();
	}

	static std::unordered_map<std::string, ref<MotionMatchingDatabase>> databases_in_registry;
	static std::mutex database_registry_mtx;

	void MotionDatabaseRegistry::add_to_registry(ref<MotionMatchingDatabase> database)
	{
		std::lock_guard<std::mutex> lock{ database_registry_mtx };
		databases_in_registry.emplace(database->database_id, database);
	}

	ref<MotionMatchingDatabase> MotionDatabaseRegistry::get_by_id(const std::string& database_id)
	{
		std::lock_guard<std::mutex> lock{ database_registry_mtx };
		auto database_iter = databases_in_registry.find(database_id);
		if (database_iter == databases_in_registry.end())
		{
			return nullptr;
		}
		return database_iter->second;
	}
}
