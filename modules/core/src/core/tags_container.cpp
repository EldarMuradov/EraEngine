#include "core/tags_container.h"

namespace era_engine
{

	TagsContainer::TagsContainer()
	{
	}

	void TagsContainer::add_tag(const std::string& tag)
	{
		tags.push_back(tag);
		is_dirty = true;
	}

	bool TagsContainer::remove_tag(const std::string& tag)
	{
		refresh();

		auto iter = std::lower_bound(tags.begin(), tags.end(), tag);
		if (iter != tags.end() && *iter == tag)
		{
			tags.erase(iter);
			return true;
		}
		return false;
	}

	bool TagsContainer::has_tag(const std::string& tag) const
	{
		const_cast<TagsContainer*>(this)->refresh();

		const auto iter = std::lower_bound(tags.begin(), tags.end(), tag);
		return iter != tags.end() && *iter == tag;
	}

	void TagsContainer::clear()
	{
		tags.clear();
	}

	size_t TagsContainer::size() const
	{
		const_cast<TagsContainer*>(this)->refresh();
		return tags.size();
	}

	void TagsContainer::refresh()
	{
		if (!is_dirty)
		{
			return;
		}

		std::sort(tags.begin(), tags.end());
		auto to_erase = std::unique(tags.begin(), tags.end());
		tags.erase(to_erase, tags.end());
		is_dirty = false;
	}

	const std::vector<std::string>& TagsContainer::get_tags() const
	{
		return tags;
	}

}