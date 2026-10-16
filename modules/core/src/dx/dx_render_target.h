// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "dx/dx.h"
#include "dx/dx_descriptor.h"
#include "dx/dx_texture.h"

namespace era_engine
{
	struct DXTexture;

	struct DXRenderTarget
	{
		DXRenderTarget(uint32 width, uint32 height)
		{
			viewport = { 0.f, 0.f, (float)width, (float)height, 0.f, 1.f };
		}

		NODISCARD DXRenderTarget& color_attachment(const ref<DXTexture>& attachment, DXRTVDescriptorHandle use_if_null = CD3DX12_DEFAULT())
		{
			ASSERT(push_index < arraysize(rtv));
			rtv[push_index++] = attachment ? attachment->default_rtv : use_if_null;

			if (attachment)
			{
				// Change numAttachments only, if attachment is valid. This way the invalid attachments at the end are ignored, when the render target is bound to the command list.
				num_attachments = push_index;
			}

			return *this;
		}

		NODISCARD DXRenderTarget& depth_attachment(const ref<DXTexture>& attachment, DXDSVDescriptorHandle use_if_null = CD3DX12_DEFAULT())
		{
			ASSERT(!dsv);
			dsv = attachment ? attachment->default_dsv : use_if_null;
			return *this;
		}

		uint32 num_attachments = 0;
		D3D12_VIEWPORT viewport;

		DXRTVDescriptorHandle rtv[8];
		DXDSVDescriptorHandle dsv = CD3DX12_DEFAULT();

	private:
		uint32 push_index = 0;
	};
}