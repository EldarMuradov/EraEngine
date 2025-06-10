#include "ecs/observable_member.h"

namespace era_engine
{
	std::vector<ObservableBase*> ObservableStorage::observable_ptrs;
	static std::mutex observable_sync;

	ObservableBase::ObservableBase()
	{
		std::lock_guard<std::mutex> _lock{ observable_sync };
		ObservableStorage::observable_ptrs.push_back(this);
	}

	ObservableBase::~ObservableBase()
	{
		std::lock_guard<std::mutex> _lock{ observable_sync };
		auto iter_to_remove = std::find(ObservableStorage::observable_ptrs.begin(), ObservableStorage::observable_ptrs.end(), this);
		if (iter_to_remove != ObservableStorage::observable_ptrs.end())
		{
			ObservableStorage::observable_ptrs.erase(iter_to_remove);
		}
	}

	void ObservableBase::notify()
	{
		++changes_count;
	}

	bool ObservableBase::is_changed() const
	{
		return changes_count != old_changes_count;
	}

	void ObservableBase::sync_changes()
	{
		old_changes_count = changes_count;
	}

	void ObservableStorage::sync_all_changes()
	{
		for (ObservableBase* observable : observable_ptrs)
		{
			observable->sync_changes();
		}
	}

}