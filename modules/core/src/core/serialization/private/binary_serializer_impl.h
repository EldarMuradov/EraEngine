#pragma once

#include "core/serialization/binary_serializer.h"

#include <variant>
#include <optional>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace era_engine
{
	using DynamicContainerSize = uint32;

	inline size_t get_tail_size(size_t tail_size, size_t offset)
	{
		if (offset >= tail_size)
		{
			return 0;
		}

		return tail_size - offset;
	}

    struct ByteArraySerializeHelper
    {
        static inline size_t serialize(uint8* out, const uint8* byte_array, const size_t array_len)
        {
            ASSERT(array_len < static_cast<size_t>(std::numeric_limits<DynamicContainerSize>::max()));

            DynamicContainerSize size = static_cast<DynamicContainerSize>(array_len);
            size_t offset = BinarySerializer::serialize(out, size);

            std::memcpy(out + offset, byte_array, size);
            return size + offset;
        }

        template <typename T>
        static inline size_t deserialize(const uint8* in, size_t tail_size, T& buffer)
        {
            DynamicContainerSize size = 0;

            size_t offset = BinarySerializer::deserialize(in, tail_size, size);
            size_t data_size = std::min(size_t(size), get_tail_size(tail_size, offset));

            buffer.resize(data_size);
            std::memcpy(buffer.raw_data(), in + offset, data_size);

            return data_size + offset;
        }

        static inline size_t deserialize(const uint8* in, size_t tail_size, BinaryData& buffer)
        {
            DynamicContainerSize size = 0;

            size_t offset = BinarySerializer::deserialize(in, tail_size, size);

            size_t data_size = std::min(size_t(size), get_tail_size(tail_size, offset));

            buffer.reset(in + offset, data_size);

            return data_size + offset;
        }

        static inline size_t get_size(const size_t string_len)
        {
            return string_len + sizeof(DynamicContainerSize);
        }
    };

    template <class T, typename E = void>
    struct Serializer
    {
        static inline size_t serialize(uint8* out, const T& obj)
        {
            return obj.serialize(out);
        }

        static inline size_t deserialize(const uint8* in, size_t tail_size, T& obj)
        {
            return obj.deserialize(in, tail_size);
        }

        static inline size_t get_size(const T& obj)
        {
            return obj.get_size();
        }
    };

    template <class A, class B>
    struct Serializer<std::pair<A, B>>
    {
        static inline size_t serialize(uint8* out, const std::pair<A, B>& obj)
        {
            size_t size = BinarySerializer::serialize(out, obj.first);
            return size + BinarySerializer::serialize(out + size, obj.second);
        }

        static inline size_t deserialize(const uint8* in, size_t tail_size, std::pair<A, B>& obj)
        {
            size_t size = BinarySerializer::deserialize(in, tail_size, obj.first);
            return size + BinarySerializer::deserialize(in + size, get_tail_size(tail_size, size), obj.second);
        }

        static inline size_t get_size(const std::pair<A, B>& obj)
        {
            return BinarySerializer::get_size(obj.first) + BinarySerializer::get_size(obj.second);
        }
    };

    template <>
    struct Serializer<vec2>
    {
        static inline size_t serialize(uint8* out, const vec2& obj)
        {
            const size_t size = BinarySerializer::get_size(obj);
            std::memcpy(out, obj.data, size);
            return size;
        }

        static inline size_t deserialize(const uint8* in, size_t tail_size, vec2& obj)
        {
            const size_t size = BinarySerializer::get_size(obj);
            if (size <= tail_size)
            {
                std::memcpy(obj.data, in, size);
            }
            return size;
        }

        static inline size_t get_size(const vec2& obj)
        {
            return sizeof(obj);
        }
    };

    template <>
    struct Serializer<vec3>
    {
        static inline size_t serialize(uint8* out, const vec3& obj)
        {
            const size_t size = BinarySerializer::get_size(obj);
            std::memcpy(out, obj.data, size);
            return size;
        }

        static inline size_t deserialize(const uint8* in, size_t tail_size, vec3& obj)
        {
            const size_t size = BinarySerializer::get_size(obj);
            if (size <= tail_size)
            {
                std::memcpy(obj.data, in, size);
            }
            return size;
        }

        static inline size_t get_size(const vec3& obj)
        {
            return sizeof(obj);
        }
    };

    template <>
    struct Serializer<vec4>
    {
        static inline size_t serialize(uint8* out, const vec4& obj)
        {
            const size_t size = BinarySerializer::get_size(obj);
            std::memcpy(out, obj.data, size);
            return size;
        }

        static inline size_t deserialize(const uint8* in, size_t tail_size, vec4& obj)
        {
            const size_t size = BinarySerializer::get_size(obj);
            if (size <= tail_size)
            {
                std::memcpy(obj.data, in, size);
            }
            return size;
        }

        static inline size_t get_size(const vec4& obj)
        {
            return sizeof(obj);
        }
    };

    template <>
    struct Serializer<mat4>
    {
        static inline size_t serialize(uint8* out, const mat4& obj)
        {
            const size_t size = BinarySerializer::get_size(obj);
            std::memcpy(out, obj.m, size);
            return size;
        }

        static inline size_t deserialize(const uint8* in, size_t tail_size, mat4& obj)
        {
            const size_t size = BinarySerializer::get_size(obj);
            if (size <= tail_size)
            {
                std::memcpy(obj.m, in, size);
            }
            return size;
        }

        static inline size_t get_size(const mat4& obj)
        {
            return sizeof(obj);
        }
    };

    template <>
    struct Serializer<mat3>
    {
        static inline size_t serialize(uint8* out, const mat3& obj)
        {
            const size_t size = BinarySerializer::get_size(obj);
            std::memcpy(out, obj.m, size);
            return size;
        }

        static inline size_t deserialize(const uint8* in, size_t tail_size, mat3& obj)
        {
            const size_t size = BinarySerializer::get_size(obj);
            if (size <= tail_size)
            {
                std::memcpy(obj.m, in, size);
            }
            return size;
        }

        static inline size_t get_size(const mat3& obj)
        {
            return sizeof(obj);
        }
    };

    template <>
    struct Serializer<mat2>
    {
        static inline size_t serialize(uint8* out, const mat2& obj)
        {
            const size_t size = BinarySerializer::get_size(obj);
            std::memcpy(out, obj.m, size);
            return size;
        }

        static inline size_t deserialize(const uint8* in, size_t tail_size, mat2& obj)
        {
            const size_t size = BinarySerializer::get_size(obj);
            if (size <= tail_size)
            {
                std::memcpy(obj.m, in, size);
            }
            return size;
        }

        static inline size_t get_size(const mat2& obj)
        {
            return sizeof(obj);
        }
    };

    template <>
    struct Serializer<quat>
    {
        static inline size_t serialize(uint8* out, const quat& obj)
        {
            const size_t size = BinarySerializer::get_size(obj);
            std::memcpy(out, obj.v4.data, size);
            return size;
        }

        static inline size_t deserialize(const uint8* in, size_t tail_size, quat& obj)
        {
            const size_t size = BinarySerializer::get_size(obj);
            if (size <= tail_size)
            {
                std::memcpy(obj.v4.data, in, size);
            }
            return size;
        }

        static inline size_t get_size(const quat& obj)
        {
            return sizeof(obj);
        }
    };

    template <>
    struct Serializer<trs>
    {
        static inline size_t serialize(uint8* out, const trs& obj)
        {
            size_t size = BinarySerializer::serialize(out, obj.position);
            size += BinarySerializer::serialize(out + size, obj.rotation);
            return size + BinarySerializer::serialize(out + size, obj.scale);
        }

        static inline size_t deserialize(const uint8* in, size_t tail_size, trs& obj)
        {
            size_t size = BinarySerializer::deserialize(in, tail_size, obj.position);
            size += BinarySerializer::deserialize(in + size, get_tail_size(tail_size, size), obj.rotation);
            return size + BinarySerializer::deserialize(in + size, get_tail_size(tail_size, size), obj.scale);
        }

        static inline size_t get_size(const trs& obj)
        {
            return BinarySerializer::get_size(obj.position) + BinarySerializer::get_size(obj.rotation) + BinarySerializer::get_size(obj.scale);
        }
    };

    template <>
    struct Serializer<BinaryDataArchive>
    {
        static inline size_t serialize(uint8* out, const BinaryDataArchive& obj)
        {
            return ByteArraySerializeHelper::serialize(out, obj.raw_data(), obj.size());
        }

        static inline size_t deserialize(const uint8* in, size_t tail_size, BinaryDataArchive& obj)
        {
            return ByteArraySerializeHelper::deserialize(in, tail_size, obj);
        }

        static inline size_t get_size(const BinaryDataArchive& obj)
        {
            return ByteArraySerializeHelper::get_size(obj.size());
        }
    };

    template <>
    struct Serializer<BinaryData>
    {
        static inline size_t serialize(uint8* out, const BinaryData& obj)
        {
            const DynamicContainerSize size_to_serialize = static_cast<DynamicContainerSize>(obj.size());
            const size_t offset = BinarySerializer::serialize(out, size_to_serialize);

            std::memcpy(out + offset, obj.data(), size_to_serialize);

            return size_to_serialize + offset;
        }

        static inline size_t deserialize(const uint8* in, size_t tail_size, BinaryData& obj)
        {
            DynamicContainerSize size = 0;
            const size_t offset = BinarySerializer::deserialize(in, tail_size, size);
            const size_t size_to_deserialize = std::min(size_t(size), get_tail_size(tail_size, offset));

            obj.reset(in + offset, size_to_deserialize);

            return size_to_deserialize + offset;
        }

        static inline size_t get_size(const BinaryData& obj)
        {
            return sizeof(DynamicContainerSize) + obj.size();
        }
    };

    template <class T>
    struct PodSerializer
    {
        static inline size_t serialize(uint8* out, const T& obj)
        {
            const size_t size = BinarySerializer::get_size(obj);
            std::memcpy(out, &obj, size);
            return size;
        }

        static inline size_t deserialize(const uint8* in, size_t tail_size, T& obj)
        {
            const size_t size = BinarySerializer::get_size(obj);
            if (size <= tail_size)
            {
                std::memcpy(&obj, in, size);
            }
            return size;
        }

        static inline size_t get_size(const T& obj)
        {
            return sizeof(T);
        }
    };

    template <>
    inline size_t PodSerializer<bool>::get_size(const bool& obj)
    {
        return sizeof(uint8);
    }

    template <class T>
    struct Serializer<T, typename std::enable_if_t<std::is_enum<T>::value>>
    {
        static inline size_t serialize(uint8* out, const T& obj)
        {
            const size_t size = sizeof(std::underlying_type_t<T>);
            std::memcpy(out, &obj, size);
            return size;
        }

        static inline size_t deserialize(const uint8* in, size_t tail_size, T& obj)
        {
            const size_t size = sizeof(std::underlying_type_t<T>);
            if (size <= tail_size)
            {
                std::memcpy(&obj, in, size);
            }
            return size;
        }

        static inline size_t get_size(const T& obj)
        {
            return sizeof(std::underlying_type_t<T>);
        }
    };

    template <class T>
    struct Serializer<T, typename std::enable_if_t<!std::is_enum<T>::value && !std::is_class<T>::value>> : public PodSerializer<T>
    {
    };

    template <class T>
    struct Serializer<std::optional<T>>
    {
        static inline size_t serialize(uint8* out, const std::optional<T>& value)
        {
            bool has_value = value.has_value();
            size_t offset = BinarySerializer::serialize(out, has_value);
            if (has_value == true)
            {
                offset += BinarySerializer::serialize(out + offset, value.value());
            }

            return offset;
        }

        static inline size_t deserialize(const uint8* in, size_t tail_size, std::optional<T>& value)
        {
            bool has_value = false;
            size_t offset = BinarySerializer::deserialize(in, tail_size, has_value);
            if (has_value == true)
            {
                value.emplace();
                offset += BinarySerializer::deserialize(in + offset, get_tail_size(tail_size, offset), value.value());
            }

            return offset;
        }

        static inline size_t get_size(const std::optional<T>& value)
        {
            size_t result = BinarySerializer::get_size(value.has_value());
            if (value.has_value() == true)
            {
                result += BinarySerializer::get_size<T>(value.value());
            }

            return result;
        }
    };

    template <class Pointer>
    struct SmartPointerSerializer
    {
        static inline size_t serialize(uint8* out, const Pointer& obj_ptr)
        {
            const uint8 empty = (obj_ptr == nullptr);
            size_t offset = BinarySerializer::serialize(out, empty);
            if (!empty)
            {
                offset += BinarySerializer::serialize(out + offset, *obj_ptr);
            }

            return offset;
        }

        static inline size_t deserialize(const uint8* in, size_t tail_size, Pointer& obj_ptr)
        {
            uint8 empty;
            size_t offset = BinarySerializer::deserialize(in, tail_size, empty);
            if (!empty)
            {
                obj_ptr.reset(new typename Pointer::element_type());
                offset += BinarySerializer::deserialize(in + offset, get_tail_size(tail_size, offset), *obj_ptr);
            }

            return offset;
        }

        static inline size_t get_size(const Pointer& obj_ptr)
        {
            size_t size = sizeof(uint8);
            if (obj_ptr)
            {
                size += BinarySerializer::get_size(*obj_ptr);
            }

            return size;
        }
    };

    template <class T>
    struct Serializer<std::unique_ptr<T>> : public SmartPointerSerializer<std::unique_ptr<T>>
    {
    };

    template <class T>
    struct Serializer<std::shared_ptr<T>> : public SmartPointerSerializer<std::shared_ptr<T>>
    {
    };

    template <class Collection, class ValueType = typename Collection::value_type>
    struct CollectionSerializer
    {
        static inline size_t get_size(const Collection& collection)
        {
            return std::reduce(collection.begin(), collection.end(), sizeof(DynamicContainerSize), [](size_t size, auto& item)
                {
                    return size + BinarySerializer::get_size(item);
                });
        }

        static inline size_t serialize(uint8* out, const Collection& collection)
        {
            ASSERT(collection.size() < static_cast<size_t>(std::numeric_limits<DynamicContainerSize>::max()));

            const auto items_cnt = static_cast<DynamicContainerSize>(collection.size());
            return std::reduce(collection.begin(), collection.end(), BinarySerializer::serialize(out, items_cnt), [out](size_t offset, auto& item)
                {
                    return offset + BinarySerializer::serialize(out + offset, item);
                });
        }

        static inline size_t deserialize(const uint8* in, size_t tail_size, Collection& collection)
        {
            DynamicContainerSize items_cnt = 0;
            size_t offset = BinarySerializer::deserialize(in, tail_size, items_cnt);
            collection.clear();

            for (DynamicContainerSize i = 0; i < items_cnt; ++i)
            {
                const size_t curr_tail_size = get_tail_size(tail_size, offset);
                if (curr_tail_size > 0)
                {
                    ValueType item;
                    offset += BinarySerializer::deserialize(in + offset, curr_tail_size, item);

                    collection.insert(collection.end(), std::move(item));
                }
            }

            return offset;
        }
    };

    template <class Collection, class ValueType = typename Collection::value_type>
    struct SequentialCollectionSerializer
    {
        static inline size_t get_size(const Collection& collection)
        {
            return std::reduce(collection.begin(), collection.end(), sizeof(DynamicContainerSize), [](size_t size, auto& item)
                {
                    return size + BinarySerializer::get_size(item);
                });
        }

        static size_t serialize(uint8* out, const Collection& collection)
        {
            ASSERT(collection.size() < static_cast<size_t>(std::numeric_limits<DynamicContainerSize>::max()));

            const auto items_cnt = static_cast<DynamicContainerSize>(collection.size());

            size_t offset = BinarySerializer::serialize(out, items_cnt);

            offset = std::reduce(collection.begin(), collection.end(), offset, [out](size_t item_offset, auto& item)
                {
                    return item_offset + BinarySerializer::serialize(out + item_offset, item);
                });

            return offset;
        }

        static size_t deserialize(const uint8* in, size_t tail_size, Collection& collection)
        {
            DynamicContainerSize items_cnt = 0;
            size_t offset = BinarySerializer::deserialize(in, tail_size, items_cnt);
            collection.clear();

            collection.reserve(items_cnt);

            for (DynamicContainerSize i = 0; i < items_cnt; ++i)
            {
                const size_t curr_tail_size = get_tail_size(tail_size, offset);
                if (curr_tail_size > 0)
                {
                    ValueType item;
                    offset += BinarySerializer::deserialize(in + offset, curr_tail_size, item);
                    collection.insert(collection.end(), std::move(item));
                }
            }

            return offset;
        }
    };

    template <class T>
    struct Serializer<std::vector<T>> : public SequentialCollectionSerializer<std::vector<T>>
    {
    };

    template <class T>
    struct Serializer<std::list<T>> : public CollectionSerializer<std::list<T>>
    {
    };

    template <>
    struct Serializer<std::string> : public SequentialCollectionSerializer<std::string>
    {
    };

    template <class K, class V, class Cmp>
    struct Serializer<std::map<K, V, Cmp>> : public CollectionSerializer<std::map<K, V, Cmp>, std::pair<K, V>>
    {
    };

    template <class K, class V>
    struct Serializer<std::unordered_map<K, V>> : public CollectionSerializer<std::unordered_map<K, V>, std::pair<K, V>>
    {
    };

    template <class T, class Cmp>
    struct Serializer<std::set<T, Cmp>> : public CollectionSerializer<std::set<T, Cmp>>
    {
    };

    template <class T>
    struct Serializer<std::unordered_set<T>> : public CollectionSerializer<std::unordered_set<T>>
    {
    };

    template <class T, size_t N>
    struct Serializer<std::array<T, N>>
    {
        static inline size_t serialize(uint8* out, const std::array<T, N>& array)
        {
            size_t offset = 0;
            for (const T& value : array)
            {
                offset += BinarySerializer::serialize(out + offset, value);
            }
            return offset;
        }

        static inline size_t deserialize(const uint8* in, size_t tail_size, std::array<T, N>& array)
        {
            size_t offset = 0;
            for (T& value : array)
            {
                const size_t current_tail_size = get_tail_size(tail_size, offset);
                if (current_tail_size > 0)
                {
                    offset += BinarySerializer::deserialize(in + offset, current_tail_size, value);
                }
            }
            return offset;
        }

        static inline size_t get_size(const std::array<T, N>& array)
        {
            size_t result = 0;

            for (const T& value : array)
            {
                result += BinarySerializer::get_size<T>(value);
            }

            return result;
        }
    };

    template <class T>
    inline size_t BinarySerializer::serialize(uint8* out, const T& obj)
    {
        return Serializer<T>::serialize(out, obj);
    }

    template <class Head, class... Tail>
    inline size_t BinarySerializer::serialize(uint8* out, const Head& hd, Tail&... tl)
    {
        const size_t size = serialize(out, hd);
        return size + serialize(out + size, tl...);
    }

    template <class T>
    inline BinaryDataArchive BinarySerializer::serialize(const T& obj)
    {
        BinaryDataArchive in(get_size(obj));
        serialize(in.raw_data(), obj);
        return in;
    }

    template <class T>
    inline size_t BinarySerializer::deserialize(const uint8* in, size_t tail_size, T& obj)
    {
        return Serializer<T>::deserialize(in, tail_size, obj);
    }

    template <class Head, class... Tail>
    inline size_t BinarySerializer::deserialize(const uint8* in, size_t tail_size, Head& hd, Tail&... tl)
    {
        const size_t size = deserialize(in, tail_size, hd);
        return size + deserialize(in + size, get_tail_size(tail_size, size), tl...);
    }

    template <class Head, class... Tail>
    inline size_t BinarySerializer::deserialize(const BinaryData& binary, Head& hd, Tail&... tl)
    {
        return deserialize(binary.data(), binary.size(), hd, tl...);
    }

    template <class T>
    inline size_t BinarySerializer::get_size(const T& obj)
    {
        return Serializer<T>::get_size(obj);
    }

    template <class Head, class... Tail>
    inline size_t BinarySerializer::get_size(const Head& hd, Tail&... tl)
    {
        return get_size(hd) + get_size(tl...);
    }
}