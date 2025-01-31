// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include <aitoolkit/fsm.hpp>
#include <aitoolkit/utility.hpp>
#include <aitoolkit/goap.hpp>

namespace era_engine::ai
{
	template <typename Type>
	using StackMachine = aitoolkit::fsm::stack_machine<Type>;

	template <typename Type>
	using StateBase = aitoolkit::fsm::state<Type>;

	template <typename Type>
	struct StateMachine : StateBase<Type> { };

	template <typename Type>
	using UtilityAction = aitoolkit::utility::action<Type>;

	template <typename Type>
	using Evaluator = aitoolkit::utility::evaluator<Type>;

	template <typename Type>
	using Action = aitoolkit::goap::action<Type>;
}