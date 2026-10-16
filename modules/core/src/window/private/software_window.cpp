// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "window/software_window.h"

namespace era_engine
{
	SoftwareWindow::SoftwareWindow(SoftwareWindow&& o) noexcept
		: Win32Window(std::move(o))
	{
		bitmap_info = o.bitmap_info;
		o.bitmap_info = 0;

		buffer = o.buffer;
		o.buffer = 0;
	}

	bool SoftwareWindow::initialize(const TCHAR* name, uint32 requested_client_width, uint32 requested_client_height,
		uint8* buffer, uint32 num_channels, uint32 buffer_width, uint32 buffer_height)
	{
		ASSERT(num_channels == 1 || num_channels == 3 || num_channels == 4);

		if (!Win32Window::initialize(name, requested_client_width, requested_client_height))
			return false;

		if (!bitmap_info)
		{
			bitmap_info = (BITMAPINFO*)malloc(sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 256);
			if (!bitmap_info)
				return false;

			bitmap_info->bmiHeader.biSize = sizeof(bitmap_info->bmiHeader);
			bitmap_info->bmiHeader.biCompression = BI_RGB;
			bitmap_info->bmiHeader.biSizeImage = 0;
			bitmap_info->bmiHeader.biXPelsPerMeter = 0;
			bitmap_info->bmiHeader.biYPelsPerMeter = 0;
			bitmap_info->bmiHeader.biClrUsed = 0;
			bitmap_info->bmiHeader.biClrImportant = 0;

			for (int i = 0; i < 256; ++i)
			{
				bitmap_info->bmiColors[i].rgbRed = i;
				bitmap_info->bmiColors[i].rgbGreen = i;
				bitmap_info->bmiColors[i].rgbBlue = i;
				bitmap_info->bmiColors[i].rgbReserved = 0;
			}
		}

		bitmap_info->bmiHeader.biWidth = buffer_width == 0 ? client_width : buffer_width;
		bitmap_info->bmiHeader.biHeight = buffer_height == 0 ? client_height : buffer_height;
		bitmap_info->bmiHeader.biPlanes = 1;
		bitmap_info->bmiHeader.biBitCount = 8 * num_channels;

		blit_x = 0;
		blit_y = 0;
		blit_width = buffer_width == 0 ? client_width : buffer_width;
		blit_height = buffer_height == 0 ? client_height : buffer_height;

		this->buffer = buffer;

		return true;
	}

	void SoftwareWindow::change_blit_region(uint32 x, uint32 y, uint32 width, uint32 height)
	{
		blit_x = x;
		blit_y = y;
		blit_width = width;
		blit_height = height;
	}

	void SoftwareWindow::shutdown()
	{
		if (bitmap_info)
		{
			free(bitmap_info);
			bitmap_info = 0;
		}
		Win32Window::shutdown();
	}

	SoftwareWindow::~SoftwareWindow()
	{
		shutdown();
	}

	void SoftwareWindow::swap_buffers()
	{
		RECT r = { 0, 0, (LONG)client_width, (LONG)client_height };
		InvalidateRect(window_handle, &r, TRUE);
		UpdateWindow(window_handle);
	}

}