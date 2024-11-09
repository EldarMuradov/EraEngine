#pragma once

#include "core/math.h"
#include "core/imgui.h"

#include "ecs/entity.h"

namespace era_engine
{
	class EntityEditorUtils final
	{
		EntityEditorUtils() = delete;
	public:

		static void edit_entity(ref<World> world, const Entity::Handle handle);

		static void show_vector3(const char* label, vec3& vector, float wdt);
		static void draw_float_wrap(float wdt, const char* label, float* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", const ImGuiSliderFlags flags = 0);
	};
}