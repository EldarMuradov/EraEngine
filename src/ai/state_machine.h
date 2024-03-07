#pragma once

#include <aitoolkit/fsm.hpp>
#include <aitoolkit/utility.hpp>
#include <aitoolkit/goap.hpp>

template <typename Type>
using stack_machine = aitoolkit::fsm::stack_machine<Type>;

template <typename Type>
using state_machine = aitoolkit::fsm::state<Type>;

template <typename Type>
using utility_action = aitoolkit::utility::action<Type>;

template <typename Type>
using evaluator = aitoolkit::utility::evaluator<Type>;

template <typename Type>
using action = aitoolkit::goap::action<Type>;