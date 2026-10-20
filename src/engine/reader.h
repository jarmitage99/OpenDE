#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace dx {
    class Reader {
    public:
        Reader(std::span<const uint8_t> data, size_t offset);

        uint8_t u8();
        int16_t i16();
        uint32_t u32();
        int32_t i32();
        uint64_t u64();
        float f32();
        int32_t index();
        std::string string();
        void bytes(void* out, size_t count);
        void skip(size_t count);
        void seek(size_t offset);
        size_t tell() const;

    private:
        void require(size_t count) const;

        std::span<const uint8_t> buffer;
        size_t position;
    };
}
