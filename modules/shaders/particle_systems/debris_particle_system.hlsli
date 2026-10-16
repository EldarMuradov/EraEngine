#include "material.hlsli"
#include "camera.hlsli"
#include "particles.hlsli"

struct DebrisParticleData
{
	vec3 position;
	uint32 max_life_life;
	vec3 velocity;
	uint32 sin_angle_cos_angle;
};

struct DebrisSimulationCB
{
	vec4 emit_positions[4];

	mat4 camera_vp;
	vec4 camera_projection_params;

	vec3 camera_position;
	uint32 frame_index;
	float drag;
};

#ifdef HLSL
#define particle_data DebrisParticleData
#endif

#ifdef PARTICLE_SIMULATION

#include "normal.hlsli"
#include "random.hlsli"

#define REQUIRES_SORTING

#define USER_PARTICLE_SIMULATION_RS \
	"CBV(b0), " \
	"DescriptorTable(SRV(t0, numDescriptors=2)), " \
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_BORDER," \
        "addressV = TEXTURE_ADDRESS_BORDER," \
        "addressW = TEXTURE_ADDRESS_BORDER," \
		"borderColor = STATIC_BORDER_COLOR_OPAQUE_WHITE," \
        "filter = FILTER_MIN_MAG_MIP_POINT)"

ConstantBuffer<DebrisSimulationCB> cb	: register(b0);
Texture2D<float> depth_buffer			: register(t0);
Texture2D<float2> scene_normals			: register(t1);
SamplerState point_sampler				: register(s0);

static particle_data emit_particle(uint emit_index)
{
	uint rng = init_rand(emit_index, cb.frame_index);

	uint emit_batch = emit_index / 256;
	emit_index = emit_index % 256;

	float2 random_on_disk = get_random_point_on_unit_disk(rng) * 0.3f;
	float3 position = float3(random_on_disk.x, 0.f, random_on_disk.y);
	float max_life = 5.5f;

	float3 velocity = normalize(position);

	position += cb.emit_positions[emit_batch].xyz;

	float i = (float)emit_index * 2.f * M_PI;
	float sin_angle, cos_angle;
	sincos(i, sin_angle, cos_angle);

	particle_data particle = {
		position,
		pack_halfs(max_life, max_life),
		velocity,
		pack_halfs(sin_angle, cos_angle)
	};

	return particle;
}

static bool simulate_particle(inout particle_data particle, float dt, out float sort_key)
{
	float life = unpack_halfs_right(particle.max_life_life);
	life -= dt;
	if (life <= 0)
	{
		sort_key = 0;
		return false;
	}
	else
	{
		float max_life = unpack_halfs_left(particle.max_life_life);
		float rel_life = get_rel_life(life, max_life);

		particle.velocity.y -= 9.81f * dt;
		particle.velocity *= cb.drag;

		float4 p = mul(cb.camera_vp, float4(particle.position, 1.f));
		p.xyz /= p.w;
		float2 uv = p.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

		float particle_depth = depth_buffer_depth_to_eye_depth(p.z, cb.camera_projection_params);
		float scene_depth = depth_buffer_depth_to_eye_depth(is_saturated(p.z) ? depth_buffer.SampleLevel(point_sampler, uv, 0) : 1.f, cb.camera_projection_params); // Due to the border sampler, this defaults to 1 (infinite depth) outside the image.


		if (particle_depth > scene_depth - 0.05f && particle_depth < scene_depth + 1.f)
		{
			float3 scene_normal = normalize(unpack_normal(scene_normals.SampleLevel(point_sampler, uv, 0)));

			float vdot_n = dot(particle.velocity, scene_normal);
			if (vdot_n < 0.f)
			{
				particle.velocity = reflect(particle.velocity, scene_normal) * 0.5f;
				//particle.position += sceneNormal * (particleDepth - sceneDepth);
			}
		}

		particle.position += particle.velocity * dt;

		particle.max_life_life = pack_halfs(max_life, life);

		float3 V = particle.position - cb.camera_position;
		sort_key = dot(V, V);

		return true;
	}
}

#endif

#ifdef PARTICLE_RENDERING

#define USER_PARTICLE_RENDERING_RS \
    "CBV(b0, visibility=SHADER_VISIBILITY_VERTEX)"

struct VSInput
{
	float3 position			: POSITION;
};

struct VSOutput
{
	float2 uv				: TEXCOORD;
	float4 position			: SV_Position;
};

ConstantBuffer<CameraCB> camera		: register(b0);

static VSOutput vertex_shader(VSInput IN, StructuredBuffer<particle_data> particles, uint index)
{
	float3 pos = particles[index].position;

	float2 rotation; // Sin(angle), cos(angle).
	unpack_halfs(particles[index].sin_angle_cos_angle, rotation.x, rotation.y);

	float2 uv = IN.position.xy * 0.5f + 0.5f;

	float size = 0.1f;
	float2 local_position = IN.position.xy * size;
	local_position = float2(dot(local_position, float2(rotation.y, -rotation.x)), dot(local_position, rotation));
	pos += local_position.x * camera.right.xyz + local_position.y * camera.up.xyz;

	VSOutput OUT;
	OUT.position = mul(camera.view_proj, float4(pos, 1.f));
	//OUT.color = particles[index].velocity;
	OUT.uv = uv;
	return OUT;
}

static float4 pixel_shader(VSOutput IN)
{
	float radius = 0.5f;
	float2 o = IN.uv - float2(0.5f, 0.5f);

	float2 o2 = o * 2.f;
	float z = 1 - sqrt(dot(o2, o2));
	float3 N = float3(o2.xy, z);

	float alpha = step(dot(o, o), radius * radius);
	return float4(abs(N), alpha);
}

#endif

// Simulation
#define DEBRIS_PARTICLE_SYSTEM_COMPUTE_RS_CBV				0
#define DEBRIS_PARTICLE_SYSTEM_COMPUTE_RS_TEXTURES			1

// Rendering
#define DEBRIS_PARTICLE_SYSTEM_RENDERING_RS_CAMERA			0