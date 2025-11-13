#pragma once

#include "asset/io.h"

namespace era_engine
{
	template <typename T>
	inline std::streamsize IO::read_data(std::istream& is, T* value, std::streamsize size)
	{
		static_assert(std::is_trivially_copyable_v<T>);
		return is.read(reinterpret_cast<char*>(value), size).gcount();
	}

	template <typename T>
	inline bool IO::read_value(std::istream& is, T& value)
	{
		static_assert(std::is_trivially_copyable_v<T>);
		return is.read(reinterpret_cast<char*>(&value), sizeof(T)).gcount() == sizeof(T);
	}

	template <typename T>
	inline bool IO::read_vector(std::istream& is, std::vector<T>& value)
	{
		uint64 count = 0;
		bool result = read_value(is, count);
		value.resize(size_t(count));

		if (count > 0)
		{
			result = read_data(is, value.data(), sizeof(T) * std::streamsize(count)) && result;
		}

		return result;

	}

	template <typename T>
	inline T IO::read_value(std::istream& is)
	{
		T value;
		read_value(is, value);
		return value;
	}

	inline void IO::align_pos_to_uint32(std::istream& is)
	{
		constexpr uint32 alignment = sizeof(uint32);
		const std::streampos current_pos = is.tellg();
		std::streamoff offset = static_cast<std::streamoff>(current_pos % alignment);

		if (offset != 0)
		{
			const std::streamoff padding = alignment - offset;
			is.seekg(padding, std::ios::cur);
		}
	}

	template <typename T>
	inline bool IO::write_data(std::ostream& os, const T* value, std::streamsize size)
	{
		return os.write(reinterpret_cast<const char*>(value), size).good();
	}

	template <typename T>
	inline bool IO::write_value(std::ostream& os, const T& value)
	{
		return os.write(reinterpret_cast<const char*>(&value), sizeof(T)).good();
	}

	template <typename T>
	inline bool IO::write_vector(std::ostream& os, const std::vector<T>& value)
	{
		const uint64 count = static_cast<uint64>(value.size());
		bool result = write_value(os, count);
		if (count > 0)
		{
			result = write_data(os, value.data(), sizeof(T) * std::streamsize(count)) && result;
		}
		return result;
	}

	template <>
	inline bool IO::read_value(std::istream& is, BinaryDataArchive& value)
	{
		uint32 value_size = 0;
		if (!read_value(is, value_size))
		{
			return false;
		}

		value.data().resize(value_size);
		if (value_size > 0)
		{
			return read_data(is, value.raw_data(), value_size);
		}
		else
		{
			return true;
		}
	}

	template <>
	inline bool IO::write_value(std::ostream& os, const BinaryDataArchive& value)
	{
		return write_value(os, static_cast<uint32>(value.size())) &&
			write_data(os, value.raw_data(), value.size());
	}

	inline void IO::align_pos_to_uint32(std::ostream& os)
	{
		constexpr uint32 alignment = sizeof(uint32);
		const uint64 current_pos = os.tellp();
		const uint32 padding_needed = alignment - (current_pos % alignment);

		if (padding_needed != alignment)
		{
			constexpr char padding[4] = {};
			os.write(padding, padding_needed);
		}
	}
}