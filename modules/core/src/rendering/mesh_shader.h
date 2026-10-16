// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include "rendering/render_pass.h"

namespace era_engine
{
	void initialize_mesh_shader();
	void test_render_mesh_shader(TransparentRenderPass* ldr_render_pass, float dt);
}