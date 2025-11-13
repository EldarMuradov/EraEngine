#include "animation/animation_clip.h"
#include "animation/animation.h"

#include "asset/io.h"

#include "core/io/binary_data.h"
#include "core/serialization/binary_serializer.h"
#include "core/log.h"

#include <acl/io/clip_reader.h>
#include <acl/io/clip_writer.h>

#include <rtm/math.h>
#include <rtm/quatf.h>
#include <rtm/scalarf.h>
#include <rtm/vector4f.h>

#include <sjson/writer.h>

#include <sstream>

namespace era_engine::animation
{
    static float scalar_to_float(const rtm::scalarf& v)
    {
        return rtm::scalar_cast(v);
    }

    static rtm::scalarf float_to_scalar(float v)
    {
        return rtm::scalar_set(v);
    }

    static rtm::quatf quat_to_quatf(const quat& q)
    {
        return rtm::quat_load(q.v4.data);
    }

    static quat quatf_to_quat(const rtm::quatf& q)
    {
        quat result;
        rtm::quat_store(q, result.v4.data);

        return result;
    }

    static vec4 vector4f_to_vec4(const rtm::vector4f& v)
    {
        vec4 result;
        rtm::vector_store(v, result.data);

        return result;
    }

    static rtm::vector4f vec4_to_vector4f(const vec4& v)
    {
        return rtm::vector_load(v.data);
    }

    static vec3 vector4f_to_vec3(const rtm::vector4f& v)
    {
        vec3 result;
        rtm::vector_store3(v, result.data);

        return result;
    }

    static rtm::vector4f vec3_to_vector4f(const vec3& v)
    {
        return rtm::vector_load3(v.data);
    }

    static trs qvvf_to_trs(const rtm::qvvf& t)
    {
        return trs(vector4f_to_vec3(t.translation),
            quatf_to_quat(t.rotation),
            vector4f_to_vec3(t.scale));
    }

    static rtm::qvvf trs_to_qvvf(const trs& t)
    {
        return { quat_to_quatf(t.rotation), vec3_to_vector4f(t.position), vec3_to_vector4f(t.scale) };
    }

    class StringStream : public sjson::StreamWriter
    {
    public:
        void write(const void* buffer, size_t buffer_size) override
        {
            ss.write(reinterpret_cast<const char*>(buffer), buffer_size);
        }

        std::stringstream ss;
    };

    namespace acl_animation_clip_details
    {
        class JointTransformSamplingWriter : public acl::track_writer
        {
        public:
            JointTransformSamplingWriter(const JointTransform& _bind_transform, JointTransform& _joint_transform)
                : bind_transform(_bind_transform)
                , joint_transform(_joint_transform)
            {
            }

            static constexpr acl::default_sub_track_mode get_default_rotation_mode()
            {
                return acl::default_sub_track_mode::constant;
            }

            static constexpr acl::default_sub_track_mode get_default_translation_mode()
            {
                return acl::default_sub_track_mode::constant;
            }

            static constexpr acl::default_sub_track_mode get_default_scale_mode()
            {
                return acl::default_sub_track_mode::constant;
            }

            rtm::quatf RTM_SIMD_CALL get_constant_default_rotation() const
            {
                return quat_to_quatf(bind_transform.get_rotation());
            }

            rtm::vector4f RTM_SIMD_CALL get_constant_default_translation() const
            {
                return vec3_to_vector4f(bind_transform.get_translation());
            }

            rtm::vector4f RTM_SIMD_CALL get_constant_default_scale() const
            {
                return vec3_to_vector4f(vec3(bind_transform.get_scale()));
            }

            void RTM_SIMD_CALL write_rotation(uint32_t track_index, rtm::quatf_arg0 rotation)
            {
                joint_transform.set_rotation(quatf_to_quat(rotation));
            }

            // Called by the decoder to write out a translation value for a specified bone index.
            void RTM_SIMD_CALL write_translation(uint32_t track_index, rtm::vector4f_arg0 translation)
            {
                joint_transform.set_translation(vector4f_to_vec3(translation));
            }

            // Called by the decoder to write out a scale value for a specified bone index.
            void RTM_SIMD_CALL write_scale(uint32_t track_index, rtm::vector4f_arg0 scale)
            {
                joint_transform.set_scale(vector4f_to_vec3(scale));
            }

        private:
            const JointTransform& bind_transform;
            JointTransform& joint_transform;
        };

        class PoseSamplingWriter : public acl::track_writer
        {
        public:
            PoseSamplingWriter(const SkeletonPose& _bind_pose, SkeletonPose& _pose, const std::vector<uint32>& _mapping, const std::vector<uint32>& _mapping2 = empty_mapping)
                : acl::track_writer()
                , bind_pose(_bind_pose)
                , pose(_pose)
                , mapping(_mapping)
                , support_mapping(_mapping2)
            {
            }

            static const std::vector<uint32> empty_mapping;

            static constexpr acl::default_sub_track_mode get_default_rotation_mode()
            {
                return acl::default_sub_track_mode::variable;
            }

            static constexpr acl::default_sub_track_mode get_default_translation_mode()
            {
                return acl::default_sub_track_mode::variable;
            }

            static constexpr acl::default_sub_track_mode get_default_scale_mode()
            {
                return acl::default_sub_track_mode::variable;
            }

            rtm::quatf RTM_SIMD_CALL get_variable_default_rotation(uint32_t track_index) const
            {
                uint32 joint_index = map_track_to_joint(track_index);
                if (joint_index == INVALID_JOINT)
                {
                    return rtm::quatf();
                }

                ASSERT(joint_index < bind_pose.size());
                return quat_to_quatf(bind_pose.get_joint_transform(joint_index).get_rotation());
            }

            rtm::vector4f RTM_SIMD_CALL get_variable_default_translation(uint32_t track_index) const
            {
                uint32 joint_index = map_track_to_joint(track_index);
                if (joint_index == INVALID_JOINT)
                {
                    return rtm::vector4f();
                }

                ASSERT(joint_index < bind_pose.size());
                return vec3_to_vector4f(bind_pose.get_joint_transform(joint_index).get_translation());
            }

            rtm::vector4f RTM_SIMD_CALL get_variable_default_scale(uint32_t track_index) const
            {
                uint32 joint_index = map_track_to_joint(track_index);
                if (joint_index == INVALID_JOINT)
                {
                    return rtm::vector4f();
                }

                ASSERT(joint_index < bind_pose.size());
                return vec3_to_vector4f(vec3(bind_pose.get_joint_transform(joint_index).get_scale()));
            }

            void RTM_SIMD_CALL write_rotation(uint32_t track_index, rtm::quatf_arg0 rotation)
            {
                uint32 joint_index = map_track_to_joint(track_index);
                if (joint_index == INVALID_JOINT)
                {
                    return;
                }

                ASSERT(joint_index < pose.size());
                pose.set_joint_rotation(quatf_to_quat(rotation), joint_index);
            }

            // Called by the decoder to write out a translation value for a specified bone index.
            void RTM_SIMD_CALL write_translation(uint32_t track_index, rtm::vector4f_arg0 translation)
            {
                uint32 joint_index = map_track_to_joint(track_index);
                if (joint_index == INVALID_JOINT)
                {
                    return;
                }

                ASSERT(joint_index < pose.size());
                pose.set_joint_translation(vector4f_to_vec3(translation), joint_index);
            }

            // Called by the decoder to write out a scale value for a specified bone index.
            void RTM_SIMD_CALL write_scale(uint32_t track_index, rtm::vector4f_arg0 scale)
            {
                uint32 joint_index = map_track_to_joint(track_index);
                if (joint_index == INVALID_JOINT)
                {
                    return;
                }

                ASSERT(joint_index < pose.size());
                pose.set_joint_scale(vector4f_to_vec3(scale), joint_index);
            }

        private:
            uint32 map_track_to_joint(uint32 track_index) const
            {
                auto remaped_track_index = track_index;
                if (remaped_track_index < support_mapping.size())
                {
                    remaped_track_index = support_mapping[remaped_track_index];
                }

                if (remaped_track_index < mapping.size())
                {
                    return mapping[remaped_track_index];
                }

                return remaped_track_index;
            }

            const SkeletonPose& bind_pose;

            SkeletonPose& pose;

            const std::vector<uint32>& mapping;
            const std::vector<uint32>& support_mapping;
        };

        const std::vector<uint32> PoseSamplingWriter::empty_mapping;

        class CurveSamplingWriter : public acl::track_writer
        {
        public:
            CurveSamplingWriter(float& out_value)
                : value(out_value)
            {
            }

            void RTM_SIMD_CALL write_float1(uint32_t track_index, rtm::scalarf_arg0 _value)
            {
                value = scalar_to_float(_value);
            }

        private:
            float& value;
        };

        class CurvesSamplingWriter : public acl::track_writer
        {
        public:
            CurvesSamplingWriter(std::vector<float>& out_values)
                : values(out_values)
            {
            }

            void RTM_SIMD_CALL write_float1(uint32_t track_index, rtm::scalarf_arg0 value)
            {
                values[track_index] = scalar_to_float(value);
            }

        private:
            std::vector<float>& values;
        };
    }

	AnimationAssetClip::AnimationAssetClip()
	{
        allocator = std::make_unique<AnimationAllocator>();
        joints = acl::track_array_qvvf();
        curves = acl::track_array_float1f();
	}

	AnimationAssetClip::AnimationAssetClip(AnimationAssetClip&& other) noexcept
        : joint_name_mapping(std::move(other.joint_name_mapping))
        , curve_name_mapping(std::move(other.curve_name_mapping))
        , allocator(std::move(other.allocator))
	{
        joints = std::move(other.joints);
        curves = std::move(other.curves);
	}

	AnimationAssetClip& AnimationAssetClip::operator=(AnimationAssetClip&& other) noexcept
	{
        if (this != &other)
        {
            joints = std::move(other.joints);
            curves = std::move(other.curves);
            joint_name_mapping = std::move(other.joint_name_mapping);
            curve_name_mapping = std::move(other.curve_name_mapping);
            allocator = std::move(other.allocator);
        }
        return *this;
    }

	AnimationAssetClip::~AnimationAssetClip()
	{
        joints = acl::track_array_qvvf();
        curves = acl::track_array_float1f();
	}

	bool AnimationAssetClip::is_valid() const
	{
		return true;
	}

	float AnimationAssetClip::get_duration() const
	{
		return joints.get_finite_duration();
	}

	bool AnimationAssetClip::sample_joint(float sample_time, std::string_view joint_name, const JointTransform& bind_transform, JointTransform& out_transform) const
	{
        auto iter = get_joint_mapping().find(joint_name.data());
        if (iter == get_joint_mapping().end())
        {
            return false;
        }

        return sample_joint(sample_time, iter->second, bind_transform, out_transform);
    }

	bool AnimationAssetClip::sample_joint(float sample_time, uint32 joint_index, const JointTransform& bind_transform, JointTransform& out_transform) const
	{
        ASSERT(joint_index < joints.get_num_tracks());
        acl_animation_clip_details::JointTransformSamplingWriter sampling_writer(bind_transform, out_transform);
        joints.sample_track(joint_index, sample_time, acl::sample_rounding_policy::none, sampling_writer);

        return true;
    }

	bool AnimationAssetClip::sample_joints(float sample_time, SkeletonPose& out_pose) const
	{
        std::vector<uint32> dummy;
        acl_animation_clip_details::PoseSamplingWriter sampling_writer(SkeletonPose(out_pose), out_pose, dummy);
        joints.sample_tracks(sample_time, acl::sample_rounding_policy::none, sampling_writer);

        return true;
    }

    bool AnimationAssetClip::sample_joints(float sample_time, const SkeletonPose& default_pose, SkeletonPose& out_pose, const std::vector<uint32>& custom_joint_name_mapping) const
    {
        acl_animation_clip_details::PoseSamplingWriter sampling_writer(default_pose, out_pose, custom_joint_name_mapping);
        joints.sample_tracks(sample_time, acl::sample_rounding_policy::none, sampling_writer);

        return true;
    }

	float AnimationAssetClip::get_sample_rate() const
	{
		return joints.get_sample_rate();
	}

	uint32 AnimationAssetClip::get_num_samples_per_track() const
	{
		return joints.get_num_samples_per_track();
	}

	bool AnimationAssetClip::sample_curve(float sample_time, std::string_view curve_name, float& out_curve_value) const
	{
        auto iter = get_curve_mapping().find(curve_name.data());
        if (iter == get_curve_mapping().end())
        {
            return false;
        }

        sample_curve(sample_time, iter->second, out_curve_value);

        return true;
    }

	void AnimationAssetClip::sample_curve(float sample_time, uint32 curve_index, float& out_curve_value) const
	{
        ASSERT(curve_index < curves.get_num_tracks());
        acl_animation_clip_details::CurveSamplingWriter sampling_writer(out_curve_value);
        curves.sample_track(curve_index, sample_time, acl::sample_rounding_policy::none, sampling_writer);
	}

	void AnimationAssetClip::sample_curves(float sample_time, std::vector<float>& out_curve_values) const
	{
        ASSERT(out_curve_values.size() == curves.get_num_tracks());
        acl_animation_clip_details::CurvesSamplingWriter sampling_writer(out_curve_values);
        curves.sample_tracks(sample_time, acl::sample_rounding_policy::none, sampling_writer);
	}

	void AnimationAssetClip::add_curve(const std::string& name, const std::vector<float>& values)
	{
        if (get_curve_mapping().count(name) == 0)
        {
            std::unordered_map<std::string, uint32> curve_name_mapping_copy = get_curve_mapping();

            uint32 curve_index = static_cast<uint32>(curve_name_mapping_copy.size());
            curve_name_mapping_copy[name] = curve_index;

            curve_name_mapping = curve_name_mapping_copy;

            acl::track_desc_scalarf track_description;
            track_description.output_index = curve_index;
            track_description.precision = 0.001f;

            uint32 num_to_allocate = get_num_samples_per_track();
            acl::track_float1f float1_track = acl::track_float1f::make_reserve(track_description, *this->allocator, num_to_allocate, get_sample_rate());
            float1_track.set_name(acl::string(*this->allocator, name.c_str(), name.size()));
            for (uint32 sample_index = 0; sample_index < uint32(values.size()); ++sample_index)
            {
                float1_track[sample_index] = values[sample_index];
            }

            acl::track_array_float1f new_curves(*this->allocator, curves.get_num_tracks() + 1);
            for (uint32 index = 0; index < curves.get_num_tracks(); ++index)
            {
                new_curves[index] = std::move(curves[index]);
            }
            new_curves[curve_index] = std::move(float1_track);

            curves = std::move(new_curves);
        }
        else
        {
            uint32 curve_index = get_curve_mapping().find(name)->second;

            acl::track_desc_scalarf track_description;
            track_description.output_index = curve_index;
            track_description.precision = 0.001f;

            uint32 num_to_allocate = get_num_samples_per_track();
            acl::track_float1f float1_track = acl::track_float1f::make_reserve(track_description, *this->allocator, num_to_allocate, get_sample_rate());
            float1_track.set_name(acl::string(*this->allocator, name.c_str(), name.size()));
            for (uint32 sample_index = 0; sample_index < uint32(values.size()); ++sample_index)
            {
                float1_track[sample_index] = values[sample_index];
            }

            curves[curve_index] = std::move(float1_track);
        }
	}

	void AnimationAssetClip::remove_curve(const std::string& name)
	{
        auto it = get_curve_mapping().find(name);
        if (it != get_curve_mapping().end())
        {
            uint32 curve_index = it->second;

            acl::track_array_float1f new_curves(*this->allocator, curves.get_num_tracks() - 1);

            std::unordered_map<std::string, uint32> new_curve_mapping;

            uint32 new_index = 0;
            for (uint32 index = 0; index < curves.get_num_tracks(); ++index)
            {
                if (index != curve_index)
                {
                    new_curves[new_index] = std::move(curves[index]);
                    acl::track_desc_scalarf& descr = new_curves[new_index].get_description();
                    descr.output_index = new_index;
                    new_curve_mapping[std::string(new_curves[new_index].get_name().c_str())] = new_index;
                    ++new_index;
                }
            }

            curve_name_mapping = std::move(new_curve_mapping);
            curves = std::move(new_curves);
        }
	}

	const std::unordered_map<std::string, uint32>& AnimationAssetClip::get_joint_mapping() const
	{
        return joint_name_mapping;
	}

	const std::unordered_map<std::string, uint32>& AnimationAssetClip::get_curve_mapping() const
	{
        return curve_name_mapping;
	}

    std::string AnimationAssetClip::get_asset_type_impl()
    {
        return std::string(".eanm");
    }

    struct AnimationSerializationData
    {
        std::unordered_map<std::string, uint32> joint_name_mapping;
        std::unordered_map<std::string, uint32> curve_name_mapping;

        BinaryData tracks;
        BinaryData curves;

        ERA_BINARY_SERIALIZE(joint_name_mapping, curve_name_mapping, tracks, curves)
    };

    bool AnimationAssetClip::serialize(std::ostream& os) const
    {
        AnimationSerializationData data;

        BinaryDataArchive tracks_data;
        BinaryDataArchive curves_data;

        {
            StringStream tracks_stream;
            sjson::Writer writer(tracks_stream);
            acl::acl_impl::write_sjson_header(joints, writer);
            acl::acl_impl::write_sjson_tracks(joints, writer);

            const std::string& content = tracks_stream.ss.str();

            tracks_data.fill(reinterpret_cast<const uint8*>(content.data()), content.size());
        }

        if (!curves.is_empty())
        {
            StringStream curve_stream;
            sjson::Writer writer(curve_stream);
            acl::acl_impl::write_sjson_header(curves, writer);
            acl::acl_impl::write_sjson_tracks(curves, writer);

            const std::string& content = curve_stream.ss.str();

            curves_data.fill(reinterpret_cast<const uint8*>(content.data()), content.size());
        }

        const std::unordered_map<std::string, uint32>& clip_curve_name_mapping = get_curve_mapping();
        const std::unordered_map<std::string, uint32>& clip_joint_name_mapping = get_joint_mapping();

        data.curve_name_mapping.insert(std::begin(clip_curve_name_mapping), std::end(clip_curve_name_mapping));
        data.joint_name_mapping.insert(std::begin(clip_joint_name_mapping), std::end(clip_joint_name_mapping));

        data.tracks = BinaryData(tracks_data);
        data.curves = BinaryData(curves_data);

        const BinaryDataArchive& serialized_data = BinarySerializer::serialize(data);

        try
        {
            if (!IO::write_value(os, serialized_data))
            {
                return false;
            }
        }
        catch (...)
        {
            LOG_ERROR("Exception thrown while serializing asset!");
            return false;
        }

        return true;
    }

    bool AnimationAssetClip::deserialize(std::istream& is)
    {
		BinaryDataArchive serialized_data;

		if (!IO::read_value(is, serialized_data))
		{
			return false;
		}

		AnimationSerializationData data{};
        if (BinarySerializer::deserialize(BinaryData(serialized_data), data) != serialized_data.size())
        {
            return false;
        }

        joint_name_mapping = std::move(data.joint_name_mapping);
        curve_name_mapping = std::move(data.curve_name_mapping);

        {
            acl::clip_reader tracks_reader(*allocator,
                reinterpret_cast<const char*>(data.tracks.data()),
                data.tracks.size());

            acl::sjson_raw_track_list raw_clip_data;
            tracks_reader.read_raw_track_list(raw_clip_data);
            joints = std::move(*acl::track_array_cast<acl::track_array_qvvf>(&raw_clip_data.track_list));
        }

        if (data.curves.size() > 0)
        {
            acl::clip_reader curves_reader(*allocator,
                reinterpret_cast<const char*>(data.curves.data()),
                data.curves.size());
            acl::sjson_raw_track_list raw_curves_data;
            curves_reader.read_raw_track_list(raw_curves_data);
            curves = std::move(*acl::track_array_cast<acl::track_array_float1f>(&raw_curves_data.track_list));
        }

        return true;
    }
}