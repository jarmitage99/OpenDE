#include "engine/reader.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <vector>

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
        if (length < 0) {
            std::vector<uint16_t> units(static_cast<size_t>(-static_cast<int64_t>(length)));
            for (uint16_t& unit : units) {
                unit = static_cast<uint16_t>(i16());
            }
            while (!units.empty() && units.back() == 0) {
                units.pop_back();
            }
            bool wide = std::any_of(units.begin(), units.end(), [](uint16_t unit) {
                return unit > 0xff;
            });
            std::string result;
            for (uint16_t unit : units) {
                if (!wide || unit < 0x80) {
                    result.push_back(static_cast<char>(unit));
                } else if (unit < 0x800) {
                    result.push_back(static_cast<char>(0xc0 | (unit >> 6)));
                    result.push_back(static_cast<char>(0x80 | (unit & 0x3f)));
                } else {
                    result.push_back(static_cast<char>(0xe0 | (unit >> 12)));
                    result.push_back(static_cast<char>(0x80 | ((unit >> 6) & 0x3f)));
                    result.push_back(static_cast<char>(0x80 | (unit & 0x3f)));
                }
            }
            return result;
        }
        if (length == 0) {
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
