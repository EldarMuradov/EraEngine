#include "cs.hlsli"
#include "post_processing_rs.hlsli"
#include "camera.hlsli"
#include "random.hlsli"

// This file is based on https://github.com/scanberg/hbao

ConstantBuffer<HBAOCB> cb				: register(b0);
ConstantBuffer<CameraCB> camera		: register(b1);

Texture2D<float> depth_buffer			: register(t0);

RWTexture2D<float> result_texture		: register(u0);

SamplerState linear_sampler				: register(s0);

static float3 position_at(float2 uv)
{
	float depth = depth_buffer.SampleLevel(linear_sampler, uv, cb.depth_buffer_mip_level);
	float3 pos = camera.restore_view_space_position_eye_depth(uv, depth);
	return pos;
}

static float length2(float3 v)
{
	return dot(v, v);
}

static float3 min_diff(float3 P, float3 right, float3 left)
{
	float3 V1 = right - P;
	float3 V2 = P - left;
	return (length2(V1) < length2(V2)) ? V1 : V2;
}

static float2 rotate_directions(float2 dir, float2 cos_sin)
{
	return float2(dir.x * cos_sin.x - dir.y * cos_sin.y,
		dir.x * cos_sin.y + dir.y * cos_sin.x);
}

static float2 snap_uv_offset(float2 uv, float2 dims, float2 inv_dims)
{
	return round(uv * dims) * inv_dims;
}

static float tan_to_sin(float x)
{
	return x * rsqrt(x * x + 1.f);
}

static float inv_length(float2 v)
{
	return rsqrt(dot(v, v));
}

static const float tan_bias = tan(30.f * M_PI / 180.f);
static float biased_tangent(float3 v)
{
	return v.z * inv_length(v.xy) + tan_bias;
}

static float tangent(float3 P, float3 S)
{
	return -(P.z - S.z) * inv_length(S.xy - P.xy);
}

static float falloff(float d2, float neg_inv_radius2)
{
	return d2 * neg_inv_radius2 + 1.f;
}

static float horizon_occlusion(float2 center_uv, float2 delta_uv, float3 P, 
	float3 d_pdu, float3 d_pdv, float jitter, float num_samples,
	float radius2, float neg_inv_radius2,
	float2 dims, float2 inv_dims)
{
	// Offset the first coord with some noise.
	float2 uv = center_uv + snap_uv_offset(jitter * delta_uv, dims, inv_dims);
	delta_uv = snap_uv_offset(delta_uv, dims, inv_dims);

	// Calculate the tangent vector.
	float3 T = delta_uv.x * d_pdu + delta_uv.y * d_pdv;

	// Get the angle of the tangent vector from the viewspace axis.
	float tan_h = biased_tangent(T);
	float sin_h = tan_to_sin(tan_h);

	float ao = 0.f;

	// Sample to find the maximum angle
	for (float s = 0; s < num_samples; ++s)
	{
		uv += delta_uv;
		float3 S = position_at(uv);
		float tan_s = tangent(P, S);
		float d2 = length2(S - P);

		// Is the sample within the radius and the angle greater?
		if (d2 < radius2 && tan_s > tan_h)
		{
			float sin_s = tan_to_sin(tan_s);
			// Apply falloff based on the distance.
			ao += falloff(d2, neg_inv_radius2) * (sin_s - sin_h);

			tan_h = tan_s;
			sin_h = sin_s;
		}
	}

	return ao;
}

[RootSignature(HBAO_RS)]
[numthreads(POST_PROCESSING_BLOCK_SIZE, POST_PROCESSING_BLOCK_SIZE, 1)]
void main(CSInput IN)
{
	if (IN.dispatch_thread_id.x >= cb.screen_width || IN.dispatch_thread_id.y >= cb.screen_height) 
	{
		return;
	}

	float2 screen_dims = float2(cb.screen_width, cb.screen_height);
	float2 inv_screen_dims = rcp(screen_dims);

	float2 center_uv = (IN.dispatch_thread_id.xy + float2(0.5f, 0.5f)) * inv_screen_dims;
	float3 center_pos = position_at(center_uv);

	if (center_pos.z < -1000.f)
	{
		result_texture[IN.dispatch_thread_id.xy] = 1.f;
		return;
	}

	float3 right_pos = position_at(center_uv + float2(inv_screen_dims.x, 0.f));
	float3 left_pos = position_at(center_uv - float2(inv_screen_dims.x, 0.f));
	float3 top_pos = position_at(center_uv - float2(0.f, inv_screen_dims.y));
	float3 bottom_pos = position_at(center_uv + float2(0.f, inv_screen_dims.y));

	float3 d_pdu = min_diff(center_pos, right_pos, left_pos);
	float3 d_pdv = min_diff(center_pos, bottom_pos, top_pos) * (screen_dims.y * inv_screen_dims.x);

	float2 ray_radius_uv = 0.5f * cb.radius * camera.proj._m00_m11 / -center_pos.z;

	float ray_radius_pix = ray_radius_uv.x * screen_dims.x;

	float ao = 1.f;

	if (ray_radius_pix > 1.f)
	{
		ao = 0.f;

		float jitter = random(center_uv * 9523.f + cb.seed) * 0.4f;

		// Compute steps.
		float num_steps = min((float)cb.max_num_steps_per_ray, ray_radius_pix);

		// Divide by Ns+1 so that the farthest samples are not fully attenuated.
		float step_size_pix = ray_radius_pix / (num_steps + 1);

		// Clamp numSteps if it is greater than the max kernel footprint.
		float max_num_steps = cb.max_num_steps_per_ray / step_size_pix;
		if (max_num_steps < num_steps)
		{
			// Use dithering to avoid AO discontinuities.
			num_steps = floor(max_num_steps + jitter);
			num_steps = max(num_steps, 1);
			step_size_pix = cb.max_num_steps_per_ray / num_steps;
		}

		float2 step_size_uv = step_size_pix * inv_screen_dims;


		float angle = random(center_uv * 5416.f + cb.seed) * M_PI * 2.f;
		float2 random_rotation = float2(cos(angle), sin(angle)); // TODO: Maybe get this from texture.

		// Apply noise to initial rotation.
		float2 dir = rotate_directions(float2(1.f, 0.f), random_rotation);

		float2 ray_delta_rotation = cb.ray_delta_rotation;

		float radius2 = cb.radius * cb.radius;
		float neg_inv_radius2 = rcp(radius2);

		for (float d = 0; d < (float)cb.num_rays; d += 1.f)
		{
			float2 delta_uv = dir * step_size_uv;

			// Sample the pixels along the direction.
			ao += horizon_occlusion(
				center_uv, delta_uv,
				center_pos,
				d_pdu, d_pdv,
				jitter, num_steps,
				radius2, neg_inv_radius2,
				screen_dims, inv_screen_dims);

			dir = rotate_directions(dir, ray_delta_rotation);
		}

		// Average the results and produce the final AO.
		ao = saturate(1.f - ao / cb.num_rays * cb.strength);
	}

	result_texture[IN.dispatch_thread_id.xy] = ao;
}
