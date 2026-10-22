#include <gtest/gtest.h>

#include <asset/model_asset.h>
#include <asset/pbr_material_desc.h>

#include <core/serialization/binary_serializer.h>

#include <fstream>

namespace
{
    using namespace era_engine;

    ModelAsset make_model()
    {
        ModelAsset model;
        model.flags = 7;

        MeshAsset mesh;
        mesh.name = "Body";
        SubmeshAsset submesh;
        submesh.positions = {vec3(0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f)};
        submesh.triangles = {{0, 1, 2}};
        mesh.submeshes.push_back(submesh);
        model.meshes.push_back(mesh);

        PbrMaterialDesc plain;
        plain.albedo = "albedo.png";
        PbrMaterialDesc emissive;
        emissive.emission_texture = "screen_em.png";
        emissive.emission = vec4(2.0f, 2.0f, 2.0f, 1.0f);
        model.materials = {plain, emissive};

        return model;
    }
}

TEST(ModelAssetSerialization, EmissionTexturesRoundTrip)
{
    const ModelAsset model = make_model();

    const BinaryDataArchive data = BinarySerializer::serialize(model);
    ModelAsset loaded;
    ASSERT_EQ(BinarySerializer::deserialize(BinaryData(data), loaded), data.size());

    ASSERT_EQ(loaded.materials.size(), 2u);
    EXPECT_EQ(loaded.materials[0].emission_texture, "");
    EXPECT_EQ(loaded.materials[1].emission_texture, "screen_em.png");
    EXPECT_EQ(loaded.materials[0].albedo, "albedo.png");
    EXPECT_EQ(loaded.meshes[0].submeshes[0].positions.size(), 3u);
}

TEST(ModelAssetTextures, FindsTexturesMovedNextToModel)
{
    const fs::path root = fs::temp_directory_path() / "era_model_texture_test";
    fs::remove_all(root);
    fs::create_directories(root / "source");
    fs::create_directories(root / "textures");
    std::ofstream(root / "source" / "tire.png") << "x";
    std::ofstream(root / "textures" / "glass.png") << "x";

    const fs::path model = root / "source" / "car.fbx";

    EXPECT_EQ(find_model_texture("car.fbm\\tire.png", model), (root / "source" / "tire.png").lexically_normal());
    EXPECT_EQ(find_model_texture("C:/Users/artist/glass.png", model), (root / "textures" / "glass.png").lexically_normal());
    EXPECT_EQ(find_model_texture("tire.png", model), (root / "source" / "tire.png").lexically_normal());

    fs::remove_all(root);
}
