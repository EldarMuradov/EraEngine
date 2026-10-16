#include "cs.hlsli"
#include "particles_rs.hlsli"

ConstantBuffer<ParticleStartCB> cb					: register(b0, space1);

RWStructuredBuffer<ParticleDispatch> dispatch_info		: register(u0, space1);
RWStructuredBuffer<ParticleDraw> draw_info				: register(u1, space1);
RWStructuredBuffer<ParticleCounters> counters			: register(u2, space1);

static uint bucketize(uint problem_size, uint bucket_size) 
{ 
	return (problem_size + bucket_size - 1) / bucket_size; 
}

[numthreads(1, 1, 1)]
[RootSignature(PARTICLE_START_RS)]
void main(CSInput IN)
{
	ParticleCounters c = counters[0];
	ParticleDraw draw = draw_info[0];
	ParticleDispatch dispatch = dispatch_info[0];

	c.emit_rate_accum += cb.new_particles_request; // TODO: How do we prevent this from running to infinity, if the system is saturated?

	uint space_left = c.num_dead_particles;
	uint num_new_particles = min((uint)c.emit_rate_accum, space_left);

	c.emit_rate_accum -= num_new_particles;

	uint num_alive_particles_in_last_frame = draw.arguments.InstanceCount;

	dispatch.emit.ThreadGroupCountX = bucketize(num_new_particles, PARTICLES_EMIT_BLOCK_SIZE);
	dispatch.emit.ThreadGroupCountY = 1;
	dispatch.emit.ThreadGroupCountZ = 1;

	dispatch.simulate.ThreadGroupCountX = bucketize(num_alive_particles_in_last_frame + num_new_particles, PARTICLES_SIMULATE_BLOCK_SIZE);
	dispatch.simulate.ThreadGroupCountY = 1;
	dispatch.simulate.ThreadGroupCountZ = 1;

	c.num_alive_particles_this_frame = num_alive_particles_in_last_frame;
	draw.arguments.InstanceCount = 0;
	draw.arguments.IndexCountPerInstance = cb.index_count;
	draw.arguments.StartIndexLocation = cb.start_index;
	draw.arguments.BaseVertexLocation = cb.base_vertex;
	c.new_particles = num_new_particles;

	dispatch_info[0] = dispatch;
	draw_info[0] = draw;
	counters[0] = c;
}
