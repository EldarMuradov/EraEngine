#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2023 Nicholas Frechette, Cody Jones, and sjson-cpp contributors
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#include "sjson/version.h"

#if defined(__cplusplus) && __cplusplus >= 202002L
	#include <bit>
#elif defined(_MSVC_LANG) && _MSVC_LANG >= 202002L
	#include <bit>
#endif

namespace sjson
{
	SJSON_CPP_IMPL_VERSION_NAMESPACE_BEGIN

	namespace sjson_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// C++20 introduced std::bit_cast which is safer than reinterpret_cast
		//////////////////////////////////////////////////////////////////////////

	#if defined(__cplusplus) && __cplusplus >= 202002L
		using std::bit_cast;
	#elif defined(_MSVC_LANG) && _MSVC_LANG >= 202002L
		using std::bit_cast;
	#else
		template<class dest_type_t, class src_type_t>
		constexpr dest_type_t bit_cast(src_type_t input) noexcept
		{
			return reinterpret_cast<dest_type_t>(input);
		}
	#endif
	}

	SJSON_CPP_IMPL_VERSION_NAMESPACE_END
}
