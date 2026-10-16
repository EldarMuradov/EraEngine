#include "rendering/ecs/renderer_holder_root_component.h"
#include "ecs/serialization/component_registry.h"
#include "ecs/serialization/serialization_meta.h"

#include "window/dx_window.h"

#include "engine/engine.h"

#include <rttr/registration>

namespace era_engine
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		rttr::registration::class_<RendererHolderRootComponent>("RendererHolderRootComponent")(rttr::metadata(SerializationMeta::NOT_SERIALIZABLE_CLASS, true))
			.constructor<ref<Entity::EcsData>>();
		ComponentRegistry::register_component<RendererHolderRootComponent>();
	}

	RendererHolderRootComponent::RendererHolderRootComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
		RendererSpec spec;
		spec.allow_object_picking = true;
		spec.allow_ao = true;
		spec.allow_sss = true;
		spec.allow_ssr = true;
		spec.allow_taa = true;
		spec.allow_bloom = true;
		spec.allow_dlss = true;

		renderer = new MainRenderer();

		DXWindow* window = get_object<DXWindow>();
		ASSERT(window != nullptr);

		ldr_render_pass = new LDRRenderPass();

		renderer->initialize(window->color_depth, window->client_width, window->client_height, spec);
	}

	RendererHolderRootComponent& RendererHolderRootComponent::operator=(const RendererHolderRootComponent& other) noexcept
	{
		if (this != &other)
		{
			camera = other.camera;
			sun = other.sun;
			ldr_render_pass = other.ldr_render_pass;
			environment = other.environment;
		}

		return *this;
	}

	RendererHolderRootComponent& RendererHolderRootComponent::operator=(RendererHolderRootComponent&& other) noexcept
	{
		if (this != &other)
		{
			camera = std::move(other.camera);
			sun = std::move(other.sun);
			ldr_render_pass = std::move(other.ldr_render_pass);
			environment = std::move(other.environment);
		}

		return *this;
	}

	RendererHolderRootComponent::~RendererHolderRootComponent()
	{
		delete ldr_render_pass;
		delete renderer;
	}

	MainRenderer* RendererHolderRootComponent::get_renderer() const
	{
		return renderer;
	}

}