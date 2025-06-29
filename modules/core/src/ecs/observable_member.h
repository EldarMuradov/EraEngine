#pragma once

#include "core_api.h"

#include "core/math.h"
#include "ecs/component.h"

namespace era_engine
{
	class World;

	class ERA_CORE_API ObservableBase
	{
	public:
		ObservableBase();
		virtual ~ObservableBase();

		void set_component(const ComponentPtr& _component_ptr);

		void notify();
		bool is_changed() const;

	protected:
		void sync_changes();

	protected:
		uint32 changes_count = 0;
		uint32 old_changes_count = 0;

		ComponentPtr component_ptr;

		friend class ObservableStorage;
	};

	template <typename Type_>
	class ObservableMember : public ObservableBase
	{
	public:
		ObservableMember() : ObservableBase() {}

		ObservableMember(const ObservableMember& other)
			: ObservableBase(), member(other.member)
		{
		}

		ObservableMember(ObservableMember&& other)
			: ObservableBase(), member(std::move(other.member))
		{
		}

		ObservableMember(const Type_& _member)
			: ObservableBase(), member(_member)
		{
		}

		ObservableMember(Type_&& _member)
			: ObservableBase(), member(std::forward<Type_>(_member))
		{
		}

		ObservableMember& operator=(const ObservableMember& other)
		{
			set_and_notify(other.member);
			return *this;
		}

		ObservableMember& operator=(ObservableMember&& other) noexcept
		{
			set_and_notify(std::forward<Type_>(other.new_member));
			return *this;
		}

		ObservableMember& operator=(Type_&& new_member)
		{
			set_and_notify(std::forward<Type_>(new_member));
			return *this;
		}
		ObservableMember& operator=(const Type_& new_member)
		{
			set_and_notify(new_member);
			return *this;
		}

		const Type_& get() const
		{
			return member;
		}

		Type_& get_for_write()
		{
			notify();
			return member;
		}

		Type_& get_silent_for_write()
		{
			return member;
		}

		const Type_* operator->() const
		{
			return &member;
		}

		const Type_& operator*() const
		{
			return member;
		}

		operator const Type_& () const
		{
			return member;
		}

	private:
		void set_and_notify(const Type_& new_member)
		{
			if (member != new_member)
			{
				member = new_member;
				notify();
			}
		}

		void set_and_notify(Type_&& new_member)
		{
			if (member != new_member)
			{
				member = std::forward<Type_>(new_member);
				notify();
			}
		}

	private:
		Type_ member;
	};

	class ObservableStorage
	{
	public:
		static std::vector<ObservableBase*> observable_ptrs;

		static void sync_all_changes(World* world);
	};

	class ERA_CORE_API ObservableMemberChangedFlagComponent final : public Component
	{
	public:
		ObservableMemberChangedFlagComponent(ref<Entity::EcsData> _data);

		ERA_VIRTUAL_REFLECT(Component)
	};
}