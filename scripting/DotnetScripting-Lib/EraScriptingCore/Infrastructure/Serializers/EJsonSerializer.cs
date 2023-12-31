﻿using EraEngine.Components;
using System.Text.Json.Serialization;

namespace EraEngine.Infrastructure.Serializers;

[JsonSerializable(typeof(Type), GenerationMode = JsonSourceGenerationMode.Metadata)]
[JsonSerializable(typeof(Type[]), GenerationMode = JsonSourceGenerationMode.Metadata)]
[JsonSerializable(typeof(Dictionary<string, EComponent>), GenerationMode = JsonSourceGenerationMode.Metadata)]
[JsonSerializable(typeof(EComponent), GenerationMode = JsonSourceGenerationMode.Metadata)]
[JsonSerializable(typeof(EEntity), GenerationMode = JsonSourceGenerationMode.Metadata)]
public partial class JsonContext : JsonSerializerContext { }

public static class EJsonSerializer
{
}
