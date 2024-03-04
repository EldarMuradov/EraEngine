#pragma once
#include <aitoolkit/behtree.hpp>

template <typename Type>
struct behaviour_tree
{
	behaviour_tree() noexcept
	{
        tree = aitoolkit::bt::sel<Type>::make(
        {
            aitoolkit::bt::seq<Type>::make(
            {
                aitoolkit::bt::check<Type>::make([](const Type& bb)
                {
                    return true;
                }),
                aitoolkit::bt::task<Type>::make([](Type& bb)
                {
                    return aitoolkit::bt::execution_state::success;
                })
            })
        });
	}

    aitoolkit::bt::execution_state evaluate(Type type)
    {
        auto state = tree.evaluate(type);
        return state;
    }

    aitoolkit::bt::sel<Type> tree;
};