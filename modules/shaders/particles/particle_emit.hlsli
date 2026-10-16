#include "cs.hlsli"
#include "particles_rs.hlsli"

ConstantBuffer<ParticleStartCB> sim_cb					: register(b0, space1);

RWStructuredBuffer<ParticleDraw> draw_info				: register(u1, space1);
RWStructuredBuffer<ParticleCounters> counters			: register(u2, space1);

RWStructuredBuffer<particle_data> particles				: register(u3, space1);
RWStructuredBuffer<uint> dead_list						: register(u4, space1);
RWStructuredBuffer<uint> current_alive_list				: register(u5, space1);

[numthreads(PARTICLES_EMIT_BLOCK_SIZE, 1, 1)]
[RootSignature(PARTICLE_START_RS)]
void main(CSInput IN)
{
	const uint i = IN.dispatch_thread_id.x;
	const uint count = counters[0].new_particles;
	if (i >= count)
	{
		return;
	}

	uint dead;
	InterlockedAdd(counters[0].num_dead_particles, -1, dead);
	
	uint index = dead_list[dead - 1];

	particles[index] = emit_particle(i);

	uint alive;
	InterlockedAdd(counters[0].num_alive_particles_this_frame, 1, alive);

	current_alive_list[alive] = index;
}
