#pragma once

#include "physics_api.h"

#include "physics/physx_api.h"

#include <core/math.h>
#include <ecs/entity.h>

namespace era_engine
{
    class World;
}

namespace era_engine::physics
{
    class ShapeComponent;

    enum class SceneQueryFilterFlag
    {
        NONE = 0,
        DYNAMICS = (1 << 0),
        STATICS = (1 << 1),
        PREFILTER = (1 << 2),
        POSTFILTER = (1 << 3)
    };
    DEFINE_BITWISE_OPERATORS_FOR_ENUM(SceneQueryFilterFlag)

    enum class SceneQueryHitType : uint8
    {
        NONE = 0,
        TOUCH,
        BLOCK
    };

    struct ERA_PHYSICS_API SceneQueryHit
    {
        SceneQueryHitType type = SceneQueryHitType::NONE;
        ShapeComponent* shape_component = nullptr;
    };

    struct ERA_PHYSICS_API SceneQueryPositionedHit final : public SceneQueryHit
    {
        vec3 position = vec3::zero;
        vec3 normal = vec3::zero;

        float distance = 0.0f;
    };

    enum class SceneQueryType : uint8
    {
        RAYCAST = 0,
        SWEEP,
        OVERLAP
    };

    enum class SceneQueryMode : uint8
    {
        ANY = 0,
        CLOSEST,
        ALL,
    };


    class ERA_PHYSICS_API SceneQueryFilterCallback
    {
    public:
        virtual ~SceneQueryFilterCallback() = default;

        virtual SceneQueryHitType prefilter(const ShapeComponent* component)
        {
            return SceneQueryHitType::NONE;
        }

        virtual SceneQueryHitType postfilter(const SceneQueryHit& hit)
        {
            return SceneQueryHitType::NONE;
        }
    };

    class ERA_PHYSICS_API SceneQueryBaseParams
    {
    public:
        SceneQueryFilterFlag filter_flags = SceneQueryFilterFlag::STATICS | SceneQueryFilterFlag::DYNAMICS;
        SceneQueryFilterCallback* filter_callback = nullptr;
    };

    class ERA_PHYSICS_API SceneQueryDistancedBaseParams : public SceneQueryBaseParams
    {
    public:
        float distance = 0.0f;
        bool use_single_sided_mesh_backface_hits = false;
    };

    class ERA_PHYSICS_API SceneQueryGeometry final
    {
    public:
        SceneQueryGeometry();

        enum class Type : uint8
        {
            SPHERE = 0,
            CAPSULE,
            BOX,
        };

        Type type;

        union
        {
            float sphere_radius;

            struct
            {
                float capsule_half_height;
                float capsule_radius;
            };

            vec3 box_half_extents;
        };
    };

    class ERA_PHYSICS_API RaycastQuery final
    {
        RaycastQuery() = delete;

    public:

        class ERA_PHYSICS_API Params final : public SceneQueryDistancedBaseParams
        {
        public:
            ray query_ray;
        };

        static bool any(World* world, const Params& params, SceneQueryPositionedHit* out_hit);

        static bool closest(World* world, const Params& params, SceneQueryPositionedHit* out_hit);

        static uint32 all(World* world, const Params& params, SceneQueryPositionedHit* out_hit_buffer, uint32 out_hit_buffer_size);
    };

    class ERA_PHYSICS_API SweepQuery final
    {
        SweepQuery() = delete;

    public:

        class ERA_PHYSICS_API Params final : public SceneQueryDistancedBaseParams
        {
        public:
            SceneQueryGeometry geometry;
            trs geometry_transform;
            vec3 direction;
        };

        static bool any(World* world, const Params& params, SceneQueryPositionedHit* out_hit);

        static bool closest(World* world, const Params& params, SceneQueryPositionedHit* out_hit);

        static uint32 all(World* world, const Params& params, SceneQueryPositionedHit* out_hit_buffer, uint32 out_hit_buffer_size);
    };

    class ERA_PHYSICS_API OverlapQuery final
    {
        OverlapQuery() = delete;

    public:

        class ERA_PHYSICS_API Params final : public SceneQueryDistancedBaseParams
        {
        public:
            SceneQueryGeometry geometry;
            trs geometry_transform;
        };

        static bool any(World* world, const Params& params, SceneQueryPositionedHit* out_hit);

        static bool closest(const ShapeComponent* shape, const SceneQueryGeometry& geom, const trs& geom_pose);

        static uint32 all(World* world, const Params& params, SceneQueryPositionedHit* out_hit_buffer, uint32 out_hit_buffer_size);
    };

    class ERA_PHYSICS_API SceneQueryFilter : public SceneQueryFilterCallback
    {
    public:
        SceneQueryFilter() = default;
        SceneQueryFilter(uint32 _collision_mask, uint32 _ignore_mask);
        SceneQueryFilter(uint32 _collision_mask, uint32 _ignore_mask, EntityPtr _ignore_entity);
        SceneQueryFilter(uint32 _collision_mask, uint32 _ignore_mask, const std::vector<EntityPtr>& _ignore_list);

        SceneQueryHitType prefilter(const ShapeComponent* component) override;

        static SceneQueryFilter construct_from_type(World* world, uint32 collision_type);

    public:
        uint32 collision_mask = 0xFFFFFFFF;
        uint32 ignore_mask = 0;
        std::vector<EntityPtr> ignore_list;
    };

}