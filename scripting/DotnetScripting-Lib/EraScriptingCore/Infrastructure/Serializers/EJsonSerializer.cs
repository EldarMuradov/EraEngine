using System.Text.Json.Serialization;

namespace EraScriptingCore.Infrastructure.Serializers;

[JsonSerializable(typeof(string), GenerationMode = JsonSourceGenerationMode.Metadata)]
internal partial class JsonContext : JsonSerializerContext { }

public static class EJsonSerializer
{
}
