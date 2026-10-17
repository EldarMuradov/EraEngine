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
		enum PresentMode : uint8
		{
			PRESENT_TO_MAIN_WINDOW = 0,
			PRESENT_TO_IMGUI_WINDOW
		};

		RendererHolderRootComponent() = default;
		RendererHolderRootComponent(ref<Entity::EcsData> _data);

		RendererHolderRootComponent(const RendererHolderRootComponent& other) noexcept = default;
		RendererHolderRootComponent(RendererHolderRootComponent&& other) noexcept = default;
		RendererHolderRootComponent& operator=(const RendererHolderRootComponent& other) noexcept;
		RendererHolderRootComponent& operator=(RendererHolderRootComponent&& other) noexcept;

		~RendererHolderRootComponent() override;

		MainRenderer* get_renderer() const;

		RenderCamera camera;

		DirectionalLight sun;
		PBREnvironment environment;
		LDRRenderPass* ldr_render_pass = nullptr;

		uint32_t width = 1920;
		uint32_t height = 1080;

		PresentMode present_mode = PRESENT_TO_MAIN_WINDOW;

		float timestep_scale = 1.0f;

		ERA_VIRTUAL_REFLECT(Component)

	private:
		MainRenderer* renderer = nullptr;

		UserInput* active_render_input = nullptr;

		friend class RenderSystem;
		friend class InputSystem;
	};

}