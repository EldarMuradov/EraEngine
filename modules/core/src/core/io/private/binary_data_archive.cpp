#include "core/io/binary_data_archive.h"

namespace era_engine
{
BinaryDataArchive::BinaryDataArchive(size_t size)
    : storage(size)
{
}

BinaryDataArchive::BinaryDataArchive(size_t size, uint8 value)
    : storage(size, value)
{
}

BinaryDataArchive::BinaryDataArchive(const uint8* source_data, size_t size)
    : storage(source_data, source_data + size)
{
}

void BinaryDataArchive::reset()
{
    storage.clear();
}

bool BinaryDataArchive::operator==(const BinaryDataArchive& b) const
{
    return storage == b.storage;
}

bool BinaryDataArchive::operator!=(const BinaryDataArchive& b) const
{
    return storage != b.storage;
}

void BinaryDataArchive::fill(const uint8* source_data, size_t size)
{
    storage.resize(size);
    memcpy(storage.data(), source_data, size);
}

const std::vector<uint8>& BinaryDataArchive::data() const
{
    return storage;
}

std::vector<uint8>& BinaryDataArchive::data()
{
    return storage;
}

const uint8* BinaryDataArchive::raw_data() const
{
    return storage.data();
}

uint8* BinaryDataArchive::raw_data()
{
    return storage.data();
}

size_t BinaryDataArchive::size() const
{
    return storage.size();
}

void BinaryDataArchive::resize(size_t new_size)
{
    storage.resize(new_size);
}
}