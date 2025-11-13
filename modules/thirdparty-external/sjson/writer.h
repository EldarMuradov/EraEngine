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

// This define allows external libraries using sjson-cpp to detect if it has already be included as a dependency
#if !defined(SJSON_CPP_WRITER)
	#define SJSON_CPP_WRITER
#endif

#include "sjson/error.h"
#include "sjson/version.h"

#include <cstdio>
#include <cstdint>
#include <cinttypes>
#include <cmath>
#include <cstring>
#include <functional>
#include <limits>
#include <type_traits>

namespace sjson
{
	SJSON_CPP_IMPL_VERSION_NAMESPACE_BEGIN

	// TODO: Cleanup the locking stuff, wrap it in #ifdef to strip when asserts are disabled

	class Writer;
	class ArrayWriter;
	class ObjectWriter;

	// TODO: Make this an argument to the writer. For now we assume that SJSON generated files
	// can be shared between various OS and having the most conservative line ending is safer.
	constexpr const char* k_line_terminator = "\r\n";

	class StreamWriter
	{
	public:
		virtual ~StreamWriter() = default;

		virtual void write(const void* buffer, size_t buffer_size) = 0;

		inline void write(const char* str) { write(str, std::strlen(str)); }
	};

	class FileStreamWriter final : public StreamWriter
	{
	public:
		explicit FileStreamWriter(std::FILE* file)
			: m_file(file)
		{}

		virtual void write(const void* buffer, size_t buffer_size) override
		{
			fwrite(buffer, sizeof(char), buffer_size, m_file);
		}

	private:
		std::FILE* m_file;
	};

	// A lambda that does not capture anything is equivalent to a static function
	// and calling a function with it as an argument is equivalent to passing a function pointer.
	// Of course, a pointer can safely and automatically coerce to 'bool' and as such
	// a clean signature like this does not work: void insert(const char*, std::function<void(ObjectWriter&)) foo)
	// The compiler finds it ambiguous with `void insert(const char*, bool)` and fails to compile.
	//
	// To resolve this, we use SFINAE template logic to deduce a real type if we pass a function
	// and if we pass bool, the type will not exist and the function is stripped.
	template<typename F, typename ArgType>
	struct HavingArgument : std::enable_if<std::is_constructible<std::function<void(ArgType)>, F>::value> {};

	class ArrayWriter
	{
	public:
		void push(const char* value);
		void push(bool value);
		void push(double value);
		void push(float value) { push(double(value)); }
		void push(int8_t value) { push_signed_integer(int64_t(value)); }
		void push(uint8_t value) { push_unsigned_integer(uint64_t(value)); }
		void push(int16_t value) { push_signed_integer(int64_t(value)); }
		void push(uint16_t value) { push_unsigned_integer(uint64_t(value)); }
		void push(int32_t value) { push_signed_integer(int64_t(value)); }
		void push(uint32_t value) { push_unsigned_integer(uint64_t(value)); }
		void push(int64_t value) { push_signed_integer(value); }
		void push(uint64_t value) { push_unsigned_integer(value); }

		template<typename F, typename HavingArgument<F, ObjectWriter&>::type* requirement = nullptr>
		void push(F writer_fun);

		template<typename F, typename HavingArgument<F, ArrayWriter&>::type* requirement = nullptr>
		void push(F writer_fun);

		// TODO: Introduce a newline type
		void push_newline();

	private:
		ArrayWriter(StreamWriter& stream_writer, uint32_t indent_level);

		ArrayWriter(const ArrayWriter&) = delete;
		ArrayWriter& operator=(const ArrayWriter&) = delete;

		void push_signed_integer(int64_t value);
		void push_unsigned_integer(uint64_t value);
		void write_indentation();

		StreamWriter& m_stream_writer;
		uint32_t m_indent_level;
		bool m_is_empty;
		bool m_is_locked;
		bool m_is_newline;

		friend ObjectWriter;
	};

	class ObjectWriter
	{
	public:
		void insert(const char* key, const char* value);
		void insert(const char* key, bool value);
		void insert(const char* key, double value);
		void insert(const char* key, float value) { insert(key, double(value)); }
		void insert(const char* key, int8_t value) { insert_signed_integer(key, int64_t(value)); }
		void insert(const char* key, uint8_t value) { insert_unsigned_integer(key, uint64_t(value)); }
		void insert(const char* key, int16_t value) { insert_signed_integer(key, int64_t(value)); }
		void insert(const char* key, uint16_t value) { insert_unsigned_integer(key, uint64_t(value)); }
		void insert(const char* key, int32_t value) { insert_signed_integer(key, int64_t(value)); }
		void insert(const char* key, uint32_t value) { insert_unsigned_integer(key, uint64_t(value)); }
		void insert(const char* key, int64_t value) { insert_signed_integer(key, int64_t(value)); }
		void insert(const char* key, uint64_t value) { insert_unsigned_integer(key, uint64_t(value)); }

		template<typename F, typename HavingArgument<F, ObjectWriter&>::type* requirement = nullptr>
		void insert(const char* key, F writer_fun);

		template<typename F, typename HavingArgument<F, ArrayWriter&>::type* requirement = nullptr>
		void insert(const char* key, F writer_fun);

		void insert_newline();

		// Implement operator[] for convenience
		class ValueRef final
		{
		public:
			ValueRef(ValueRef&& other) noexcept;
			~ValueRef();

			void operator=(const char* value);
			void operator=(bool value);
			void operator=(double value);
			void operator=(float value) { *this = double(value); }
			void operator=(int8_t value) { assign_signed_integer(int64_t(value)); }
			void operator=(uint8_t value) { assign_unsigned_integer(uint64_t(value)); }
			void operator=(int16_t value) { assign_signed_integer(int64_t(value)); }
			void operator=(uint16_t value) { assign_unsigned_integer(uint64_t(value)); }
			void operator=(int32_t value) { assign_signed_integer(int64_t(value)); }
			void operator=(uint32_t value) { assign_unsigned_integer(uint64_t(value)); }
			void operator=(int64_t value) { assign_signed_integer(int64_t(value)); }
			void operator=(uint64_t value) { assign_unsigned_integer(uint64_t(value)); }

			template<typename F, typename HavingArgument<F, ObjectWriter&>::type* requirement = nullptr>
			void operator=(F writer_fun);

			template<typename F, typename HavingArgument<F, ArrayWriter&>::type* requirement = nullptr>
			void operator=(F writer_fun);

		private:
			ValueRef(ObjectWriter& object_writer, const char* key);

			ValueRef(const ValueRef&) = delete;
			ValueRef& operator=(const ValueRef&) = delete;

			void assign_signed_integer(int64_t value);
			void assign_unsigned_integer(uint64_t value);

			ObjectWriter* m_object_writer;
			bool m_is_empty;
			bool m_is_locked;

			friend ObjectWriter;
		};

		ValueRef operator[](const char* key) { return ValueRef(*this, key); }

	protected:
		inline ObjectWriter(StreamWriter& stream_writer, uint32_t indent_level);

		ObjectWriter(const ObjectWriter&) = delete;
		ObjectWriter& operator=(const ObjectWriter&) = delete;

		void insert_signed_integer(const char* key, int64_t value);
		void insert_unsigned_integer(const char* key, uint64_t value);
		void write_indentation();

	private:
		StreamWriter& m_stream_writer;
		uint32_t m_indent_level;
		bool m_is_locked;
		bool m_has_live_value_ref;

		friend ArrayWriter;
	};

	class Writer : public ObjectWriter
	{
	public:
		explicit Writer(StreamWriter& stream_writer);

	private:
		Writer(const Writer&) = delete;
		Writer& operator=(const Writer&) = delete;
	};

	//////////////////////////////////////////////////////////////////////////

	inline ObjectWriter::ObjectWriter(StreamWriter& stream_writer, uint32_t indent_level)
		: m_stream_writer(stream_writer)
		, m_indent_level(indent_level)
		, m_is_locked(false)
		, m_has_live_value_ref(false)
	{}

	inline void ObjectWriter::insert(const char* key, const char* value)
	{
		SJSON_CPP_ASSERT(!m_is_locked, "Cannot insert SJSON value in locked object");
		SJSON_CPP_ASSERT(!m_has_live_value_ref, "Cannot insert SJSON value in object when it has a live ValueRef");

		write_indentation();

		m_stream_writer.write(key);
		m_stream_writer.write(" = \"");
		m_stream_writer.write(value);
		m_stream_writer.write("\"");
		m_stream_writer.write(k_line_terminator);
	}

	inline void ObjectWriter::insert(const char* key, bool value)
	{
		SJSON_CPP_ASSERT(!m_is_locked, "Cannot insert SJSON value in locked object");
		SJSON_CPP_ASSERT(!m_has_live_value_ref, "Cannot insert SJSON value in object when it has a live ValueRef");

		write_indentation();

		m_stream_writer.write(key);
		m_stream_writer.write(" = ");

		char buffer[256];
		int length = snprintf(buffer, sizeof(buffer), "%s%s", value ? "true" : "false", k_line_terminator);
		SJSON_CPP_ASSERT(length > 0 && length < static_cast<int>(sizeof(buffer)), "Failed to insert SJSON value: [" SJSON_ASSERT_STRING_FORMAT_SPECIFIER " = " SJSON_ASSERT_STRING_FORMAT_SPECIFIER "]", key, value);
		m_stream_writer.write(buffer, static_cast<size_t>(length));
	}

	inline void ObjectWriter::insert(const char* key, double value)
	{
		SJSON_CPP_ASSERT(!m_is_locked, "Cannot insert SJSON value in locked object");
		SJSON_CPP_ASSERT(!m_has_live_value_ref, "Cannot insert SJSON value in object when it has a live ValueRef");

		write_indentation();

		m_stream_writer.write(key);
		m_stream_writer.write(" = ");

		if (std::isnan(value))
		{
			m_stream_writer.write("\"nan\"");
			m_stream_writer.write(k_line_terminator);
		}
		else if (std::isinf(value))
		{
			m_stream_writer.write("\"");

			if (value < 0.0)
				m_stream_writer.write("-");

			m_stream_writer.write("inf\"");
			m_stream_writer.write(k_line_terminator);
		}
		else
		{
			char buffer[256];
			int length = snprintf(buffer, sizeof(buffer), "%.17g%s", value, k_line_terminator);
			SJSON_CPP_ASSERT(length > 0 && length < static_cast<int>(sizeof(buffer)), "Failed to insert SJSON value: [" SJSON_ASSERT_STRING_FORMAT_SPECIFIER " = %.17g]", key, value);
			m_stream_writer.write(buffer, static_cast<size_t>(length));
		}
	}

	inline void ObjectWriter::insert_signed_integer(const char* key, int64_t value)
	{
		SJSON_CPP_ASSERT(!m_is_locked, "Cannot insert SJSON value in locked object");
		SJSON_CPP_ASSERT(!m_has_live_value_ref, "Cannot insert SJSON value in object when it has a live ValueRef");

		write_indentation();

		m_stream_writer.write(key);
		m_stream_writer.write(" = ");

		char buffer[256];
		int length = snprintf(buffer, sizeof(buffer), "%" PRId64 "%s", value, k_line_terminator);
		SJSON_CPP_ASSERT(length > 0 && length < static_cast<int>(sizeof(buffer)), "Failed to insert SJSON value: [" SJSON_ASSERT_STRING_FORMAT_SPECIFIER " = %lld]", key, value);
		m_stream_writer.write(buffer, static_cast<size_t>(length));
	}

	inline void ObjectWriter::insert_unsigned_integer(const char* key, uint64_t value)
	{
		SJSON_CPP_ASSERT(!m_is_locked, "Cannot insert SJSON value in locked object");
		SJSON_CPP_ASSERT(!m_has_live_value_ref, "Cannot insert SJSON value in object when it has a live ValueRef");

		write_indentation();

		m_stream_writer.write(key);
		m_stream_writer.write(" = ");

		char buffer[256];
		int length = snprintf(buffer, sizeof(buffer), "%" PRIu64 "%s", value, k_line_terminator);
		SJSON_CPP_ASSERT(length > 0 && length < static_cast<int>(sizeof(buffer)), "Failed to insert SJSON value: [" SJSON_ASSERT_STRING_FORMAT_SPECIFIER " = %llu]", key, value);
		m_stream_writer.write(buffer, static_cast<size_t>(length));
	}

	template<typename F, typename HavingArgument<F, ObjectWriter&>::type* requirement>
	inline void ObjectWriter::insert(const char* key, F writer_fun)
	{
		SJSON_CPP_ASSERT(!m_is_locked, "Cannot insert SJSON object in locked object");
		SJSON_CPP_ASSERT(!m_has_live_value_ref, "Cannot insert SJSON object in object when it has a live ValueRef");

		write_indentation();

		m_stream_writer.write(key);
		m_stream_writer.write(" = {");
		m_stream_writer.write(k_line_terminator);
		m_is_locked = true;

		ObjectWriter object_writer(m_stream_writer, m_indent_level + 1);
		writer_fun(object_writer);

		m_is_locked = false;
		write_indentation();

		m_stream_writer.write("}");
		m_stream_writer.write(k_line_terminator);
	}

	template<typename F, typename HavingArgument<F, ArrayWriter&>::type* requirement >
	inline void ObjectWriter::insert(const char* key, F writer_fun)
	{
		SJSON_CPP_ASSERT(!m_is_locked, "Cannot insert SJSON array in locked object");
		SJSON_CPP_ASSERT(!m_has_live_value_ref, "Cannot insert SJSON array in object when it has a live ValueRef");

		write_indentation();

		m_stream_writer.write(key);
		m_stream_writer.write(" = [ ");
		m_is_locked = true;

		ArrayWriter array_writer(m_stream_writer, m_indent_level + 1);
		writer_fun(array_writer);

		if (array_writer.m_is_newline)
		{
			write_indentation();
			m_stream_writer.write("]");
			m_stream_writer.write(k_line_terminator);
		}
		else
		{
			m_stream_writer.write(" ]");
			m_stream_writer.write(k_line_terminator);
		}

		m_is_locked = false;
	}

	inline void ObjectWriter::write_indentation()
	{
		for (uint32_t level = 0; level < m_indent_level; ++level)
			m_stream_writer.write("\t");
	}

	inline void ObjectWriter::insert_newline()
	{
		SJSON_CPP_ASSERT(!m_is_locked, "Cannot insert newline in locked object");
		SJSON_CPP_ASSERT(!m_has_live_value_ref, "Cannot insert newline in object when it has a live ValueRef");

		m_stream_writer.write(k_line_terminator);
	}

	inline ObjectWriter::ValueRef::ValueRef(ObjectWriter& object_writer, const char* key)
		: m_object_writer(&object_writer)
		, m_is_empty(true)
		, m_is_locked(false)
	{
		SJSON_CPP_ASSERT(!object_writer.m_is_locked, "Cannot insert SJSON value in locked object");
		SJSON_CPP_ASSERT(!object_writer.m_has_live_value_ref, "Cannot insert SJSON value in object when it has a live ValueRef");

		object_writer.write_indentation();
		object_writer.m_stream_writer.write(key);
		object_writer.m_stream_writer.write(" = ");
		object_writer.m_has_live_value_ref = true;
		object_writer.m_is_locked = true;
	}

	inline ObjectWriter::ValueRef::ValueRef(ValueRef&& other) noexcept
		: m_object_writer(other.m_object_writer)
		, m_is_empty(other.m_is_empty)
		, m_is_locked(other.m_is_locked)
	{
		other.m_object_writer = nullptr;
	}

	inline ObjectWriter::ValueRef::~ValueRef()
	{
		if (m_object_writer != nullptr)
		{
			SJSON_CPP_ASSERT(!m_is_empty, "ValueRef has no associated value");
			SJSON_CPP_ASSERT(!m_is_locked, "ValueRef is locked");
			SJSON_CPP_ASSERT(m_object_writer->m_has_live_value_ref, "Expected a live ValueRef to be present");
			SJSON_CPP_ASSERT(m_object_writer->m_is_locked, "Expected object writer to be locked");

			m_object_writer->m_has_live_value_ref = false;
			m_object_writer->m_is_locked = false;
		}
	}

	inline void ObjectWriter::ValueRef::operator=(const char* value)
	{
		SJSON_CPP_ASSERT(m_is_empty, "Cannot write multiple values within a ValueRef");
		SJSON_CPP_ASSERT(m_object_writer != nullptr, "ValueRef not initialized");
		SJSON_CPP_ASSERT(!m_is_locked, "Cannot assign a value when locked");

		m_object_writer->m_stream_writer.write("\"");
		m_object_writer->m_stream_writer.write(value);
		m_object_writer->m_stream_writer.write("\"");
		m_object_writer->m_stream_writer.write(k_line_terminator);
		m_is_empty = false;
	}

	inline void ObjectWriter::ValueRef::operator=(bool value)
	{
		SJSON_CPP_ASSERT(m_is_empty, "Cannot write multiple values within a ValueRef");
		SJSON_CPP_ASSERT(m_object_writer != nullptr, "ValueRef not initialized");
		SJSON_CPP_ASSERT(!m_is_locked, "Cannot assign a value when locked");

		char buffer[256];
		int length = snprintf(buffer, sizeof(buffer), "%s%s", value ? "true" : "false", k_line_terminator);
		SJSON_CPP_ASSERT(length > 0 && length < static_cast<int>(sizeof(buffer)), "Failed to assign SJSON value: " SJSON_ASSERT_STRING_FORMAT_SPECIFIER, value);
		m_object_writer->m_stream_writer.write(buffer, static_cast<size_t>(length));
		m_is_empty = false;
	}

	inline void ObjectWriter::ValueRef::operator=(double value)
	{
		SJSON_CPP_ASSERT(m_is_empty, "Cannot write multiple values within a ValueRef");
		SJSON_CPP_ASSERT(m_object_writer != nullptr, "ValueRef not initialized");
		SJSON_CPP_ASSERT(!m_is_locked, "Cannot assign a value when locked");

		if (std::isnan(value))
		{
			m_object_writer->m_stream_writer.write("\"nan\"");
			m_object_writer->m_stream_writer.write(k_line_terminator);
		}
		else if (std::isinf(value))
		{
			m_object_writer->m_stream_writer.write("\"");

			if (value < 0.0)
				m_object_writer->m_stream_writer.write("-");

			m_object_writer->m_stream_writer.write("inf\"");
			m_object_writer->m_stream_writer.write(k_line_terminator);
		}
		else
		{
			char buffer[256];
			int length = snprintf(buffer, sizeof(buffer), "%.17g%s", value, k_line_terminator);
			SJSON_CPP_ASSERT(length > 0 && length < static_cast<int>(sizeof(buffer)), "Failed to assign SJSON value: %.17g", value);
			m_object_writer->m_stream_writer.write(buffer, static_cast<size_t>(length));
		}

		m_is_empty = false;
	}

	template<typename F, typename HavingArgument<F, ObjectWriter&>::type* requirement>
	inline void ObjectWriter::ValueRef::operator=(F writer_fun)
	{
		SJSON_CPP_ASSERT(m_is_empty, "Cannot write multiple values within a ValueRef");
		SJSON_CPP_ASSERT(m_object_writer != nullptr, "ValueRef not initialized");
		SJSON_CPP_ASSERT(!m_is_locked, "Cannot assign a value when locked");

		m_object_writer->m_stream_writer.write("{");
		m_object_writer->m_stream_writer.write(k_line_terminator);
		m_is_locked = true;

		ObjectWriter object_writer(m_object_writer->m_stream_writer, m_object_writer->m_indent_level + 1);
		writer_fun(object_writer);

		m_is_locked = false;
		m_object_writer->write_indentation();
		m_object_writer->m_stream_writer.write("}");
		m_object_writer->m_stream_writer.write(k_line_terminator);
		m_is_empty = false;
	}

	template<typename F, typename HavingArgument<F, ArrayWriter&>::type* requirement>
	inline void ObjectWriter::ValueRef::operator=(F writer_fun)
	{
		SJSON_CPP_ASSERT(m_is_empty, "Cannot write multiple values within a ValueRef");
		SJSON_CPP_ASSERT(m_object_writer != nullptr, "ValueRef not initialized");
		SJSON_CPP_ASSERT(!m_is_locked, "Cannot assign a value when locked");

		m_object_writer->m_stream_writer.write("[ ");
		m_is_locked = true;

		ArrayWriter array_writer(m_object_writer->m_stream_writer, m_object_writer->m_indent_level + 1);
		writer_fun(array_writer);

		if (array_writer.m_is_newline)
		{
			m_object_writer->write_indentation();
			m_object_writer->m_stream_writer.write("]");
			m_object_writer->m_stream_writer.write(k_line_terminator);
		}
		else
		{
			m_object_writer->m_stream_writer.write(" ]");
			m_object_writer->m_stream_writer.write(k_line_terminator);
		}

		m_is_locked = false;
		m_is_empty = false;
	}

	inline void ObjectWriter::ValueRef::assign_signed_integer(int64_t value)
	{
		SJSON_CPP_ASSERT(m_is_empty, "Cannot write multiple values within a ValueRef");
		SJSON_CPP_ASSERT(m_object_writer != nullptr, "ValueRef not initialized");
		SJSON_CPP_ASSERT(!m_is_locked, "Cannot assign a value when locked");

		char buffer[256];
		int length = snprintf(buffer, sizeof(buffer), "%" PRId64 "%s", value, k_line_terminator);
		SJSON_CPP_ASSERT(length > 0 && length < static_cast<int>(sizeof(buffer)), "Failed to assign SJSON value: %lld", value);
		m_object_writer->m_stream_writer.write(buffer, static_cast<size_t>(length));
		m_is_empty = false;
	}

	inline void ObjectWriter::ValueRef::assign_unsigned_integer(uint64_t value)
	{
		SJSON_CPP_ASSERT(m_is_empty, "Cannot write multiple values within a ValueRef");
		SJSON_CPP_ASSERT(m_object_writer != nullptr, "ValueRef not initialized");
		SJSON_CPP_ASSERT(!m_is_locked, "Cannot assign a value when locked");

		char buffer[256];
		int length = snprintf(buffer, sizeof(buffer), "%" PRIu64 "%s", value, k_line_terminator);
		SJSON_CPP_ASSERT(length > 0 && length < static_cast<int>(sizeof(buffer)), "Failed to assign SJSON value: %llu", value);
		m_object_writer->m_stream_writer.write(buffer, static_cast<size_t>(length));
		m_is_empty = false;
	}

	//////////////////////////////////////////////////////////////////////////

	inline ArrayWriter::ArrayWriter(StreamWriter& stream_writer, uint32_t indent_level)
		: m_stream_writer(stream_writer)
		, m_indent_level(indent_level)
		, m_is_empty(true)
		, m_is_locked(false)
		, m_is_newline(false)
	{}

	inline void ArrayWriter::push(const char* value)
	{
		SJSON_CPP_ASSERT(!m_is_locked, "Cannot push SJSON value in locked array");

		if (!m_is_empty && !m_is_newline)
			m_stream_writer.write(", ");

		if (m_is_newline)
			write_indentation();

		m_stream_writer.write("\"");
		m_stream_writer.write(value);
		m_stream_writer.write("\"");
		m_is_empty = false;
		m_is_newline = false;
	}

	inline void ArrayWriter::push(bool value)
	{
		SJSON_CPP_ASSERT(!m_is_locked, "Cannot push SJSON value in locked array");

		if (!m_is_empty && !m_is_newline)
			m_stream_writer.write(", ");

		if (m_is_newline)
			write_indentation();

		char buffer[256];
		int length = snprintf(buffer, sizeof(buffer), "%s", value ? "true" : "false");
		SJSON_CPP_ASSERT(length > 0 && length < static_cast<int>(sizeof(buffer)), "Failed to push SJSON value: " SJSON_ASSERT_STRING_FORMAT_SPECIFIER, value);
		m_stream_writer.write(buffer, static_cast<size_t>(length));
		m_is_empty = false;
		m_is_newline = false;
	}

	inline void ArrayWriter::push(double value)
	{
		SJSON_CPP_ASSERT(!m_is_locked, "Cannot push SJSON value in locked array");

		if (!m_is_empty && !m_is_newline)
			m_stream_writer.write(", ");

		if (m_is_newline)
			write_indentation();

		if (std::isnan(value))
		{
			m_stream_writer.write("\"nan\"");
		}
		else if (std::isinf(value))
		{
			m_stream_writer.write("\"");

			if (value < 0.0)
				m_stream_writer.write("-");

			m_stream_writer.write("inf\"");
		}
		else
		{
			char buffer[256];
			int length = snprintf(buffer, sizeof(buffer), "%.17g", value);
			SJSON_CPP_ASSERT(length > 0 && length < static_cast<int>(sizeof(buffer)), "Failed to push SJSON value: %.17g", value);
			m_stream_writer.write(buffer, static_cast<size_t>(length));
		}

		m_is_empty = false;
		m_is_newline = false;
	}

	inline void ArrayWriter::push_signed_integer(int64_t value)
	{
		SJSON_CPP_ASSERT(!m_is_locked, "Cannot push SJSON value in locked array");

		if (!m_is_empty && !m_is_newline)
			m_stream_writer.write(", ");

		if (m_is_newline)
			write_indentation();

		char buffer[256];
		int length = snprintf(buffer, sizeof(buffer), "%" PRId64, value);
		SJSON_CPP_ASSERT(length > 0 && length < static_cast<int>(sizeof(buffer)), "Failed to push SJSON value: %lld", value);
		m_stream_writer.write(buffer, static_cast<size_t>(length));
		m_is_empty = false;
		m_is_newline = false;
	}

	inline void ArrayWriter::push_unsigned_integer(uint64_t value)
	{
		SJSON_CPP_ASSERT(!m_is_locked, "Cannot push SJSON value in locked array");

		if (!m_is_empty && !m_is_newline)
			m_stream_writer.write(", ");

		if (m_is_newline)
			write_indentation();

		char buffer[256];
		int length = snprintf(buffer, sizeof(buffer), "%" PRIu64, value);
		SJSON_CPP_ASSERT(length > 0 && length < static_cast<int>(sizeof(buffer)), "Failed to push SJSON value: %llu", value);
		m_stream_writer.write(buffer, static_cast<size_t>(length));
		m_is_empty = false;
		m_is_newline = false;
	}

	template<typename F, typename HavingArgument<F, ObjectWriter&>::type* requirement>
	inline void ArrayWriter::push(F writer_fun)
	{
		SJSON_CPP_ASSERT(!m_is_locked, "Cannot push SJSON object in locked array");

		if (!m_is_empty && !m_is_newline)
		{
			m_stream_writer.write(",");
			m_stream_writer.write(k_line_terminator);
		}
		else if (m_is_empty)
			m_stream_writer.write(k_line_terminator);

		write_indentation();
		m_stream_writer.write("{");
		m_stream_writer.write(k_line_terminator);
		m_is_locked = true;

		ObjectWriter object_writer(m_stream_writer, m_indent_level + 1);
		writer_fun(object_writer);

		write_indentation();
		m_stream_writer.write("}");
		m_stream_writer.write(k_line_terminator);

		m_is_locked = false;
		m_is_empty = false;
		m_is_newline = true;
	}

	template<typename F, typename HavingArgument<F, ArrayWriter&>::type* requirement>
	inline void ArrayWriter::push(F writer_fun)
	{
		SJSON_CPP_ASSERT(!m_is_locked, "Cannot push SJSON array in locked array");

		if (!m_is_empty && !m_is_newline)
			m_stream_writer.write(", ");

		if (m_is_newline)
			write_indentation();

		m_stream_writer.write("[ ");
		m_is_locked = true;

		ArrayWriter array_writer(m_stream_writer, m_indent_level);
		writer_fun(array_writer);

		m_is_locked = false;
		m_stream_writer.write(" ]");
		m_is_empty = false;
		m_is_newline = false;
	}

	inline void ArrayWriter::push_newline()
	{
		SJSON_CPP_ASSERT(!m_is_locked, "Cannot insert newline in locked array");

		m_stream_writer.write(k_line_terminator);
		m_is_newline = true;
	}

	inline void ArrayWriter::write_indentation()
	{
		for (uint32_t level = 0; level < m_indent_level; ++level)
			m_stream_writer.write("\t");
	}

	//////////////////////////////////////////////////////////////////////////

	inline Writer::Writer(StreamWriter& stream_writer)
		: ObjectWriter(stream_writer, 0)
	{}

	SJSON_CPP_IMPL_VERSION_NAMESPACE_END
}
