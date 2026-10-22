#pragma once

#include <physics/vehicles/vehicle_import_utils.h>

#include <string>

namespace era_engine
{
    class World;
    class MultiMesh;

    class VehicleImportPanel final
    {
    public:
        explicit VehicleImportPanel(World* _world);

        void open();
        void draw();

    private:
        void load_model();
        void analyze();
        void create_vehicle();

    private:
        World* world = nullptr;
        bool is_open = false;

        std::string model_path;

        char vehicle_name[64] = "vehicle";

        physics::VehicleImportSettings settings;
        physics::VehicleImportSetup setup;
        ref<MultiMesh> source;

        std::string status;
    };
}
