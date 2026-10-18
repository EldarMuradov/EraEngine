// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "core/camera.h"
#include "core/bounding_volumes.h"

namespace era_engine
{
    struct UserInput;

    struct ERA_CORE_API RuntimeCameraController
    {
        void initialize(RenderCamera* camera) { this->camera = camera; }

        void update(uint32 viewport_width, uint32 viewport_height);

        RenderCamera* camera = nullptr;
    };

    struct ERA_CORE_API CameraController
    {
        void initialize(RenderCamera* camera) { this->camera = camera; }

        // Returns true, if camera is moved, and therefore input is captured
        bool center_camera_on_object(const BoundingBox& aabb);
        bool update(const UserInput& input, uint32 viewport_width, uint32 viewport_height, float dt);

        RenderCamera* camera = nullptr;

    private:
        float orbit_radius = 10.f;
        float centering_time = -1.f;

        vec3 centering_position_start;
        vec3 centering_position_target;
    };
}