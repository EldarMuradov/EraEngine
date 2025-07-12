#pragma once

#include "core_api.h"

#include "rendering/main_renderer.h"
#include "rendering/light_source.h"
#include "rendering/pbr.h"
#include "rendering/pbr_environment.h"

#include "core/camera.h"

#include "ecs/component.h"

namespace era_engine
{
	class UserInput;

	class ERA_CORE_API RendererHolderRootComponent final : public Component
	{
	public:
		RendererHolderRootComponent() = default;
		RendererHolderRootComponent(ref<Entity::EcsData> _data);

		RendererHolderRootComponent(const RendererHolderRootComponent& other) noexcept = default;
		RendererHolderRootComponent(RendererHolderRootComponent&& other) noexcept = default;
		RendererHolderRootComponent& operator=(const RendererHolderRootComponent& other) noexcept;
		RendererHolderRootComponent& operator=(RendererHolderRootComponent&& other) noexcept;

		~RendererHolderRootComponent() override;

		render_camera camera;

		directional_light sun;
		pbr_environment environment;
		ldr_render_pass* ldrRenderPass = nullptr;

		uint32_t width = 1920;
		uint32_t height = 1080;

		float timestep_scale = 1.0f;

		ERA_VIRTUAL_REFLECT(Component)

	private:
		main_renderer* renderer = nullptr;

		UserInput* active_render_input = nullptr;

		friend class RenderSystem;
		friend class InputSystem;
	};

}