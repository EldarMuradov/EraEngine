#include "rendering/ecs/renderer_holder_root_component.h"

#include "window/dx_window.h"

#include "engine/engine.h"

#include <rttr/registration>

namespace era_engine
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		rttr::registration::class_<RendererHolderRootComponent>("RendererHolderRootComponent")
			.constructor<ref<Entity::EcsData>>();
	}

	RendererHolderRootComponent::RendererHolderRootComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
		renderer_spec spec;
		spec.allowObjectPicking = true;
		spec.allowAO = true;
		spec.allowSSS = true;
		spec.allowSSR = true;
		spec.allowTAA = true;
		spec.allowBloom = true;
		spec.allowDLSS = true;

		renderer = new main_renderer();

		dx_window* window = get_object<dx_window>();
		ASSERT(window != nullptr);

		ldrRenderPass = new ldr_render_pass();

		renderer->initialize(window->colorDepth, window->clientWidth, window->clientHeight, spec);
	}

	RendererHolderRootComponent& era_engine::RendererHolderRootComponent::operator=(const RendererHolderRootComponent& other) noexcept
	{
		if (this != &other)
		{
			camera = other.camera;
			sun = other.sun;
			ldrRenderPass = other.ldrRenderPass;
			environment = other.environment;
		}

		return *this;
	}

	RendererHolderRootComponent& era_engine::RendererHolderRootComponent::operator=(RendererHolderRootComponent&& other) noexcept
	{
		if (this != &other)
		{
			camera = std::move(other.camera);
			sun = std::move(other.sun);
			ldrRenderPass = std::move(other.ldrRenderPass);
			environment = std::move(other.environment);
		}

		return *this;
	}

	RendererHolderRootComponent::~RendererHolderRootComponent()
	{
		delete ldrRenderPass;
		delete renderer;
	}

}