// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"
#include "core/math.h"

#include "rendering/render_pass.h"

#include "dx/dx_command_list.h"

#include "particles_rs.hlsli"

namespace era_engine
{
    struct DXCommandList;

    enum SortMode
    {
        SORT_MODE_NONE,
        SORT_MODE_FRONT_TO_BACK,
        SORT_MODE_BACK_TO_FRONT,
    };

    struct ERA_CORE_API CommonParticleSimulationData
    {
        mat4 prev_frame_camera_view;
        mat4 prev_frame_camera_view_proj;

        vec4 camera_projection_params;

        vec3 camera_position;

        ref<DXTexture> prev_frame_depth_buffer;
        ref<DXTexture> prev_frame_normals;
    };

    struct ERA_CORE_API ParticleSystem
    {
        virtual ~ParticleSystem() {}

        virtual void update(struct DXCommandList* cl, const CommonParticleSimulationData& common, float dt) = 0;

    protected:
        struct ParticleParameterSetter
        {
            virtual void set_root_parameters(DXCommandList* cl) = 0;
        };

        void initialize_as_billboard(uint32 particle_struct_size, uint32 max_num_particles, SortMode sort_mode = SORT_MODE_NONE);
        void initialize_as_mesh(uint32 particle_struct_size, DXMesh mesh, SubmeshInfo submesh, uint32 max_num_particles, SortMode sort_mode = SORT_MODE_NONE);

        ParticleDrawInfo get_draw_info(const struct DXPipeline& render_pipeline);

        static DXMesh billboard_mesh;
        SubmeshInfo submesh;

    protected:
        void update_internal(struct DXCommandList* cl, float new_particles, float dt, const DXPipeline& emit_pipeline, const DXPipeline& simulate_pipeline, ParticleParameterSetter* parameter_setter);

    private:
        static void initialize_pipeline();

        void initialize_internal(uint32 particle_struct_size, uint32 max_num_particles, SubmeshInfo submesh, SortMode sort_mode);

        template <typename T>
        void set_resources(DXCommandList* cl, const T& cb, uint32 offset, ParticleParameterSetter* parameter_setter)
        {
            cl->set_compute_32bit_constants(offset + PARTICLE_COMPUTE_RS_CB, cb);
            set_resources(cl, offset, parameter_setter);
        }

        void set_resources(DXCommandList* cl, uint32 offset, ParticleParameterSetter* parameter_setter);

        uint32 get_alive_list_offset(uint32 alive);
        uint32 get_dead_list_offset();
        uint32 get_num_user_root_parameters(const struct DXPipeline& pipeline);

        uint32 max_num_particles;
        uint32 current_alive = 0;

        ref<DXBuffer> particles_buffer;
        ref<DXBuffer> list_buffer; // Counters, dead, alive 0, alive 1.
        ref<DXBuffer> dispatch_buffer;
        ref<DXBuffer> sort_buffer;

        SortMode sort_mode;

        uint32 index;

        static ref<DXBuffer> particle_draw_command_buffer;
        static DXCommandSignature particle_command_signature;

        friend void initialize_render_utils();
        template <typename T>
        friend struct ParticleRenderPipeline;
    };

    template <typename render_data_t>
    struct ParticleRenderPipeline
    {
        static void render(DXCommandList* cl, const mat4& view_proj, const ParticleRenderCommand<render_data_t>& rc);
    };

    template <typename render_data_t>
    inline void ParticleRenderPipeline<render_data_t>::render(DXCommandList* cl, const mat4& view_proj, const ParticleRenderCommand<render_data_t>& rc)
    {
        const ParticleDrawInfo& info = rc.draw_info;

        cl->set_root_graphics_srv(info.root_parameter_offset + PARTICLE_RENDERING_RS_PARTICLES, info.particle_buffer->gpu_virtual_address);
        cl->set_root_graphics_srv(info.root_parameter_offset + PARTICLE_RENDERING_RS_ALIVE_LIST, info.alive_list->gpu_virtual_address + info.alive_list_offset);

        cl->set_vertex_buffer(0, rc.vertex_buffer.positions);
        if (rc.vertex_buffer.others)
            cl->set_vertex_buffer(1, rc.vertex_buffer.others);

        cl->set_index_buffer(rc.index_buffer);

        cl->draw_indirect(ParticleSystem::particle_command_signature, 1, info.command_buffer, info.command_buffer_offset);
    }
}

#define BUILD_PARTICLE_SHADER_NAME(name, suffix) name##suffix

#define EMIT_PIPELINE_NAME(name) BUILD_PARTICLE_SHADER_NAME(name, "_emit_cs")
#define SIMULATE_PIPELINE_NAME(name) BUILD_PARTICLE_SHADER_NAME(name, "_sim_cs")
#define VERTEX_SHADER_NAME(name) BUILD_PARTICLE_SHADER_NAME(name, "_vs")
#define PIXEL_SHADER_NAME(name) BUILD_PARTICLE_SHADER_NAME(name, "_ps")