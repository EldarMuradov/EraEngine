#include "ecs/serialization/reflection_serializer.h"
#include "ecs/serialization/private/yaml_value_codec.h"

#include "core/log.h"

namespace era_engine
{
	std::string ReflectionSerializer::serialize(const rttr::instance& object)
	{
		YAML::Emitter emitter;
		emitter << YamlValueCodec::write_object(object, YamlWriteContext{});
		return emitter.c_str();
	}

	bool ReflectionSerializer::deserialize(std::string_view text, const rttr::instance& object)
	{
		try
		{
			const YAML::Node node = YAML::Load(std::string(text));
			YamlValueCodec::read_object(node, object, YamlReadContext{});
			return true;
		}
		catch (const YAML::Exception& exception)
		{
			LOG_ERROR("Serialization> Failed to parse YAML: %s", exception.what());
			return false;
		}
	}
}
