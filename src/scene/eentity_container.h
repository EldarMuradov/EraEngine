#pragma once

#include <vector>
#include <unordered_map>
#include <mutex>

struct eentity;

struct lock
{
	lock(std::mutex& mutex) : sync(&mutex)
	{
		sync->lock();
	}

	~lock() 
	{
		sync->unlock();
	}

	std::mutex* sync;
};

struct eentity_container 
{
	struct eentity_node 
	{
		eentity_node() = default;
		~eentity_node() {}

		std::vector<eentity*> childs;
	};

	static void emplacePair(eentity* parent, eentity* child) noexcept
	{
		lock l(sync);

		if (!parent || !child)
			return;

		if (container.find(parent) == container.end())
			container.emplace(std::make_pair(parent, eentity_node()));
		container.at(parent).childs.push_back(child);
	}

	static void erasePair(eentity* parent, eentity* child) noexcept
	{
		lock l(sync);

		if (!parent || !child)
			return;

		if (container.find(parent) == container.end())
			return;

		auto& iter = container.at(parent).childs.begin();
		const auto& end = container.at(parent).childs.end();
		
		for (; iter != end; ++iter) 
		{
			if (*iter == child)
				container.at(parent).childs.erase(iter);
		}
	}

	static std::vector<eentity*> getChilds(eentity* parent)
	{
		if (!parent || container.find(parent) == container.end())
			return std::vector<eentity*>();

		return container.at(parent).childs;
	}

private:
	static std::unordered_map<eentity*, eentity_node> container;
	static std::mutex sync;
};