// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "editor/transformation_gizmo.h"

#include "core/imgui.h"

#include "dx/dx_command_list.h"
#include "dx/dx_pipeline.h"

#include "geometry/mesh_builder.h"

#include "core/bounding_volumes.h"

#include "rendering/render_utils.h"
#include "rendering/debug_visualization.h"

namespace era_engine
{
    enum GizmoAxis
    {
        GIZMO_AXIS_X,
        GIZMO_AXIS_Y,
        GIZMO_AXIS_Z,
    };

    static union
    {
        struct
        {
            SubmeshInfo translation_submesh;
            SubmeshInfo rotation_submesh;
            SubmeshInfo scale_submesh;
            SubmeshInfo plane_submesh;
            SubmeshInfo box_submesh;
        };

        SubmeshInfo submeshes[5];
    };

    static BoundingCylinder cylinders[3] =
        {
            BoundingCylinder{vec3(0.f), vec3(1.f, 0.f, 0.f), 1.f},
            BoundingCylinder{vec3(0.f), vec3(0.f, 1.f, 0.f), 1.f},
            BoundingCylinder{vec3(0.f), vec3(0.f, 0.f, 1.f), 1.f},
    };

    static BoundingTorus tori[3] =
        {
            BoundingTorus{vec3(0.f), vec3(1.f, 0.f, 0.f), 1.f, 1.f},
            BoundingTorus{vec3(0.f), vec3(0.f, 1.f, 0.f), 1.f, 1.f},
            BoundingTorus{vec3(0.f), vec3(0.f, 0.f, 1.f), 1.f, 1.f},
    };

    struct GizmoRectangle
    {
        vec3 position;
        vec3 tangent;
        vec3 bitangent;
        vec2 radius;
    };

    static GizmoRectangle rectangles[] =
        {
            GizmoRectangle{vec3(1.f, 0.f, 1.f), vec3(1.f, 0.f, 0.f), vec3(0.f, 0.f, 1.f), vec2(1.f)},
            GizmoRectangle{vec3(1.f, 1.f, 0.f), vec3(1.f, 0.f, 0.f), vec3(0.f, 1.f, 0.f), vec2(1.f)},
            GizmoRectangle{vec3(0.f, 1.f, 1.f), vec3(0.f, 1.f, 0.f), vec3(0.f, 0.f, 1.f), vec2(1.f)},
    };

    static DXMesh mesh;

    void initialize_transformation_gizmos()
    {
        MeshBuilder mesh(MESH_CREATION_FLAGS_WITH_POSITIONS | MESH_CREATION_FLAGS_WITH_NORMALS);
        float shaft_length = 1.f;
        float head_length = 0.2f;
        float radius = 0.03f;
        float head_radius = 0.065f;

        ArrowMeshDesc arrow_mesh;
        arrow_mesh.slices = 6;
        arrow_mesh.shaft_radius = radius;
        arrow_mesh.head_radius = head_radius;
        arrow_mesh.shaft_length = shaft_length;
        arrow_mesh.head_length = head_length;
        mesh.push_arrow(arrow_mesh);
        translation_submesh = mesh.end_submesh();

        TorusMeshDesc torus_mesh;
        torus_mesh.slices = 6;
        torus_mesh.segments = 64;
        torus_mesh.torus_radius = shaft_length;
        torus_mesh.tube_radius = radius;
        mesh.push_torus(torus_mesh);
        rotation_submesh = mesh.end_submesh();

        MaceMeshDesc mace_mesh;
        mace_mesh.slices = 6;
        mace_mesh.shaft_radius = radius;
        mace_mesh.head_radius = head_radius;
        mace_mesh.shaft_length = shaft_length;
        mace_mesh.head_length = head_length;
        mesh.push_mace(mace_mesh);
        scale_submesh = mesh.end_submesh();

        BoxMeshDesc plane_mesh;
        plane_mesh.radius = vec3(shaft_length, 0.01f, shaft_length) * 0.2f;
        plane_mesh.center = vec3(shaft_length, 0.f, shaft_length) * 0.35f;
        mesh.push_box(plane_mesh);
        plane_submesh = mesh.end_submesh();

        BoxMeshDesc box_mesh;
        box_mesh.radius = (shaft_length + head_length) * 0.3f;
        mesh.push_box(box_mesh);
        box_submesh = mesh.end_submesh();

        era_engine::mesh = mesh.create_dx_mesh();

        for (uint32 i = 0; i < 3; ++i)
        {
            cylinders[i].position_b *= shaft_length + head_length;
            cylinders[i].radius *= radius * 1.1f;

            tori[i].major_radius *= shaft_length;
            tori[i].tube_radius *= radius * 1.1f;

            rectangles[i].position *= shaft_length * 0.35f;
            rectangles[i].radius *= shaft_length * 0.2f;
        }
    }

    uint32 TransformationGizmo::handle_translation(trs& transform, ray r, const UserInput& input, float snapping, float scaling)
    {
        quat rot = (space == TRANSFORMATION_GLOBAL) ? quat::identity : transform.rotation;

        uint32 hover_axis_index = -1;
        float min_t = FLT_MAX;
        for (uint32 i = 0; i < 3; ++i)
        {
            float t;
            BoundingCylinder cylinder = {rot * cylinders[i].position_a * scaling + transform.position, rot * cylinders[i].position_b * scaling + transform.position, cylinders[i].radius * scaling};
            if (r.intersect_cylinder(cylinder, t) && t < min_t)
            {
                hover_axis_index = i;
                min_t = t;
            }

            if (r.intersect_rectangle(rot * rectangles[i].position * scaling + transform.position, rot * rectangles[i].tangent, rot * rectangles[i].bitangent, rectangles[i].radius * scaling, t) && t < min_t)
            {
                hover_axis_index = i + 3;
                min_t = t;
            }
        }

        if (input.mouse.left.click_event && hover_axis_index != -1)
        {
            dragging = true;
            axis_index = hover_axis_index;

            if (hover_axis_index < 3)
            {
                vec3 candidate0(0.f);
                candidate0.data[(axis_index + 1) % 3] = 1.f;
                vec3 candidate1(0.f);
                candidate1.data[(axis_index + 2) % 3] = 1.f;

                const vec4 axis_planes[] =
                    {
                        create_plane(transform.position, rot * candidate0),
                        create_plane(transform.position, rot * candidate1),
                    };

                float a = abs(dot(r.direction, axis_planes[0].xyz));
                float b = abs(dot(r.direction, axis_planes[1].xyz));

                plane = (a > b) ? axis_planes[0] : axis_planes[1];

                vec3 axis(0.f);
                axis.data[axis_index] = 1.f;
                axis = rot * axis;

                float t;
                r.intersect_plane(plane.xyz, plane.w, t);
                anchor = dot(r.origin + t * r.direction - transform.position, axis);
            }
            else
            {
                plane = create_plane(rot * rectangles[axis_index - 3].position + transform.position, rot * cross(rectangles[axis_index - 3].tangent, rectangles[axis_index - 3].bitangent));
                anchor3 = r.origin + min_t * r.direction - transform.position;
            }
            original_position = transform.position;

            original_transform = transform;
        }

        if (dragging)
        {
            float t;
            r.intersect_plane(plane.xyz, plane.w, t);

            if (axis_index < 3)
            {
                vec3 axis(0.f);
                axis.data[axis_index] = 1.f;
                axis = rot * axis;

                float amount = (dot(r.origin + t * r.direction - original_position, axis) - anchor);

                if (snapping > 0.f)
                {
                    amount = ::round(amount / snapping) * snapping;
                }

                transform.position = original_position + amount * axis;
            }
            else
            {
                vec3 amount = r.origin + t * r.direction - original_position - anchor3;

                if (snapping > 0.f)
                {
                    vec3 tangent = rot * rectangles[axis_index - 3].tangent;
                    vec3 bitangent = rot * rectangles[axis_index - 3].bitangent;

                    float tangent_amount = ::round(dot(tangent, amount) / snapping) * snapping;
                    float bitangent_amount = ::round(dot(bitangent, amount) / snapping) * snapping;

                    amount = tangent * tangent_amount + bitangent * bitangent_amount;
                }

                transform.position = original_position + amount;
            }
        }

        return dragging ? axis_index : hover_axis_index;
    }

    uint32 TransformationGizmo::handle_rotation(trs& transform, ray r, const UserInput& input, float snapping, float scaling)
    {
        quat rot = (space == TRANSFORMATION_GLOBAL) ? quat::identity : transform.rotation;

        uint32 hover_axis_index = -1;
        float min_t = FLT_MAX;
        for (uint32 i = 0; i < 3; ++i)
        {
            float t;
            BoundingTorus torus = {rot * tori[i].position * scaling + transform.position, rot * tori[i].up_axis, tori[i].major_radius * scaling, tori[i].tube_radius * scaling};
            if (r.intersect_torus(torus, t) && t < min_t)
            {
                hover_axis_index = i;
                min_t = t;
            }
        }

        if (input.mouse.left.click_event && hover_axis_index != -1)
        {
            dragging = true;
            axis_index = hover_axis_index;

            vec3 plane_normal(0.f);
            plane_normal.data[axis_index] = 1.f;
            plane_normal = rot * plane_normal;
            plane = create_plane(transform.position, plane_normal);

            float t;
            r.intersect_plane(plane.xyz, plane.w, t);

            vec3 p = r.origin + t * r.direction - transform.position;
            p = conjugate(rot) * p;
            float x = p.data[(axis_index + 1) % 3];
            float y = p.data[(axis_index + 2) % 3];

            anchor = ::atan2(y, x);
            original_rotation = quat(plane_normal, anchor);

            original_transform = transform;
        }

        if (dragging)
        {
            float t;
            r.intersect_plane(plane.xyz, plane.w, t);

            vec3 p = r.origin + t * r.direction - transform.position;
            p = conjugate(rot) * p;
            float x = p.data[(axis_index + 1) % 3];
            float y = p.data[(axis_index + 2) % 3];

            float angle = ::atan2(y, x);

            if (snapping > 0.f)
            {
                angle = ::round((angle - anchor) / snapping) * snapping + anchor;
            }

            quat current_rotation(plane.xyz, angle);
            quat delta_rotation = current_rotation * conjugate(original_rotation);
            transform.rotation = normalize(delta_rotation * transform.rotation);
            if (space == TRANSFORMATION_GLOBAL)
            {
                original_rotation = current_rotation;
            }
        }

        return dragging ? axis_index : hover_axis_index;
    }

    uint32 TransformationGizmo::handle_scaling(trs& transform, ray r, const UserInput& input, float scaling, const RenderCamera& camera, bool allow_non_uniform)
    {
        // We only allow scaling in local space
        quat rot = transform.rotation;

        const float cylinder_height = cylinders[0].position_b.x;
        const float uniform_radius = cylinder_height * 0.3f * scaling;

        BoundingBox uniform_box = BoundingBox::from_center_radius(vec3(0.f), uniform_radius);

        ray local_ray = r;
        local_ray.origin = inverse_transform_position(transform, r.origin) * transform.scale; // inverseTransformPosition applies scale, but we don't want this.
        local_ray.direction = inverse_transform_direction(transform, r.direction);

        uint32 hover_axis_index = -1;
        float min_t = FLT_MAX;
        if (local_ray.intersect_aabb(uniform_box, min_t))
        {
            hover_axis_index = 3;
        }
        else if (allow_non_uniform)
        {
            float min_t = FLT_MAX;
            for (uint32 i = 0; i < 3; ++i)
            {
                float t;
                BoundingCylinder cylinder = {rot * cylinders[i].position_a * scaling + transform.position, rot * cylinders[i].position_b * scaling + transform.position, cylinders[i].radius * scaling};
                if (r.intersect_cylinder(cylinder, t) && t < min_t)
                {
                    hover_axis_index = i;
                    min_t = t;
                }
            }
        }

        if (input.mouse.left.click_event && hover_axis_index != -1)
        {
            if (hover_axis_index < 3)
            {
                // Non-uniform scaling
                vec3 candidate0(0.f);
                candidate0.data[(hover_axis_index + 1) % 3] = 1.f;
                vec3 candidate1(0.f);
                candidate1.data[(hover_axis_index + 2) % 3] = 1.f;

                const vec4 axis_planes[] =
                    {
                        create_plane(transform.position, rot * candidate0),
                        create_plane(transform.position, rot * candidate1),
                    };

                dragging = true;
                axis_index = hover_axis_index;

                float a = abs(dot(r.direction, axis_planes[0].xyz));
                float b = abs(dot(r.direction, axis_planes[1].xyz));

                plane = (a > b) ? axis_planes[0] : axis_planes[1];

                vec3 axis(0.f);
                axis.data[axis_index] = 1.f;
                axis = rot * axis;

                float t;
                r.intersect_plane(plane.xyz, plane.w, t);
                anchor = dot(r.origin + t * r.direction - transform.position, axis);
                anchor = max(anchor, 0.0001f);
                original_scale = transform.scale;
            }
            else
            {
                // Uniform scaling
                ASSERT(hover_axis_index == 3);

                dragging = true;
                axis_index = hover_axis_index;

                plane = create_plane(transform.position, camera.rotation * vec3(0.f, 0.f, 1.f)); // Backward axis.

                float t;
                r.intersect_plane(plane.xyz, plane.w, t);
                anchor = dot(r.origin + t * r.direction - transform.position, camera.rotation * vec3(1.f, 1.f, 0.f));
                anchor = max(anchor, 0.0001f);
                original_scale = transform.scale;
            }

            original_transform = transform;
        }

        if (dragging)
        {
            if (axis_index < 3)
            {
                float t;
                r.intersect_plane(plane.xyz, plane.w, t);

                vec3 axis(0.f);
                axis.data[axis_index] = 1.f;
                axis = rot * axis;

                vec3 d = (dot(r.origin + t * r.direction - transform.position, axis) / anchor) * axis;

                d = conjugate(rot) * d;
                transform.scale.data[axis_index] = original_scale.data[axis_index] * d.data[axis_index];
            }
            else
            {
                ASSERT(axis_index == 3);

                float t;
                r.intersect_plane(plane.xyz, plane.w, t);

                float d = (dot(r.origin + t * r.direction - transform.position, camera.rotation * vec3(1.f, 1.f, 0.f)) / anchor);

                transform.scale = original_scale * d;
            }
        }

        return dragging ? axis_index : hover_axis_index;
    }

    bool TransformationGizmo::handle_user_input(bool allow_keyboard_input,
        bool allow_translation, bool allow_rotation, bool allow_scaling, bool allow_space_change)
    {
        bool keyboard_interaction = false;
        bool ui_interaction = false;

        if (allow_keyboard_input && !ImGui::IsAnyItemActive() && !ImGui::AnyModifiersDown())
        {
            if (type != TRANSFORMATION_TYPE_SCALE)
            {
                if (ImGui::IsKeyPressed('G') && allow_space_change)
                {
                    space = (TransformationSpace)(1 - space);
                    dragging = false;
                    keyboard_interaction = true;
                }
            }
            if (ImGui::IsKeyPressed('Q'))
            {
                type = TRANSFORMATION_TYPE_NONE;
                dragging = false;
                keyboard_interaction = true;
            }
            if (ImGui::IsKeyPressed('W') && allow_translation)
            {
                type = TRANSFORMATION_TYPE_TRANSLATION;
                dragging = false;
                keyboard_interaction = true;
            }
            if (ImGui::IsKeyPressed('E') && allow_rotation)
            {
                type = TRANSFORMATION_TYPE_ROTATION;
                dragging = false;
                keyboard_interaction = true;
            }
            if (ImGui::IsKeyPressed('R') && allow_scaling)
            {
                type = TRANSFORMATION_TYPE_SCALE;
                dragging = false;
                keyboard_interaction = true;
            }
        }

        if (ImGui::BeginControlsWindow("##GizmoControls", ImVec2(0.f, 0.f), ImVec2(20.f, 20.f)))
        {
            TransformationSpace constant_local = TRANSFORMATION_LOCAL;
            TransformationSpace& space = (type == TRANSFORMATION_TYPE_SCALE) ? constant_local : this->space;

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

            bool allow_global = space == TRANSFORMATION_GLOBAL || allow_space_change;
            bool allow_local = space == TRANSFORMATION_LOCAL || allow_space_change;
            if (type == TRANSFORMATION_TYPE_SCALE)
            {
                allow_global = false;
            }

            ImGui::PushID(&this->space);
            ui_interaction |= ImGui::IconRadioButton(IMGUI_ICON_GLOBAL, (int*)&space, TRANSFORMATION_GLOBAL, IMGUI_ICON_DEFAULT_SIZE, allow_global);
            ImGui::SameLine(0.f, IMGUI_ICON_DEFAULT_SPACING);
            ui_interaction |= ImGui::IconRadioButton(IMGUI_ICON_LOCAL, (int*)&space, TRANSFORMATION_LOCAL, IMGUI_ICON_DEFAULT_SIZE, allow_local);
            ImGui::PopID();

            ImGui::SameLine(0.f, (float)IMGUI_ICON_DEFAULT_SIZE);

            ImGui::PushID(&type);
            ui_interaction |= ImGui::IconRadioButton(IMGUI_ICON_CROSS, (int*)&type, TRANSFORMATION_TYPE_NONE, IMGUI_ICON_DEFAULT_SIZE, true);
            ImGui::SameLine(0.f, IMGUI_ICON_DEFAULT_SPACING);
            ui_interaction |= ImGui::IconRadioButton(IMGUI_ICON_TRANSLATE, (int*)&type, TRANSFORMATION_TYPE_TRANSLATION, IMGUI_ICON_DEFAULT_SIZE, allow_translation);
            ImGui::SameLine(0.f, IMGUI_ICON_DEFAULT_SPACING);
            ui_interaction |= ImGui::IconRadioButton(IMGUI_ICON_ROTATE, (int*)&type, TRANSFORMATION_TYPE_ROTATION, IMGUI_ICON_DEFAULT_SIZE, allow_rotation);
            ImGui::SameLine(0.f, IMGUI_ICON_DEFAULT_SPACING);
            ui_interaction |= ImGui::IconRadioButton(IMGUI_ICON_SCALE, (int*)&type, TRANSFORMATION_TYPE_SCALE, IMGUI_ICON_DEFAULT_SIZE, allow_scaling);
            ImGui::PopID();

            ImGui::SameLine(0.f, (float)IMGUI_ICON_DEFAULT_SIZE);

            ImGui::PopStyleColor();
        }

        ImGui::End();

        return keyboard_interaction || ui_interaction;
    }

    void TransformationGizmo::manipulate_internal(trs& transform, const RenderCamera& camera, const UserInput& input, bool allow_input, LDRRenderPass* ldr_render_pass, bool allow_non_uniform_scale)
    {
        if (!input.mouse.left.down || !allow_input)
        {
            dragging = false;
        }

        if (type == TRANSFORMATION_TYPE_NONE)
            return;

        uint32 highlight_axis = -1;

        // Scale gizmos based on distance to camera.
        float scaling = length(transform.position - camera.position) / camera.get_min_projection_extent() * 0.1f;

        if (allow_input)
        {
            float snapping = input.keyboard[KEY_CTRL].down ? (type == TRANSFORMATION_TYPE_ROTATION ? deg2rad(45.f) : 0.5f) : 0.f;

            ray r = camera.generate_world_space_ray(input.mouse.rel_x, input.mouse.rel_y);

            switch (type)
            {
                case TRANSFORMATION_TYPE_TRANSLATION:
                    highlight_axis = handle_translation(transform, r, input, snapping, scaling);
                    break;
                case TRANSFORMATION_TYPE_ROTATION:
                    highlight_axis = handle_rotation(transform, r, input, snapping, scaling);
                    break;
                case TRANSFORMATION_TYPE_SCALE:
                    highlight_axis = handle_scaling(transform, r, input, scaling, camera, allow_non_uniform_scale);
                    break; // TODO: Snapping for scale.
            }
        }

        // Render
        quat rot = (space == TRANSFORMATION_GLOBAL && type != TRANSFORMATION_TYPE_SCALE) ? quat::identity : transform.rotation;

        {
            const quat rotations[] =
                {
                    rot * quat(vec3(0.f, 0.f, -1.f), deg2rad(90.f)),
                    rot,
                    rot * quat(vec3(1.f, 0.f, 0.f), deg2rad(90.f)),
                };

            const vec4 colors[] =
                {
                    vec4(1.f, 0.f, 0.f, 1.f),
                    vec4(0.f, 1.f, 0.f, 1.f),
                    vec4(0.f, 0.f, 1.f, 1.f),
                };

            if (type != TRANSFORMATION_TYPE_SCALE || allow_non_uniform_scale)
            {
                for (uint32 i = 0; i < 3; ++i)
                {
                    DebugRenderData data = {
                        create_model_matrix(transform.position, rotations[i], scaling),
                        mesh.vertex_buffer, mesh.index_buffer,
                        submeshes[type],
                        colors[i] * (highlight_axis == i ? 0.5f : 1.f)};

                    ldr_render_pass->render_overlay<DebugSimplePipeline>(data);
                }
            }

            if (type == TRANSFORMATION_TYPE_SCALE)
            {
                DebugRenderData data = {
                    create_model_matrix(transform.position, rot, scaling),
                    mesh.vertex_buffer, mesh.index_buffer,
                    box_submesh,
                    vec4(0.5f) * (highlight_axis == 3 ? 0.5f : 1.f)};

                ldr_render_pass->render_overlay<DebugSimplePipeline>(data);
            }
        }

        if (type == TRANSFORMATION_TYPE_TRANSLATION)
        {
            const quat rotations[] =
                {
                    rot,
                    rot * quat(vec3(1.f, 0.f, 0.f), deg2rad(-90.f)),
                    rot * quat(vec3(0.f, 0.f, 1.f), deg2rad(90.f)),
                };

            const vec4 colors[] =
                {
                    vec4(1.f, 0.f, 1.f, 1.f),
                    vec4(1.f, 1.f, 0.f, 1.f),
                    vec4(0.f, 1.f, 1.f, 1.f),
                };

            for (uint32 i = 0; i < 3; ++i)
            {
                DebugRenderData data = {
                    create_model_matrix(transform.position, rotations[i], scaling),
                    mesh.vertex_buffer, mesh.index_buffer,
                    plane_submesh,
                    colors[i] * (highlight_axis == i + 3 ? 0.5f : 1.f)};

                ldr_render_pass->render_overlay<DebugSimplePipeline>(data);
            }
        }
    }

    void TransformationGizmo::manipulate_handles(vec3* handles, uint32 num_handles)
    {
    }

    bool TransformationGizmo::manipulate_transformation(trs& transform, const RenderCamera& camera, const UserInput& input, bool allow_input, LDRRenderPass* ldr_render_pass)
    {
        bool input_captured = handle_user_input(allow_input, true, true, true, true);
        manipulate_internal(transform, camera, input, allow_input, ldr_render_pass);
        return dragging;
    }

    bool TransformationGizmo::manipulate_position(vec3& position, const RenderCamera& camera, const UserInput& input, bool allow_input, LDRRenderPass* ldr_render_pass)
    {
        if (type == TRANSFORMATION_TYPE_ROTATION || type == TRANSFORMATION_TYPE_SCALE)
        {
            type = TRANSFORMATION_TYPE_TRANSLATION;
        }
        space = TRANSFORMATION_GLOBAL;

        bool input_captured = handle_user_input(allow_input, true, false, false, false);
        trs transform = {position, quat::identity, vec3(1.f, 1.f, 1.f)};
        manipulate_internal(transform, camera, input, allow_input, ldr_render_pass);
        position = transform.position;
        return dragging;
    }

    bool TransformationGizmo::manipulate_position_rotation(vec3& position, quat& rotation, const RenderCamera& camera, const UserInput& input, bool allow_input, LDRRenderPass* ldr_render_pass)
    {
        if (type == TRANSFORMATION_TYPE_SCALE)
        {
            type = TRANSFORMATION_TYPE_TRANSLATION;
        }

        bool input_captured = handle_user_input(allow_input, true, true, false, true);
        trs transform = {position, rotation, vec3(1.f, 1.f, 1.f)};
        manipulate_internal(transform, camera, input, allow_input, ldr_render_pass);
        position = transform.position;
        rotation = transform.rotation;
        return dragging;
    }

    bool TransformationGizmo::manipulate_position_scale(vec3& position, vec3& scale, const RenderCamera& camera, const UserInput& input, bool allow_input, LDRRenderPass* ldr_render_pass)
    {
        if (type == TRANSFORMATION_TYPE_ROTATION)
        {
            type = TRANSFORMATION_TYPE_TRANSLATION;
        }

        bool input_captured = handle_user_input(allow_input, true, false, true, false);
        trs transform = {position, quat::identity, scale};
        manipulate_internal(transform, camera, input, allow_input, ldr_render_pass);
        position = transform.position;
        scale = transform.scale;
        return dragging;
    }

    bool TransformationGizmo::manipulate_nothing(const RenderCamera& camera, const UserInput& input, bool allow_input, LDRRenderPass* ldr_render_pass)
    {
        dragging = false;
        bool input_captured = handle_user_input(allow_input, true, true, true, true);
        return false;
    }

    bool TransformationGizmo::manipulate_bounding_sphere(BoundingSphere& sphere, const trs& parent_transform, const RenderCamera& camera, const UserInput& input, bool allow_input, LDRRenderPass* ldr_render_pass)
    {
        if (type == TRANSFORMATION_TYPE_ROTATION)
        {
            type = TRANSFORMATION_TYPE_TRANSLATION;
        }

        bool input_captured = handle_user_input(allow_input, true, false, true, true);
        trs transform = {parent_transform.rotation * sphere.center + parent_transform.position, parent_transform.rotation, vec3(sphere.radius)};
        manipulate_internal(transform, camera, input, allow_input, ldr_render_pass, false);
        sphere.center = conjugate(parent_transform.rotation) * (transform.position - parent_transform.position);
        sphere.radius = transform.scale.x;
        return dragging;
    }

    bool TransformationGizmo::manipulate_bounding_capsule(BoundingCapsule& capsule, const trs& parent_transform, const RenderCamera& camera, const UserInput& input, bool allow_input, LDRRenderPass* ldr_render_pass)
    {
        bool input_captured = handle_user_input(allow_input, true, true, true, true);

        vec3 a = parent_transform.position + parent_transform.rotation * capsule.position_a;
        vec3 b = parent_transform.position + parent_transform.rotation * capsule.position_b;

        vec3 center = 0.5f * (a + b);

        trs transform = {center, parent_transform.rotation, capsule.radius};

        a = inverse_transform_position(transform, a);
        b = inverse_transform_position(transform, b);

        manipulate_internal(transform, camera, input, allow_input, ldr_render_pass, false);

        a = transform_position(transform, a);
        b = transform_position(transform, b);

        capsule.position_a = conjugate(parent_transform.rotation) * (a - parent_transform.position);
        capsule.position_b = conjugate(parent_transform.rotation) * (b - parent_transform.position);

        capsule.radius = transform.scale.x;
        return dragging;
    }

    bool TransformationGizmo::manipulate_bounding_cylinder(BoundingCylinder& cylinder, const trs& parent_transform, const RenderCamera& camera, const UserInput& input, bool allow_input, LDRRenderPass* ldr_render_pass)
    {
        BoundingCapsule c = {cylinder.position_a, cylinder.position_b, cylinder.radius};
        bool result = manipulate_bounding_capsule(c, parent_transform, camera, input, allow_input, ldr_render_pass);
        cylinder = {c.position_a, c.position_b, c.radius};
        return result;
    }

    bool TransformationGizmo::manipulate_bounding_box(BoundingBox& aabb, const trs& parent_transform, const RenderCamera& camera, const UserInput& input, bool allow_input, LDRRenderPass* ldr_render_pass)
    {
        if (type == TRANSFORMATION_TYPE_ROTATION)
        {
            type = TRANSFORMATION_TYPE_TRANSLATION;
        }

        bool input_captured = handle_user_input(allow_input, true, false, true, true);
        trs transform = {parent_transform.rotation * aabb.get_center() + parent_transform.position, parent_transform.rotation, aabb.get_radius()};
        manipulate_internal(transform, camera, input, allow_input, ldr_render_pass);
        vec3 center = conjugate(parent_transform.rotation) * (transform.position - parent_transform.position);
        vec3 radius = transform.scale;
        aabb = BoundingBox::from_center_radius(center, radius);
        return dragging;
    }

    bool TransformationGizmo::manipulate_oriented_bounding_box(BoundingOrientedBox& obb, const trs& parent_transform, const RenderCamera& camera, const UserInput& input, bool allow_input, LDRRenderPass* ldr_render_pass)
    {
        bool input_captured = handle_user_input(allow_input, true, true, true, true);
        trs transform = {parent_transform.rotation * obb.center + parent_transform.position, parent_transform.rotation, obb.radius};
        manipulate_internal(transform, camera, input, allow_input, ldr_render_pass);
        obb.center = conjugate(parent_transform.rotation) * (transform.position - parent_transform.position);
        obb.rotation = conjugate(parent_transform.rotation) * transform.rotation * obb.rotation;
        obb.radius = transform.scale;
        return dragging;
    }
}