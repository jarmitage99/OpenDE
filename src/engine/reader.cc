#include "engine/reader.h"

#include <cstring>
#include <stdexcept>

namespace dx {
    Reader::Reader(std::span<const uint8_t> data, size_t offset)
        : buffer(data)
        , position(offset)
    {
    }

    void Reader::require(size_t count) const
    {
        if (position > buffer.size() || buffer.size() - position < count) {
            throw std::runtime_error("read past end of package data");
        }
    }

    void Reader::bytes(void* out, size_t count)
    {
        require(count);
        std::memcpy(out, buffer.data() + position, count);
        position += count;
    }

    uint8_t Reader::u8()
    {
        require(1);
        return buffer[position++];
    }

    int16_t Reader::i16()
    {
        int16_t value;
        bytes(&value, sizeof(value));
        return value;
    }

    uint32_t Reader::u32()
    {
        uint32_t value;
        bytes(&value, sizeof(value));
        return value;
    }

    int32_t Reader::i32()
    {
        int32_t value;
        bytes(&value, sizeof(value));
        return value;
    }

    uint64_t Reader::u64()
    {
        uint64_t value;
        bytes(&value, sizeof(value));
        return value;
    }

    float Reader::f32()
    {
        float value;
        bytes(&value, sizeof(value));
        return value;
    }

    int32_t Reader::index()
    {
        uint8_t byte = u8();
        bool negative = (byte & 0x80) != 0;
        uint32_t value = byte & 0x3fu;
        if (byte & 0x40) {
            int shift = 6;
            do {
                byte = u8();
                value |= static_cast<uint32_t>(byte & 0x7f) << shift;
                shift += 7;
            } while ((byte & 0x80) && shift < 32);
        }
        int32_t result = static_cast<int32_t>(value);
        if (negative) {
            return -result;
        }
        return result;
    }

    std::string Reader::string()
    {
        int32_t length = index();
        if (length <= 0) {
            return {};
        }
        std::string result(static_cast<size_t>(length), '\0');
        bytes(result.data(), result.size());
        if (!result.empty() && result.back() == '\0') {
            result.pop_back();
        }
        return result;
    }

    void Reader::skip(size_t count)
    {
        require(count);
        position += count;
    }

    void Reader::seek(size_t offset)
    {
        if (offset > buffer.size()) {
            throw std::runtime_error("seek past end of package data");
        }
        position = offset;
    }

    size_t Reader::tell() const
    {
        return position;
    }
}
