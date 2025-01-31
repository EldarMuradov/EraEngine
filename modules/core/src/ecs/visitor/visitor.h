#pragma once

#include <rttr/visitor.h>

namespace era_engine
{
    class Visitor : public rttr::visitor
    {
    public:
        Visitor()
        {
        }

        template<typename T, typename...Base_Classes>
        void visit_type_begin(const type_info<T>& info)
        {
            visited_types.push_back(info.type_item);
        }

        template<typename T, typename...Base_Classes>
        void visit_type_end(const type_info<T>& info)
        {
            using declaring_type_t = typename type_info<T>::declaring_type;

            iterate_base_classes<declaring_type_t, Base_Classes...>();

            visited_types.push_back(info.type_item);
        }

        template<typename T, typename...Ctor_Args>
        void visit_constructor(const constructor_info<T>& info)
        {
            visited_ctors.push_back(info.ctor_item);
        }

        template<typename T, typename...Ctor_Args>
        void visit_constructor_function(const constructor_function_info<T>& info)
        {
            visited_ctors.push_back(info.ctor_item);
        }

        template<typename T>
        void visit_global_method(const method_info<T>& info)
        {
            visited_meths.push_back(info.method_item);
        }

        template<typename T>
        void visit_method(const method_info<T>& info)
        {
            visited_meths.push_back(info.method_item);
        }

        template<typename T>
        void visit_property(const property_info<T>& info)
        {
            visited_props.push_back(info.property_item);
        }

        template<typename T>
        void visit_getter_setter_property(const property_getter_setter_info<T>& info)
        {
            visited_props.push_back(info.property_item);
        }

        template<typename T>
        void visit_global_property(const property_info<T>& info)
        {
            visited_props.push_back(info.property_item);
        }

        template<typename T>
        void visit_global_getter_setter_property(const property_getter_setter_info<T>& info)
        {
            visited_props.push_back(info.property_item);
        }

        template<typename T>
        void visit_readonly_property(const property_info<T>& info)
        {
            visited_props.push_back(info.property_item);
        }

        template<typename T>
        void visit_global_readonly_property(const property_info<T>& info)
        {
            visited_props.push_back(info.property_item);
        }

    public:

        std::vector<rttr::type> visited_types;
        std::vector<rttr::constructor> visited_ctors;
        std::vector<rttr::method> visited_meths;
        std::vector<rttr::property> visited_props;

        RTTR_ENABLE(rttr::visitor)
    };

    template<> struct rttr_visitor_list_history<24> {
        using previous = typename rttr_visitor_list_read<24 - 1>::type; using type = typename rttr::detail::push_back<Visitor, previous>::type;
    };
}