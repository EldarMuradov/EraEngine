#pragma once

#include "physics_api.h"

#include "physics/physx_api.h"
#include "physics/snippets_utils/enginedrivetrain/EngineDrivetrain.h"
#include "physics/vehicles/vehicle_config.h"
#include "physics/vehicles/vehicle_controller.h"

#include <string>

namespace era_engine::physics
{
    class VehicleRoadQueryFilter final : public physx::PxQueryFilterCallback
    {
    public:
        physx::PxQueryHitType::Enum preFilter(const physx::PxFilterData& filter_data,
            const physx::PxShape* shape,
            const physx::PxRigidActor* actor,
            physx::PxHitFlags& query_flags) override;

        physx::PxQueryHitType::Enum postFilter(const physx::PxFilterData& filter_data,
            const physx::PxQueryHit& hit,
            const physx::PxShape* shape,
            const physx::PxRigidActor* actor) override;

        const physx::PxRigidActor* self = nullptr;

        uint32 collision_mask = 0;
    };

    class VehicleSimulation final : public snippetvehicle::EngineDriveVehicle
    {
    public:
        static constexpr uint32 MAX_AXLES = physx::PxVehicleLimits::eMAX_NB_WHEELS / 2;

        VehicleSimulation();

        static uint32 get_wheel_id(uint32 axle, bool left);

        bool build_params(const VehicleConfig& config, std::string& error);

        bool create(physx::PxPhysics& physics,
            const physx::PxCookingParams& cooking_params,
            physx::PxMaterial& material,
            physx::PxScene& scene);

        void apply_commands(const VehicleCommands& commands);
        void simulate(float dt);

        void apply_stability_assist(float strength);

        float get_forward_speed() const;
        float get_engine_rpm() const;
        float get_steer_angle() const;

        // -1 = reverse, 0 = neutral, 1.. = forward gears.
        int32 get_current_gear() const;

        uint32 get_nb_wheels() const;
        bool is_wheel_grounded(uint32 wheel_id) const;
        uint32 get_nb_grounded_wheels() const;

        physx::PxTransform get_wheel_local_pose(uint32 wheel_id, bool should_spin = true) const;

        physx::PxRigidBody* get_rigid_body() const;
        physx::PxShape* get_wheel_shape(uint32 wheel_id) const;

        static physx::PxShape* create_convex_shape(physx::PxPhysics& physics,
            const physx::PxCookingParams& cooking_params,
            physx::PxMaterial& material,
            const std::vector<physx::PxVec3>& points);

    private:
        void replace_chassis_shape_with_hull(physx::PxPhysics& physics, const physx::PxCookingParams& cooking_params, physx::PxMaterial& material);

    public:
        void getDataForSuspensionComponent(const physx::PxVehicleAxleDescription*& axleDescription,
            const physx::PxVehicleRigidBodyParams*& rigidBodyParams,
            const physx::PxVehicleSuspensionStateCalculationParams*& suspensionStateCalculationParams,
            physx::PxVehicleArrayData<const physx::PxReal>& steerResponseStates,
            const physx::PxVehicleRigidBodyState*& rigidBodyState,
            physx::PxVehicleArrayData<const physx::PxVehicleWheelParams>& wheelParams,
            physx::PxVehicleArrayData<const physx::PxVehicleSuspensionParams>& suspensionParams,
            physx::PxVehicleArrayData<const physx::PxVehicleSuspensionComplianceParams>& suspensionComplianceParams,
            physx::PxVehicleArrayData<const physx::PxVehicleSuspensionForceParams>& suspensionForceParams,
            physx::PxVehicleSizedArrayData<const physx::PxVehicleAntiRollForceParams>& antiRollForceParams,
            physx::PxVehicleArrayData<const physx::PxVehicleRoadGeometryState>& wheelRoadGeomStates,
            physx::PxVehicleArrayData<physx::PxVehicleSuspensionState>& suspensionStates,
            physx::PxVehicleArrayData<physx::PxVehicleSuspensionComplianceState>& suspensionComplianceStates,
            physx::PxVehicleArrayData<physx::PxVehicleSuspensionForce>& suspensionForces,
            physx::PxVehicleAntiRollTorque*& antiRollTorque) override;

        void getDataForRigidBodyComponent(const physx::PxVehicleAxleDescription*& axleDescription,
            const physx::PxVehicleRigidBodyParams*& rigidBodyParams,
            physx::PxVehicleArrayData<const physx::PxVehicleSuspensionForce>& suspensionForces,
            physx::PxVehicleArrayData<const physx::PxVehicleTireForce>& tireForces,
            const physx::PxVehicleAntiRollTorque*& antiRollTorque,
            physx::PxVehicleRigidBodyState*& rigidBodyState) override;

    public:
        physx::PxVehiclePhysXSimulationContext context;
        VehicleRoadQueryFilter road_query_filter;

        physx::PxVehicleAntiRollForceParams anti_roll_params[MAX_AXLES];
        uint32 nb_anti_roll = 0;
        physx::PxVehicleAntiRollTorque anti_roll_torque;

        Enum differential_type = eDIFFTYPE_MULTIWHEELDRIVE;
        float wheel_base = 0.0f;
        float max_grip = 1.0f;

        std::vector<physx::PxVec3> collision_hull;

        std::string name;
    };
}
