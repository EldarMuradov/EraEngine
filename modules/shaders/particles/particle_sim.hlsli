#include "cs.hlsli"
#include "particles_rs.hlsli"

ConstantBuffer<ParticleSimCB> sim_cb					: register(b0, space1);

RWStructuredBuffer<ParticleDraw> draw_info				: register(u1, space1);
RWStructuredBuffer<ParticleCounters> counters			: register(u2, space1);

RWStructuredBuffer<particle_data> particles				: register(u3, space1);
RWStructuredBuffer<uint> dead_list						: register(u4, space1);
RWStructuredBuffer<uint> current_alive_list				: register(u5, space1);
RWStructuredBuffer<uint> new_alive_list					: register(u6, space1);

#ifdef REQUIRES_SORTING
RWStructuredBuffer<float> sort_keys						: register(u7, space1);
#endif

[numthreads(PARTICLES_SIMULATE_BLOCK_SIZE, 1, 1)]
[RootSignature(PARTICLE_SIM_RS)]
void main(CSInput IN)
{
	const uint i = IN.dispatch_thread_id.x;
	const uint count = counters[0].num_alive_particles_this_frame;
	if (i >= count)
	{
		return;
	}

	float dt = sim_cb.dt;

	uint index = current_alive_list[i];

	particle_data particle = particles[index];

#ifdef REQUIRES_SORTING
	float sort_key;
	bool should_live = simulate_particle(particle, dt, sort_key);
#else
	bool should_live = simulate_particle(particle, dt);
#endif

	if (should_live)
	{
		particles[index] = particle;

		uint alive;
		InterlockedAdd(draw_info[0].arguments.InstanceCount, 1, alive);
		new_alive_list[alive] = index;

#ifdef REQUIRES_SORTING
		sort_keys[alive] = sort_key;
#endif
	}
	else
	{
		uint dead;
		InterlockedAdd(counters[0].num_dead_particles, 1, dead);
		dead_list[dead] = index;
	}
}