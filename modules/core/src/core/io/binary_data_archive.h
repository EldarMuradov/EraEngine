#pragma once 

#include "core_api.h"

namespace era_engine
{
	class ERA_CORE_API BinaryDataArchive final
	{
	public:
        BinaryDataArchive() = default;
        BinaryDataArchive(size_t size);
        BinaryDataArchive(size_t size, uint8 value);
        BinaryDataArchive(const uint8* source_data, size_t size);

        void fill(const uint8* source_data, size_t size);

        size_t size() const;

        void reset();
        void resize(size_t new_size);

        const uint8* raw_data() const;
        uint8* raw_data();

        const std::vector<uint8>& data() const;
        std::vector<uint8>& data();

        bool operator==(const BinaryDataArchive& other) const;
        bool operator!=(const BinaryDataArchive& other) const;

    private:
        std::vector<uint8> storage;
	};
}