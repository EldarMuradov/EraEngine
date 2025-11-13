#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2018 Nicholas Frechette, Cody Jones, and sjson-cpp contributors
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

//////////////////////////////////////////////////////////////////////////
// The Android NDK r10 and possibly earlier used a STD lib that poorly supported C++11.
// As such, it is necessary to polyfill a few missing things.
//////////////////////////////////////////////////////////////////////////
#if defined(__GNUG__) && defined(__ANDROID__) && __GNUC__ < 5
	#define SJSON_CPP_IMPL_POLYFILL_STD
#endif

#if defined(SJSON_CPP_IMPL_POLYFILL_STD)
	#include <stdlib.h>
#else
	#include <cstdlib>
#endif

namespace sjson
{
	SJSON_CPP_IMPL_VERSION_NAMESPACE_BEGIN

	namespace sjson_impl
	{
#if defined(SJSON_CPP_IMPL_POLYFILL_STD)
		// We need to polyfill these, bring the C version into our namespace
		using ::strtoull;
		using ::strtoll;
		using ::strtof;
#else
		// These properly exist in the C++ version, use them
		using std::strtoull;
		using std::strtoll;
		using std::strtof;
#endif
	}

	SJSON_CPP_IMPL_VERSION_NAMESPACE_END
}
