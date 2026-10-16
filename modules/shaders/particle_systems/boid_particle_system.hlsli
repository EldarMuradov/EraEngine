#include "camera.hlsli"

struct BoidParticleData
{
	vec3 position;
	uint32 max_life_life;
	vec3 velocity;
	uint32 color;
};

struct BoidParticleSettings
{
	float radius;
};

struct BoidSimulationCB
{
	vec3 emit_position;
	uint32 frame_index;
	float radius;
};

#ifdef HLSL
#define particle_data BoidParticleData

#include "material.hlsli"
#endif

#ifdef PARTICLE_SIMULATION

#define USER_PARTICLE_SIMULATION_RS \
	"RootConstants(num32BitConstants=5, b0)"

ConstantBuffer<BoidSimulationCB> cb		: register(b0);

static particle_data emit_particle(uint emit_index)
{
	uint rng = init_rand(emit_index, cb.frame_index);

	const float radius = cb.radius;

	float2 offset_2d = get_random_point_on_disk(rng, radius);
	float height = next_rand(rng) * 2.f + 3.f;
	float3 position = cb.emit_position + float3(offset_2d.x, height, offset_2d.y);

	float max_life = next_rand(rng) + 4.5f;

	float2 velocity = get_random_point_on_unit_disk(rng) * (5.f + next_rand(rng) * 1.5f);

	particle_data particle = {
		position,
		pack_halfs(max_life, max_life),
		float3(velocity.x, 0.f, velocity.y),
		pack_color(0, 0, 255, 255)  //packColor(nextRand(rng), nextRand(rng), nextRand(rng), 1.f)
	};

	return particle;
}

static bool simulate_particle(inout particle_data particle, float dt)
{
	float life = unpack_halfs_right(particle.max_life_life);
	life -= dt;
	if (life <= 0)
	{
		return false;
	}
	else
	{
		float3 gravity = float3(0.f, -9.81f * dt, 0.f);
		particle.position = particle.position + 0.5f * gravity * dt + particle.velocity * dt;
		particle.velocity = particle.velocity + gravity;
		if (particle.position.y <= cb.emit_position.y)
		{
			particle.position.y = cb.emit_position.y;
			particle.velocity.y = 0.f;
		}

		float max_life = unpack_halfs_left(particle.max_life_life);
		particle.max_life_life = pack_halfs(max_life, life);

		return true;
	}
}

#endif

#ifdef PARTICLE_RENDERING

#include "lighting.hlsli"

#define USER_PARTICLE_RENDERING_RS \
    "CBV(b0), " \
	"DescriptorTable(SRV(t0, numDescriptors=3), visibility=SHADER_VISIBILITY_PIXEL), " \
	"StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_CLAMP," \
        "addressV = TEXTURE_ADDRESS_CLAMP," \
        "addressW = TEXTURE_ADDRESS_CLAMP," \
        "filter = FILTER_MIN_MAG_MIP_LINEAR," \
        "visibility=SHADER_VISIBILITY_PIXEL)"

struct VSInput
{
	float3 position				: POSITION;
	float2 uv					: TEXCOORDS;
	float3 normal				: NORMAL;
	float3 tangent				: TANGENT;
};

struct VSOutput
{
	float3 world_position		: POSITION;
	float3 normal				: NORMAL;
	nointerpolation uint color	: COLOR;
	float4 position				: SV_Position;
};

ConstantBuffer<CameraCB> camera		: register(b0);

TextureCube<float4> irradiance_texture	: register(t0);
TextureCube<float4> environment_texture	: register(t1);
Texture2D<float2> brdf					: register(t2);

SamplerState clamp_sampler				: register(s0);

static VSOutput vertex_shader(VSInput IN, StructuredBuffer<particle_data> particles, uint index)
{
	uint max_life_life = particles[index].max_life_life;
	float life = unpack_halfs_right(max_life_life);
	float max_life = unpack_halfs_left(max_life_life);
	float rel_life = clamp(1.f - life / max_life, 0.01f, 0.99f);

	float scale = smoothstep(rel_life, 0.f, 0.005f);
	scale = min(scale, smoothstep(1.f - rel_life, 0.f, 0.005f));


	float3 backward = particles[index].velocity;
	backward.y = 0.f;
	backward = normalize(backward);
	float3 up = float3(0.f, 1.f, 0.f);
	float3 right = cross(up, backward);

	float3 local_position = IN.position;
	float3 local_normal = IN.normal;

	float3 rotated_position = local_position.x * right + local_position.y * up + local_position.z * backward;
	float3 rotated_normal = local_normal.x * right + local_normal.y * up + local_normal.z * backward;

	float3 pos = particles[index].position + rotated_position * scale;

	VSOutput OUT;
	OUT.world_position = pos;
	OUT.position = mul(camera.view_proj, float4(pos, 1.f));
	OUT.normal = rotated_normal;
	OUT.color = particles[index].color;
	return OUT;
}

static float4 pixel_shader(VSOutput IN)
{
	SurfaceInfo surface;

	surface.albedo = unpack_color(IN.color);
	surface.N = normalize(IN.normal);

	surface.roughness = 0.6f;
	surface.roughness = clamp(surface.roughness, 0.01f, 0.99f);
	surface.metallic = 0.f;

	surface.emission = float3(0.f, 0.f, 0.f);

	surface.P = IN.world_position;
	float3 cam_to_p = surface.P - camera.position.xyz;
	surface.V = -normalize(cam_to_p);

	surface.infer_remaining_properties();

	float3 L = normalize(float3(1.f, 0.5f, -1.f));
	float3 radiance = float3(1.f, 1.f, 1.f) * 20.f;

	LightInfo light;
	light.initialize(surface, L, radiance);

	LightContribution total_lighting = { float3(0.f, 0.f, 0.f), float3(0.f, 0.f, 0.f) };
	total_lighting.add(calculate_direct_lighting(surface, light), 1.f);

	AmbientFactors factors = get_ambient_factors(surface);
	total_lighting.diffuse += diffuse_ibl(factors.kd, surface, irradiance_texture, clamp_sampler);
	total_lighting.specular += specular_ibl(factors.ks, surface, environment_texture, brdf, clamp_sampler);

	return total_lighting.evaluate(surface.albedo);
}

#endif

// Simulation
#define BOID_PARTICLE_SYSTEM_COMPUTE_RS_CBV			0

// Rendering
#define BOID_PARTICLE_SYSTEM_RENDERING_RS_CAMERA	0
#define BOID_PARTICLE_SYSTEM_RENDERING_RS_PBR		1