#pragma once

#include "core_api.h"

namespace era_engine
{

	class ERA_CORE_API TagsContainer final
	{
	public:
		TagsContainer();
		~TagsContainer() = default;

		void add_tag(const std::string& tag);
		bool remove_tag(const std::string& tag);

		bool has_tag(const std::string& tag) const;

		void clear();

		void refresh();

		const std::vector<std::string>& get_tags() const;

	private:
		bool is_dirty = false;
		std::vector<std::string> tags;
	};

}