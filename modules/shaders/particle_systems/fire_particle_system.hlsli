#include "material.hlsli"
#include "camera.hlsli"
#include "particles.hlsli"

struct FireParticleData
{
	vec3 position;
	uint32 max_life_life;
	vec3 velocity;
	uint32 sin_angle_cos_angle;
};

defineSpline(float, 4)
defineSpline(float, 8)

struct FireParticleSettings
{
	spline(float, 4) size_over_lifetime;
	spline(float, 4) intensity_over_lifetime;
	spline(float, 4) atlas_progression_over_lifetime;
};

struct FireSimulationCB
{
	vec3 emit_position;
	uint32 frame_index;
	vec3 camera_position;
	uint32 padding;
};

struct FireRenderingCB
{
	FireParticleSettings settings;
	TextureAtlasCB atlas;
};

#ifdef HLSL
#define particle_data FireParticleData
#endif

#ifdef PARTICLE_SIMULATION

#define REQUIRES_SORTING

#define USER_PARTICLE_SIMULATION_RS \
	"RootConstants(num32BitConstants=8, b0)"

ConstantBuffer<FireSimulationCB> cb		: register(b0);

static particle_data emit_particle(uint emit_index)
{
	uint rng = init_rand(emit_index, cb.frame_index);

	float radius_at_distance_one = 0.1f;
	float2 disk_point = get_random_point_on_disk(rng, radius_at_distance_one);

	float3 position = cb.emit_position;
	float3 velocity = normalize(float3(1.f, disk_point.x, disk_point.y));
	velocity.x *= 4.f;

	float max_life = next_rand_between(rng, 1.3f, 1.8f);

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
		float max_life = unpack_halfs_left(particle.max_life_life);
		float rel_life = get_rel_life(life, max_life);

		float3 velocity = particle.velocity;
		velocity.y += 5.f * dt;

		const float damping = 1.f;
		//velocity *= 1.f / (1.f + dt * damping);

		particle.position = particle.position + velocity * dt;
		particle.max_life_life = pack_halfs(max_life, life);
		particle.velocity = velocity;

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
	float intensity			: INTENSITY;
	float2 uv0				: TEXCOORDS0;
	float2 uv1				: TEXCOORDS1;
	float uv_blend			: BLEND;
	float alpha_scale		: ALPHASCALE;
	float4 position			: SV_Position;
};

ConstantBuffer<FireRenderingCB> cb	: register(b0);
ConstantBuffer<CameraCB> camera		: register(b1);

Texture2D<float4> tex					: register(t0);
SamplerState tex_sampler					: register(s0);

static float2 get_uvs(TextureAtlasCB atlas, uint atlas_index, float2 original_uv)
{
	uint x = atlas.get_x(atlas_index);
	uint y = atlas.get_y(atlas_index);

	float inv_cols = atlas.get_inv_num_cols();
	float inv_rows = atlas.get_inv_num_rows();
	float2 uv0 = float2(x * inv_cols, y * inv_rows);
	float2 uv1 = float2((x + 1) * inv_cols, (y + 1) * inv_rows);

	return lerp(uv0, uv1, original_uv);
}

static VSOutput vertex_shader(VSInput IN, StructuredBuffer<particle_data> particles, uint index)
{
	float3 pos = particles[index].position;

	uint max_life_life = particles[index].max_life_life;
	float life, max_life;
	unpack_halfs(max_life_life, max_life, life);
	float rel_life = get_rel_life(life, max_life);

	float2 rotation; // Sin(angle), cos(angle).
	unpack_halfs(particles[index].sin_angle_cos_angle, rotation.x, rotation.y);

	float size = cb.settings.size_over_lifetime.evaluate(4, rel_life);
	float2 local_position = IN.position.xy * size;
	local_position = float2(dot(local_position, float2(rotation.y, -rotation.x)), dot(local_position, rotation));
	pos += local_position.x * camera.right.xyz + local_position.y * camera.up.xyz;

	TextureAtlasCB atlas = cb.atlas;
	float atlas_progression = cb.settings.atlas_progression_over_lifetime.evaluate(4, rel_life);
	float atlas_index = atlas_progression * (atlas.get_total_num_cells() - 1);
	float2 original_uv = IN.position.xy * 0.5f + 0.5f;

	VSOutput OUT;
	OUT.position = mul(camera.view_proj, float4(pos, 1.f));
	OUT.intensity = 20 * cb.settings.intensity_over_lifetime.evaluate(4, rel_life);

	OUT.uv0 = get_uvs(atlas, (uint)atlas_index, original_uv);
	OUT.uv1 = get_uvs(atlas, (uint)atlas_index + 1, original_uv);
	OUT.uv_blend = frac(atlas_index);

	OUT.alpha_scale = 1.f - smoothstep(0.9f, 1.f, atlas_progression);

	return OUT;
}

static float4 pixel_shader(VSOutput IN)
{
	float4 color = lerp(tex.Sample(tex_sampler, IN.uv0), tex.Sample(tex_sampler, IN.uv1), IN.uv_blend);
	float3 emission = color.rgb * float3(0.75f, 0.25f, 0.09f) * IN.intensity;
	return merge_alpha_blended(color.rgb, (float3)0.f, emission, color.a * IN.alpha_scale);
}

#endif

// Simulation
#define FIRE_PARTICLE_SYSTEM_COMPUTE_RS_CBV			0

// Rendering
#define FIRE_PARTICLE_SYSTEM_RENDERING_RS_CBV		0
#define FIRE_PARTICLE_SYSTEM_RENDERING_RS_CAMERA	1
#define FIRE_PARTICLE_SYSTEM_RENDERING_RS_TEXTURE	2