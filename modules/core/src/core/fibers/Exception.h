#pragma once

#include <exception>
#include <string>

namespace era_engine
{
	class Exception : public std::exception
	{
		const std::string m_str;

	public:
		Exception() throw() = default;
		Exception(const std::string& str) throw() : m_str(str) {};
		~Exception() throw() = default;

		virtual const char* what() const override { return m_str.c_str(); }
	};
}