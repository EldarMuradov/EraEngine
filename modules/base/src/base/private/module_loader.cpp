#include "base/module_loader.h"

namespace era_engine
{
    static std::vector<std::string> split(std::string_view source, char delimiter)
    {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream token_stream(source.data());
        while (std::getline(token_stream, token, delimiter))
        {
            if (!token.empty())
            {
                tokens.push_back(token);
            }
        }
        return tokens;
    }

    ModuleLoader::ModuleLoader(Engine* _engine, const std::string& _modules)
        : engine(_engine)
    {
        using namespace rttr;

        load_module_impl(type::get_types());

        std::vector<std::string> names = split(_modules, ';');

        for (const std::string& name : names)
        {
            all_loaded &= load_module(name);
        }
    }

    ModuleLoader::~ModuleLoader()
    {
    }

    bool ModuleLoader::load_module(std::string_view name)
    {
        using namespace rttr;
        library module_lib(name.data());

        if (!module_lib.load())
        {
            std::cerr << module_lib.get_error_string() << std::endl;
            return false;
        }

        return load_module_impl(module_lib.get_types());
    }

    bool ModuleLoader::unload_module(std::string_view name)
    {
        return false;
    }

    void ModuleLoader::add_module(IModule* _module)
    {
        modules.push_back(_module);
    }

    bool ModuleLoader::status() const
    {
        return all_loaded;
    }

    bool ModuleLoader::load_module_impl(const rttr::array_range<rttr::type>& types, bool is_unique /*= true*/)
    {
        using namespace rttr;

        type base_type = type::get<IModule>();

        bool found = false;
        for (const type& type_instance : types)
        {
            if (base_type.is_base_of(type_instance) && type_instance.get_raw_type() != base_type && type_instance.is_class())
            {
                IModule* module = type_instance.create().convert<IModule*>();
                module->initialize(static_cast<void*>(engine));
                modules.push_back(module);
                found = true;
                if (is_unique)
                {
                    break;
                }
            }
        }

        return found;
    }

}