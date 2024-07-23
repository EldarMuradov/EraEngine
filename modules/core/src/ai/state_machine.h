// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include <aitoolkit/fsm.hpp>
#include <aitoolkit/utility.hpp>
#include <aitoolkit/goap.hpp>

namespace era_engine::ai
{
	template <typename Type>
	using stack_machine = aitoolkit::fsm::stack_machine<Type>;

	template <typename Type>
	using state_base = aitoolkit::fsm::state<Type>;

	template <typename Type>
	struct state_machine : state_base<Type> { };

	template <typename Type>
	using utility_action = aitoolkit::utility::action<Type>;

	template <typename Type>
	using evaluator = aitoolkit::utility::evaluator<Type>;

	template <typename Type>
	using action = aitoolkit::goap::action<Type>;
}