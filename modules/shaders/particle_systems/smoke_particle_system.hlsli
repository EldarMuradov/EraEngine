#include "material.hlsli"
#include "camera.hlsli"

struct SmokeParticleData
{
	vec3 position;
	uint32 max_life_life;
	vec3 velocity;
	uint32 sin_angle_cos_angle;
};

defineSpline(float, 4)
defineSpline(float, 8)

struct SmokeParticleSettings
{
	spline(float, 8) life_scale_from_distance;
	spline(float, 4) intensity_over_lifetime;
	spline(float, 4) atlas_progression_over_lifetime;
};

struct SmokeSimulationCB
{
	vec3 emit_position;
	uint32 frame_index;
	vec3 camera_position;
	uint32 padding;

	spline(float, 8) life_scale_from_distance;
};

struct SmokeRenderingCB
{
	spline(float, 4) intensity_over_lifetime;
	spline(float, 4) atlas_progression_over_lifetime;

	TextureAtlasCB atlas;
};

#ifdef HLSL
#define particle_data SmokeParticleData
#endif

#ifdef PARTICLE_SIMULATION

#define REQUIRES_SORTING

#define USER_PARTICLE_SIMULATION_RS \
	"CBV(b0)"

ConstantBuffer<SmokeSimulationCB> cb		: register(b0);

static particle_data emit_particle(uint emit_index)
{
	uint rng = init_rand(emit_index, cb.frame_index);

	const float radius = 4.f;

	float2 offset_2d = get_random_point_on_disk(rng, radius);
	float3 offset = float3(offset_2d.x, next_rand(rng) * 0.5f, offset_2d.y);
	float3 position = cb.emit_position + offset;
	float3 velocity = float3(next_rand(rng), next_rand(rng) * 3.f + 4.f, next_rand(rng));

	float distance = saturate(length(offset.xz) / radius);
	float life_scale = cb.life_scale_from_distance.evaluate(8, distance);

	float max_life = next_rand(rng) * 5.f * life_scale + 3.f;

	float angle = next_rand(rng) * 2.f * M_PI;
	float sin_angle, cos_angle;
	sincos(angle, sin_angle, cos_angle);

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
		float3 gravity = float3(0.f, -1.f * dt, 0.f);
		particle.position = particle.position + 0.5f * gravity * dt + particle.velocity * dt;
		particle.velocity = particle.velocity + gravity;
		float max_life = unpack_halfs_left(particle.max_life_life);
		particle.max_life_life = pack_halfs(max_life, life);

		float3 V = particle.position - cb.camera_position;
		sort_key = dot(V, V);

		return true;
	}
}

#endif

#ifdef PARTICLE_RENDERING

#define USER_PARTICLE_RENDERING_RS \
    "CBV(b0, visibility=SHADER_VISIBILITY_VERTEX), " \
    "CBV(b1, visibility=SHADER_VISIBILITY_VERTEX), " \
	"DescriptorTable(SRV(t0, numDescriptors=1), visibility=SHADER_VISIBILITY_PIXEL), " \
	"StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_WRAP," \
        "addressV = TEXTURE_ADDRESS_WRAP," \
        "addressW = TEXTURE_ADDRESS_WRAP," \
        "filter = FILTER_MIN_MAG_MIP_LINEAR," \
        "visibility=SHADER_VISIBILITY_PIXEL)"

struct VSInput
{
	float3 position			: POSITION;
};

struct VSOutput
{
	float intensity : INTENSITY;
	float2 uv				: TEXCOORDS;
	float4 position			: SV_Position;
};

ConstantBuffer<SmokeRenderingCB> cb	: register(b0);
ConstantBuffer<CameraCB> camera		: register(b1);

Texture2D<float4> tex					: register(t0);
SamplerState tex_sampler					: register(s0);

static VSOutput vertex_shader(VSInput IN, StructuredBuffer<particle_data> particles, uint index)
{
	float3 pos = particles[index].position;

	uint max_life_life = particles[index].max_life_life;
	float life = unpack_halfs_right(max_life_life);
	float max_life = unpack_halfs_left(max_life_life);
	float rel_life = clamp(1.f - life / max_life, 0.01f, 0.99f);

	float2 rotation; // Sin(angle), cos(angle).
	unpack_halfs(particles[index].sin_angle_cos_angle, rotation.x, rotation.y);

	float2 local_position = IN.position.xy * 1.f;
	local_position = float2(dot(local_position, float2(rotation.y, -rotation.x)), dot(local_position, rotation));
	pos += local_position.x * camera.right.xyz + local_position.y * camera.up.xyz;

	TextureAtlasCB atlas = cb.atlas;
	uint atlas_index = cb.atlas_progression_over_lifetime.evaluate(4, rel_life) * (atlas.get_total_num_cells() - 1);
	uint x = atlas.get_x(atlas_index);
	uint y = atlas.get_y(atlas_index);

	float inv_cols = atlas.get_inv_num_cols();
	float inv_rows = atlas.get_inv_num_rows();
	float2 uv0 = float2(x * inv_cols, y * inv_rows);
	float2 uv1 = float2((x + 1) * inv_cols, (y + 1) * inv_rows);

	VSOutput OUT;
	OUT.position = mul(camera.view_proj, float4(pos, 1.f));
	OUT.uv = lerp(uv0, uv1, IN.position.xy * 0.5f + 0.5f);
	OUT.intensity = 10.f;// cb.intensityOverLifetime.evaluate(4, relLife);
	return OUT;
}

static float4 pixel_shader(VSOutput IN)
{
	float4 color = tex.Sample(tex_sampler, IN.uv);
	color.rgb *= IN.intensity;
	return color;
}

#endif

// Simulation
#define SMOKE_PARTICLE_SYSTEM_COMPUTE_RS_CBV		0

// Rendering
#define SMOKE_PARTICLE_SYSTEM_RENDERING_RS_CBV		0
#define SMOKE_PARTICLE_SYSTEM_RENDERING_RS_CAMERA	1
#define SMOKE_PARTICLE_SYSTEM_RENDERING_RS_TEXTURE	2