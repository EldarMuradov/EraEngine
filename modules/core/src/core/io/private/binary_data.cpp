#include "core/io/binary_data.h"
#include "core/io/binary_data_archive.h"

namespace era_engine
{
    BinaryData::BinaryData(const uint8* _storage, size_t _size)
        : storage(_storage)
        , storage_size(_size)
    {
    }

    BinaryData::BinaryData(const BinaryDataArchive& binary)
        : BinaryData(binary.raw_data(), binary.size())
    {
    }

    BinaryData::BinaryData(const std::string& string)
        : BinaryData(reinterpret_cast<const uint8*>(string.data()), string.size())
    {
    }

    const uint8* BinaryData::data() const
    {
        return storage;
    }

    size_t BinaryData::size() const
    {
        return storage_size;
    }

    void BinaryData::reset(const uint8* _storage, size_t _size)
    {
        storage = _storage;
        storage_size = _size;
    }
}