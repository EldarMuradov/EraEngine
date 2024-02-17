#pragma once

#include <functional>
#include "px/core/px_physics_engine.h"

namespace helper
{
	template <int... Is>
	struct index {};

	template <int N, int... Is>
	struct gen_seq : gen_seq<N - 1, N - 1, Is...> {};

	template <int... Is>
	struct gen_seq<0, Is...> : index<Is...> {};
}

template<typename Func, typename... Args>
struct px_task : physx::PxLightCpuTask
{
	px_task() = default;

	px_task(std::function<Func(Args...)> f, Args&&... ars)
		: func(f),
		args(std::forward<Args>(ars)...) {}
	~px_task() {}

	virtual const char* getName() const { return "PhysX Task"; }
	virtual void run() { call(args); }

private:
	template <typename... Args, int... Is>
	void call(std::tuple<Args...>& tup, helper::index<Is...>)
	{
		func(std::get<Is>(tup)...);
	}

	template <typename... Args>
	void call(std::tuple<Args...>& tup)
	{
		call(tup, helper::gen_seq<sizeof...(Args)>{});
	}

protected:
	std::tuple<Args...> args;
	std::function<Func(Args...)> func;
};