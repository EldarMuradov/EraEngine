#include "particles_rs.hlsli"

StructuredBuffer<particle_data> particles			: register(t0, space1);
StructuredBuffer<uint> alive_list					: register(t1, space1);

VSOutput main(VSInput IN, uint instance_id	: SV_InstanceID)
{
	uint index = alive_list[instance_id];
	return vertex_shader(IN, particles, index);
}
