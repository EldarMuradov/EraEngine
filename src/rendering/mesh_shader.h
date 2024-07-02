// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include "render_pass.h"

namespace era_engine
{
	void initializeMeshShader();
	void testRenderMeshShader(transparent_render_pass* ldrRenderPass, float dt);
}