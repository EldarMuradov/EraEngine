#ifndef MATERIAL_HLSLI
#define MATERIAL_HLSLI


static uint32 pack_color(uint32 r, uint32 g, uint32 b, uint32 a)
{
    return ((a & 0xFF) << 24) | ((b & 0xFF) << 16) | ((g & 0xFF) << 8) | (r & 0xFF);
}

static uint32 pack_color(float r, float g, float b, float a)
{
    return pack_color(
        (uint32)clamp(r * 255.f, 0.f, 255.f),
        (uint32)clamp(g * 255.f, 0.f, 255.f),
        (uint32)clamp(b * 255.f, 0.f, 255.f),
        (uint32)clamp(a * 255.f, 0.f, 255.f));
}

static uint32 pack_color(vec4 rgba)
{
    return pack_color(rgba.r, rgba.g, rgba.b, rgba.a);
}

static vec4 unpack_color(uint32 c)
{
    const float mul = 1.f / 255.f;
    float r = (c & 0xFF) * mul;
    float g = ((c >> 8) & 0xFF) * mul;
    float b = ((c >> 16) & 0xFF) * mul;
    float a = ((c >> 24) & 0xFF) * mul;
    return vec4(r, g, b, a);
}

struct TextureAtlasCB
{
	uint32 total_num_cells_num_cols; // Each 16 bit.
	uint32 inv_cols_inv_rows; // Each 16 bit halfs.

	void initialize(uint32 rows, uint32 cols)
	{
		total_num_cells_num_cols = ((rows * cols) << 16) | cols;

#ifndef HLSL
		uint32 inv_cols = half(1.f / cols).h;
		uint32 inv_rows = half(1.f / rows).h;
#else
		uint32 inv_cols = f32tof16(1.f / cols);
		uint32 inv_rows = f32tof16(1.f / rows);
#endif

		inv_cols_inv_rows = (inv_cols << 16) | inv_rows;
	}

	uint32 get_total_num_cells()
	{
		return total_num_cells_num_cols >> 16;
	}

	uint32 get_num_cols()
	{
		return total_num_cells_num_cols & 0xFFFF;
	}

	float get_inv_num_cols()
	{
		uint32 i = inv_cols_inv_rows >> 16;
#ifndef HLSL
		return half((uint16)i);
#else
		return f16tof32(i);
#endif
	}

	float get_inv_num_rows()
	{
		uint32 i = inv_cols_inv_rows;
#ifndef HLSL
		return half((uint16)i);
#else
		return f16tof32(i);
#endif
	}

	uint32 get_x(uint32 i)
	{
		return i % get_num_cols();
	}

	uint32 get_y(uint32 i)
	{
		return i / get_num_cols();
	}
};

#define MATERIAL_USE_ALBEDO_TEXTURE			(1 << 0)
#define MATERIAL_USE_NORMAL_TEXTURE			(1 << 1)
#define MATERIAL_USE_ROUGHNESS_TEXTURE		(1 << 2)
#define MATERIAL_USE_METALLIC_TEXTURE		(1 << 3)
#define MATERIAL_DOUBLE_SIDED				(1 << 4)
#define MATERIAL_USE_32_BIT_INDICES			(1 << 5) // Ugly to put this in the material. Only used in the ray tracing shaders.
#define MATERIAL_USE_EMISSION_TEXTURE		(1 << 6)

struct PBRMaterialCB // 24 bytes.
{
    vec3 emission;      // Since emission can be HDR, we use full precision floats here.
    uint32 albedo_tint;  // RGBA packed into one uint32. Can only be between 0 and 1, which is why we are only using 8 bit per channel.
    uint32 roughness_metallic_flags_translucency; // 8 bit each.
	uint32 normal_map_strength_unused_uv_scale; // 8 bit for normal map strength, 8 bit unused, 16 bit half for uvScale.

    void initialize(vec4 albedo_, vec3 emission_, float roughness_, float metallic_, uint32 flags_, float normal_map_strength_ = 1.f, float translucency_ = 0.f,
		float uv_scale = 1.f)
    {
        emission = emission_;
        albedo_tint = pack_color(albedo_);
        
		roughness_ = clamp(roughness_, 0.f, 1.f);
		metallic_ = clamp(metallic_, 0.f, 1.f);
		translucency_ = clamp(translucency_, 0.f, 1.f);
		normal_map_strength_ = clamp(normal_map_strength_, 0.f, 1.f);

		roughness_metallic_flags_translucency =
			((uint32)(roughness_ * 0xFF) << 24) |
			((uint32)(metallic_ * 0xFF) << 16) |
			(flags_ << 8) |
			((uint32)(translucency_ * 0xFF) << 0);

#ifndef HLSL
		uint32 uv_scale_half = half(uv_scale).h;
#else
		uint32 uv_scale_half = f32tof16(uv_scale);
#endif
		
		normal_map_strength_unused_uv_scale =
			((uint32)(normal_map_strength_ * 0xFF) << 24) |
			0 |
			uv_scale_half;
    }

#ifndef HLSL
    PBRMaterialCB() {}

    PBRMaterialCB(vec4 albedo_, vec3 emission_, float roughness_, float metallic_, uint32 flags_, float normal_map_strength_ = 1.f, float translucency_ = 0.f, float uv_scale_ = 1.f)
    {
        initialize(albedo_, emission_, roughness_, metallic_, flags_, normal_map_strength_, translucency_, uv_scale_);
    }
#endif

    vec4 get_albedo()
    {
        return unpack_color(albedo_tint);
    }

    float get_roughness_override()
    {
        return ((roughness_metallic_flags_translucency >> 24) & 0xFF) * (1.f / 255.f);
    }

    float get_metallic_override()
    {
        return ((roughness_metallic_flags_translucency >> 16) & 0xFF) * (1.f / 255.f);
    }

	float get_normal_map_strength()
	{
		return ((normal_map_strength_unused_uv_scale >> 24) & 0xFF) * (1.f / 255.f);
	}

	float uv_scale()
	{
		uint32 uv_scale_half = normal_map_strength_unused_uv_scale & 0xFFFF;
#ifndef HLSL
		return half((uint16)uv_scale_half);
#else
		return f16tof32(uv_scale_half);
#endif
	}

	float get_translucency()
	{
		return ((roughness_metallic_flags_translucency >> 0) & 0xFF) * (1.f / 255.f);
	}

	uint32 get_flags()
	{
		return (roughness_metallic_flags_translucency >> 8) & 0xFF;
	}
};

struct PBRDecalCB
{
	vec3 position;
	uint32 albedo_tint;			// RGBA packed into one uint32.
	vec3 right;					// Scaled by half dimension.
	uint32 roughness_override_metallic_override;
	vec3 up;					// Scaled by half dimension.
	uint32 viewport_xy;			// Top left corner packed into 16 bits each.
	vec3 forward;				// Scaled by half dimension.
	uint32 viewport_scale;       // Width and height packed into 16 bits each.

	void initialize(vec3 position_, vec3 right_, vec3 up_, vec3 forward_, vec4 albedo_, float roughness_, float metallic_, vec4 viewport_)
	{
		position = position_;
		right = right_;
		up = up_;
		forward = forward_;

		albedo_tint = pack_color(albedo_);

		uint32 r = (uint32)(roughness_ * 0xFFFF);
		uint32 m = (uint32)(metallic_ * 0xFFFF);
		roughness_override_metallic_override = (r << 16) | m;

		uint32 x = (uint32)(viewport_.x * 0xFFFF);
		uint32 y = (uint32)(viewport_.y * 0xFFFF);
		uint32 w = (uint32)(viewport_.z * 0xFFFF);
		uint32 h = (uint32)(viewport_.w * 0xFFFF);
		viewport_xy = (x << 16) | y;
		viewport_scale = (w << 16) | h;
	}

#ifndef HLSL
	PBRDecalCB() {}

	PBRDecalCB(vec3 position_, vec3 right_, vec3 up_, vec3 forward_, vec4 albedo_, float roughness_, float metallic_, vec4 viewport_)
	{
		initialize(position_, right_, up_, forward_, albedo_, roughness_, metallic_, viewport_);
	}
#endif

	vec4 get_albedo()
	{
		return unpack_color(albedo_tint);
	}

	float get_roughness_override()
	{
		return (roughness_override_metallic_override >> 16) / (float)0xFFFF;
	}

	float get_metallic_override()
	{
		return (roughness_override_metallic_override & 0xFFFF) / (float)0xFFFF;
	}

	vec4 get_viewport()
	{
		float x = (viewport_xy >> 16) / (float)0xFFFF;
		float y = (viewport_xy & 0xFFFF) / (float)0xFFFF;
		float w = (viewport_scale >> 16) / (float)0xFFFF;
		float h = (viewport_scale & 0xFFFF) / (float)0xFFFF;
		return vec4(x, y, w, h);
	}
};


// Alpha-blending performs the following operation: final = alpha * src + (1 - alpha) * dest.
// The factor on the destination-color is correct, and so is the factor on the diffuse part of the source-color.
// However emission and specular light should not be modulated by alpha, since these components are not affected by transparency.
// To counteract the hardware alpha-blending for these components, we pre-divide them by alpha.

static vec4 merge_alpha_blended(vec3 diffuse, vec3 specular, vec3 emission, float alpha)
{
	return vec4((emission + specular) * (1.f / max(alpha, 1e-5f)) + diffuse, alpha);
}

#endif