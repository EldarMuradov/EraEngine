#pragma once

#include "core_api.h"
#include "core/io/binary_data_archive.h"

namespace era_engine
{
    class BinaryDataArchive;

    class ERA_CORE_API BinaryData final
    {
    public:
        BinaryData() = default;
        BinaryData(const uint8* storage, size_t size);

        explicit BinaryData(const BinaryDataArchive& binary);
        explicit BinaryData(const std::string& string);

        const uint8* data() const;

        size_t size() const;
        void reset(const uint8* storage, size_t size);

    private:
        const uint8* storage = nullptr;
        size_t storage_size = 0;
    };
}