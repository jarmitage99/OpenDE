#include "compiler/assets.h"

#include "compiler/exec_parse.h"
#include "core/objects.h"
#include "engine/mesh.h"
#include "engine/package.h"
#include "engine/resources.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <set>
#include <unordered_map>

namespace dx::compiler {
    namespace {
        constexpr uint32_t PolyFlagMasked = 0x00000002;
        constexpr int FontCharacters = 256;

        const uint8_t fontRemap[FontCharacters] = {
            0,
            1,
            2,
            3,
            4,
            5,
            6,
            7,
            8,
            9,
            10,
            11,
            12,
            13,
            14,
            15,
            16,
            17,
            18,
            19,
            20,
            21,
            22,
            23,
            24,
            25,
            26,
            27,
            28,
            29,
            30,
            31,
            32,
            33,
            34,
            35,
            36,
            37,
            38,
            39,
            40,
            41,
            42,
            43,
            44,
            45,
            46,
            47,
            48,
            49,
            50,
            51,
            52,
            53,
            54,
            55,
            56,
            57,
            58,
            59,
            60,
            61,
            62,
            63,
            64,
            65,
            66,
            67,
            68,
            69,
            70,
            71,
            72,
            73,
            74,
            75,
            76,
            77,
            78,
            79,
            80,
            81,
            82,
            83,
            84,
            85,
            86,
            87,
            88,
            89,
            90,
            91,
            92,
            93,
            94,
            95,
            96,
            97,
            98,
            99,
            100,
            101,
            102,
            103,
            104,
            105,
            106,
            107,
            108,
            109,
            110,
            111,
            112,
            113,
            114,
            115,
            116,
            117,
            118,
            119,
            120,
            121,
            122,
            123,
            124,
            125,
            126,
            127,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            173,
            184,
            156,
            207,
            190,
            124,
            245,
            0,
            184,
            166,
            174,
            170,
            196,
            169,
            238,
            248,
            241,
            253,
            252,
            239,
            230,
            244,
            250,
            247,
            251,
            248,
            175,
            172,
            171,
            243,
            168,
            183,
            181,
            182,
            199,
            142,
            143,
            146,
            128,
            212,
            144,
            210,
            211,
            222,
            214,
            215,
            216,
            209,
            165,
            227,
            224,
            226,
            229,
            153,
            158,
            157,
            235,
            233,
            234,
            154,
            237,
            231,
            225,
            133,
            160,
            131,
            196,
            132,
            134,
            145,
            135,
            138,
            130,
            136,
            137,
            141,
            161,
            140,
            139,
            208,
            164,
            149,
            162,
            147,
            228,
            148,
            246,
            155,
            151,
            163,
            150,
            129,
            236,
            232,
            152,
        };

        struct Color {
            uint8_t r = 0;
            uint8_t g = 0;
            uint8_t b = 0;
            uint8_t a = 0;
        };

        struct Vector3 {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
        };

        struct Image {
            uint32_t width = 0;
            uint32_t height = 0;
            std::vector<uint8_t> pixels;
            std::array<Color, 256> palette;
        };

        struct Tracked {
            UObject* object = nullptr;
            std::vector<Name> properties;
        };

        Color unpack(uint32_t value)
        {
            Color color;
            color.r = static_cast<uint8_t>(value & 0xff);
            color.g = static_cast<uint8_t>((value >> 8) & 0xff);
            color.b = static_cast<uint8_t>((value >> 16) & 0xff);
            color.a = static_cast<uint8_t>((value >> 24) & 0xff);
            return color;
        }

        uint32_t pack(Color color)
        {
            return static_cast<uint32_t>(color.r) | (static_cast<uint32_t>(color.g) << 8) | (static_cast<uint32_t>(color.b) << 16) | (static_cast<uint32_t>(color.a) << 24);
        }

        Vector3 plane(Color color)
        {
            return { static_cast<float>(color.r) / 255.0f, static_cast<float>(color.g) / 255.0f, static_cast<float>(color.b) / 255.0f };
        }

        int appFloor(float value)
        {
            return static_cast<int>(std::nearbyint(static_cast<double>(value) - 0.5));
        }

        uint8_t channel(float value)
        {
            return static_cast<uint8_t>(std::clamp(appFloor(value * 256.0f), 0, 255));
        }

        Color toColor(Vector3 value, float divisor)
        {
            float scale = 1.0f / divisor;
            Color color;
            color.r = channel(value.x * scale);
            color.g = channel(value.y * scale);
            color.b = channel(value.z * scale);
            return color;
        }

        void accumulate(Vector3& sum, Vector3 value)
        {
            sum.x += value.x;
            sum.y += value.y;
            sum.z += value.z;
        }

        std::unordered_map<std::string, uint32_t>& classUnique()
        {
            static std::unordered_map<std::string, uint32_t> counters;
            return counters;
        }

        std::vector<Tracked>& tracked()
        {
            static std::vector<Tracked> objects;
            return objects;
        }

        bool numberedName(const std::string& name, const std::string& base, uint32_t& number)
        {
            if (name.size() <= base.size() || !iequals(std::string_view(name).substr(0, base.size()), base)) {
                return false;
            }
            for (size_t i = base.size(); i < name.size(); i++) {
                if (!std::isdigit(static_cast<unsigned char>(name[i]))) {
                    return false;
                }
            }
            number = static_cast<uint32_t>(std::stoul(name.substr(base.size())));
            return true;
        }

        void seedCounters(UObject* package)
        {
            static std::set<std::string> seeded;
            std::string key = package->name.str();
            std::transform(key.begin(), key.end(), key.begin(), [](char c) {
                return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            });
            if (!seeded.insert(key).second) {
                return;
            }
            std::filesystem::path shipped = objects::gameRoot() / "System" / (package->name.str() + ".u");
            if (!std::filesystem::exists(shipped)) {
                return;
            }
            Package file(shipped);
            uint32_t palette = std::numeric_limits<uint32_t>::max();
            uint32_t texture = std::numeric_limits<uint32_t>::max();
            for (size_t i = 0; i < file.exports().size(); i++) {
                int32_t ref = static_cast<int32_t>(i + 1);
                std::string name = file.nameAt(file.exports()[i].name);
                std::string className = file.classOf(ref);
                uint32_t number = 0;
                if (className == "Palette" && numberedName(name, "Palette", number)) {
                    palette = std::min(palette, number);
                } else if (className == "Texture" && numberedName(name, "Texture", number)) {
                    texture = std::min(texture, number);
                }
            }
            if (palette != std::numeric_limits<uint32_t>::max() && palette > 0) {
                classUnique()["Palette"] = palette - 1;
            }
            if (texture != std::numeric_limits<uint32_t>::max()) {
                classUnique()["Texture"] = texture;
            }
        }

        UObject* findChild(UObject* outer, Name name, UClass* cls)
        {
            for (UObject* object : objects::all()) {
                if (object && !object->destroyed && object->outer == outer && object->name == name && (!cls || object->cls == cls)) {
                    return object;
                }
            }
            return nullptr;
        }

        Name uniqueObjectName(UObject* outer, const std::string& base)
        {
            uint32_t& counter = classUnique()[base];
            while (true) {
                Name candidate(base + std::to_string(counter++));
                if (!findChild(outer, candidate, nullptr)) {
                    return candidate;
                }
            }
        }

        UClass* engineClass(const char* name)
        {
            Name className(name);
            for (UObject* object : objects::all()) {
                UClass* cls = cast<UClass>(object);
                if (cls && !cls->destroyed && cls->name == className && cls->outer && cls->outer->name == Name("Engine") && !cls->outer->outer) {
                    objects::ensureLoaded(cls);
                    return cls;
                }
            }
            return objects::findClass(std::string("Engine.") + name);
        }

        UClass* textureClass()
        {
            return engineClass("Texture");
        }

        std::filesystem::path resolveFile(const std::filesystem::path& directory, std::string text)
        {
            std::replace(text.begin(), text.end(), '\\', '/');
            std::filesystem::path current = directory;
            std::filesystem::path relative(text);
            for (const std::filesystem::path& part : relative) {
                std::filesystem::path exact = current / part;
                if (std::filesystem::exists(exact) || !std::filesystem::is_directory(current)) {
                    current = exact;
                    continue;
                }
                std::filesystem::path found = exact;
                for (const auto& item : std::filesystem::directory_iterator(current)) {
                    if (iequals(item.path().filename().string(), part.string())) {
                        found = item.path();
                        break;
                    }
                }
                current = found;
            }
            return current;
        }

        std::filesystem::path sourcePath(const std::filesystem::path& directory, std::string text)
        {
            std::replace(text.begin(), text.end(), '\\', '/');
            return directory / text;
        }

        bool readFile(const std::filesystem::path& path, std::vector<uint8_t>& bytes)
        {
            std::ifstream in(path, std::ios::binary);
            if (!in) {
                return false;
            }
            bytes.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
            return true;
        }

        bool writeFile(const std::filesystem::path& path, const std::vector<uint8_t>& bytes)
        {
            std::error_code error;
            std::filesystem::create_directories(path.parent_path(), error);
            std::ofstream out(path, std::ios::binary | std::ios::trunc);
            if (!out) {
                return false;
            }
            out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            return static_cast<bool>(out);
        }

        uint16_t read16(const std::vector<uint8_t>& bytes, size_t at)
        {
            return static_cast<uint16_t>(bytes[at] | (bytes[at + 1] << 8));
        }

        uint32_t read32(const std::vector<uint8_t>& bytes, size_t at)
        {
            return static_cast<uint32_t>(bytes[at]) | (static_cast<uint32_t>(bytes[at + 1]) << 8) | (static_cast<uint32_t>(bytes[at + 2]) << 16) | (static_cast<uint32_t>(bytes[at + 3]) << 24);
        }

        void put16(std::vector<uint8_t>& bytes, uint32_t value)
        {
            bytes.push_back(static_cast<uint8_t>(value & 0xff));
            bytes.push_back(static_cast<uint8_t>((value >> 8) & 0xff));
        }

        void put32(std::vector<uint8_t>& bytes, uint32_t value)
        {
            put16(bytes, value & 0xffff);
            put16(bytes, value >> 16);
        }

        bool powerOfTwo(uint32_t value)
        {
            return (value & (value - 1)) == 0;
        }

        bool decodeImage(const std::vector<uint8_t>& bytes, Image& image, std::string& error)
        {
            if (bytes.size() >= 54 && bytes[0] == 'B' && bytes[1] == 'M') {
                uint32_t offBits = read32(bytes, 10);
                image.width = read32(bytes, 18);
                image.height = read32(bytes, 22);
                uint16_t planes = read16(bytes, 26);
                uint16_t bitCount = read16(bytes, 28);
                uint32_t compression = read32(bytes, 30);
                uint32_t colorsUsed = read32(bytes, 46);
                if (!powerOfTwo(image.width) || !powerOfTwo(image.height)) {
                    error = "Texture dimensions are not powers of two";
                    return false;
                }
                if (compression != 0) {
                    error = "RLE compression of BMP images not supported";
                    return false;
                }
                if (planes != 1 || bitCount != 8) {
                    error = "BMP uses an unsupported format (" + std::to_string(planes) + "/" + std::to_string(bitCount) + ")";
                    return false;
                }
                uint32_t used = std::min<uint32_t>(256, colorsUsed ? colorsUsed : 256);
                for (uint32_t i = 0; i < 256; i++) {
                    if (i < used) {
                        size_t at = 54 + i * 4;
                        image.palette[i] = { bytes[at + 2], bytes[at + 1], bytes[at], 255 };
                    } else {
                        image.palette[i] = { 0, 0, 0, 255 };
                    }
                }
                image.pixels.assign(static_cast<size_t>(image.width) * image.height, 0);
                uint32_t stride = (image.width + 3) & ~3u;
                for (uint32_t y = 0; y < image.height; y++) {
                    size_t from = offBits + static_cast<size_t>(y) * stride;
                    if (from + image.width > bytes.size()) {
                        error = "BMP data truncated";
                        return false;
                    }
                    std::memcpy(image.pixels.data() + static_cast<size_t>(image.height - 1 - y) * image.width, bytes.data() + from, image.width);
                }
                return true;
            }
            if (bytes.size() >= 128 + 768 && bytes[0] == 10) {
                image.width = static_cast<uint32_t>(read16(bytes, 8) + 1 - read16(bytes, 4));
                image.height = static_cast<uint32_t>(read16(bytes, 10) + 1 - read16(bytes, 6));
                uint8_t bitsPerPixel = bytes[3];
                uint8_t numPlanes = bytes[65];
                if (!powerOfTwo(image.width) || !powerOfTwo(image.height)) {
                    error = "Texture dimensions are not powers of two";
                    return false;
                }
                if (numPlanes != 1 || bitsPerPixel != 8) {
                    error = "PCX uses an unsupported format (" + std::to_string(numPlanes) + "/" + std::to_string(bitsPerPixel) + ")";
                    return false;
                }
                image.pixels.assign(static_cast<size_t>(image.width) * image.height, 0);
                size_t dest = 0;
                size_t at = 128;
                while (dest < image.pixels.size() && at < bytes.size()) {
                    uint8_t color = bytes[at++];
                    if ((color & 0xc0) == 0xc0 && at < bytes.size()) {
                        size_t run = color & 0x3f;
                        color = bytes[at++];
                        size_t count = std::min(run, image.pixels.size() - dest);
                        std::fill_n(image.pixels.begin() + static_cast<std::ptrdiff_t>(dest), count, color);
                        dest += run;
                    } else {
                        image.pixels[dest++] = color;
                    }
                }
                size_t paletteAt = bytes.size() - 768;
                for (size_t i = 0; i < 256; i++) {
                    image.palette[i] = { bytes[paletteAt + i * 3], bytes[paletteAt + i * 3 + 1], bytes[paletteAt + i * 3 + 2], 255 };
                }
                return true;
            }
            error = "Bad image format for texture import";
            return false;
        }

        std::vector<uint8_t> encodePcx(const Image& image)
        {
            std::vector<uint8_t> bytes;
            bytes.push_back(10);
            bytes.push_back(5);
            bytes.push_back(1);
            bytes.push_back(8);
            put16(bytes, 0);
            put16(bytes, 0);
            put16(bytes, image.width - 1);
            put16(bytes, image.height - 1);
            put16(bytes, image.width);
            put16(bytes, image.height);
            bytes.resize(bytes.size() + 48, 0);
            bytes.push_back(0);
            bytes.push_back(1);
            put16(bytes, image.width);
            put16(bytes, 0);
            put16(bytes, 0);
            put16(bytes, 0);
            bytes.resize(128, 0);
            for (uint32_t y = 0; y < image.height; y++) {
                const uint8_t* row = image.pixels.data() + static_cast<size_t>(y) * image.width;
                uint32_t x = 0;
                while (x < image.width) {
                    uint8_t value = row[x];
                    uint32_t run = 1;
                    while (x + run < image.width && run < 63 && row[x + run] == value) {
                        run++;
                    }
                    if (run > 1 || (value & 0xc0) == 0xc0) {
                        bytes.push_back(static_cast<uint8_t>(0xc0 | run));
                    }
                    bytes.push_back(value);
                    x += run;
                }
            }
            bytes.push_back(12);
            for (const Color& color : image.palette) {
                bytes.push_back(color.r);
                bytes.push_back(color.g);
                bytes.push_back(color.b);
            }
            return bytes;
        }

        std::vector<uint8_t> encodeBmp(const Image& image)
        {
            uint32_t stride = (image.width + 3) & ~3u;
            uint32_t offBits = 14 + 40 + 256 * 4;
            uint32_t imageSize = stride * image.height;
            std::vector<uint8_t> bytes;
            bytes.push_back('B');
            bytes.push_back('M');
            put32(bytes, offBits + imageSize);
            put16(bytes, 0);
            put16(bytes, 0);
            put32(bytes, offBits);
            put32(bytes, 40);
            put32(bytes, image.width);
            put32(bytes, image.height);
            put16(bytes, 1);
            put16(bytes, 8);
            put32(bytes, 0);
            put32(bytes, imageSize);
            put32(bytes, 0);
            put32(bytes, 0);
            put32(bytes, 256);
            put32(bytes, 256);
            for (const Color& color : image.palette) {
                bytes.push_back(color.b);
                bytes.push_back(color.g);
                bytes.push_back(color.r);
                bytes.push_back(0);
            }
            for (uint32_t y = 0; y < image.height; y++) {
                const uint8_t* row = image.pixels.data() + static_cast<size_t>(image.height - 1 - y) * image.width;
                bytes.insert(bytes.end(), row, row + image.width);
                bytes.resize(bytes.size() + (stride - image.width), 0);
            }
            return bytes;
        }

        bool writeImage(const Image& image, const std::filesystem::path& file)
        {
            std::string extension = file.extension().string();
            if (iequals(extension, ".bmp")) {
                return writeFile(file, encodeBmp(image));
            }
            return writeFile(file, encodePcx(image));
        }

        template <typename T>
        void setValue(UObject* object, const char* property, T value, int32_t index = 0)
        {
            uint8_t* address = object->propertyAddress(Name(property), index);
            if (address) {
                std::memcpy(address, &value, sizeof(T));
            }
        }

        template <typename T>
        T getValue(UObject* object, const char* property, int32_t index = 0)
        {
            T value {};
            uint8_t* address = object->propertyAddress(Name(property), index);
            if (address) {
                std::memcpy(&value, address, sizeof(T));
            }
            return value;
        }

        void setColor(UObject* object, const char* property, Color color)
        {
            uint8_t* address = object->propertyAddress(Name(property));
            if (address) {
                address[0] = color.r;
                address[1] = color.g;
                address[2] = color.b;
                address[3] = color.a;
            }
        }

        std::vector<UProperty*> polyFlagProperties(UObject* texture)
        {
            std::vector<UProperty*> result;
            UClass* cls = texture->cls;
            bool started = false;
            for (UProperty* property : cls->properties) {
                if (property->name == Name("bInvisible")) {
                    started = true;
                }
                if (started && property->type == PropType::Bool && result.size() < 32) {
                    result.push_back(property);
                }
            }
            return result;
        }

        uint32_t polyFlags(UObject* texture)
        {
            uint32_t flags = 0;
            std::vector<UProperty*> properties = polyFlagProperties(texture);
            for (size_t i = 0; i < properties.size(); i++) {
                uint32_t value = 0;
                std::memcpy(&value, texture->data + properties[i]->offset, sizeof(value));
                if (value) {
                    flags |= 1u << i;
                }
            }
            return flags;
        }

        void setPolyFlags(UObject* texture, uint32_t flags, std::vector<Name>& explicitProperties)
        {
            std::vector<UProperty*> properties = polyFlagProperties(texture);
            for (size_t i = 0; i < properties.size(); i++) {
                uint32_t value = (flags >> i) & 1u;
                std::memcpy(texture->data + properties[i]->offset, &value, sizeof(value));
                explicitProperties.push_back(properties[i]->name);
            }
        }

        UPalette* texturePalette(UObject* texture)
        {
            UObject* palette = getValue<UObject*>(texture, "Palette");
            objects::ensureLoaded(palette);
            return cast<UPalette>(palette);
        }

        UPalette* newPalette(UObject* outer, uint32_t flags)
        {
            UClass* cls = objects::intrinsicClass(Name("Palette"));
            Name name = uniqueObjectName(outer, "Palette");
            return cast<UPalette>(objects::newObject(cls, outer, name, flags));
        }

        UPalette* replaceWithExisting(UPalette* palette)
        {
            for (UObject* object : objects::all()) {
                UPalette* other = cast<UPalette>(object);
                if (!other || other == palette || other->destroyed || other->outer != palette->outer) {
                    continue;
                }
                objects::ensureLoaded(other);
                if (other->colors.size() >= 256 && std::equal(palette->colors.begin(), palette->colors.begin() + 256, other->colors.begin())) {
                    palette->destroyed = true;
                    return other;
                }
            }
            return palette;
        }

        uint8_t ceilLogTwo(uint32_t value)
        {
            uint8_t bits = 0;
            while ((1u << bits) < value) {
                bits++;
            }
            return bits;
        }

        TextureMip makeMip(uint32_t ubits, uint32_t vbits)
        {
            TextureMip mip;
            mip.ubits = static_cast<uint8_t>(ubits);
            mip.vbits = static_cast<uint8_t>(vbits);
            mip.width = 1u << ubits;
            mip.height = 1u << vbits;
            mip.data.assign(static_cast<size_t>(mip.width) * mip.height, 0);
            return mip;
        }

        void initTexture(UTexture* texture, uint32_t width, uint32_t height, std::vector<Name>& explicitProperties)
        {
            setValue<int32_t>(texture, "USize", static_cast<int32_t>(width));
            setValue<int32_t>(texture, "UClamp", static_cast<int32_t>(width));
            setValue<int32_t>(texture, "VSize", static_cast<int32_t>(height));
            setValue<int32_t>(texture, "VClamp", static_cast<int32_t>(height));
            uint8_t ubits = ceilLogTwo(width);
            uint8_t vbits = ceilLogTwo(height);
            setValue<uint8_t>(texture, "UBits", ubits);
            setValue<uint8_t>(texture, "VBits", vbits);
            texture->mips.clear();
            texture->mips.push_back(makeMip(ubits, vbits));
            for (const char* property : { "USize", "UClamp", "VSize", "VClamp", "UBits", "VBits" }) {
                explicitProperties.emplace_back(property);
            }
        }

        double appSeconds()
        {
            return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
        }

        void postLoadTexture(UTexture* texture, std::vector<Name>& explicitProperties)
        {
            if (!texturePalette(texture)) {
                UPalette* gray = newPalette(texture->outer, 0);
                gray->colors.clear();
                for (uint32_t i = 0; i < 256; i++) {
                    gray->colors.push_back(pack({ static_cast<uint8_t>(i), static_cast<uint8_t>(i), static_cast<uint8_t>(i), 0 }));
                }
                gray->destroyed = true;
            }
            int32_t usize = getValue<int32_t>(texture, "USize");
            int32_t vsize = getValue<int32_t>(texture, "VSize");
            setValue<int32_t>(texture, "UClamp", std::clamp(getValue<int32_t>(texture, "UClamp"), 0, usize));
            setValue<int32_t>(texture, "VClamp", std::clamp(getValue<int32_t>(texture, "VClamp"), 0, vsize));
            setValue<float>(texture, "Accumulator", 0.0f);
            double now = appSeconds();
            int32_t parts[2];
            std::memcpy(parts, &now, sizeof(now));
            setValue<int32_t>(texture, "InternalTime", parts[0], 0);
            setValue<int32_t>(texture, "InternalTime", parts[1], 1);
            explicitProperties.emplace_back("InternalTime");
        }

        UTexture* textureFactory(UObject* outer, Name name, uint32_t flags, const Image& image, std::vector<Name>& explicitProperties)
        {
            UClass* cls = textureClass();
            if (!cls) {
                return nullptr;
            }
            UTexture* texture = nullptr;
            if (name.none()) {
                name = uniqueObjectName(outer, "Texture");
            } else if (UObject* existing = findChild(outer, name, nullptr)) {
                texture = existing->cls == cls ? cast<UTexture>(existing) : nullptr;
                if (texture) {
                    cls->destroyValue(texture->data);
                    cls->initValue(texture->data);
                    cls->copyValue(texture->data, cls->defaultObject->data);
                    objects::syncObjectProperties(texture);
                    texture->flags = flags;
                    texture->compressedMips.clear();
                } else {
                    existing->destroyed = true;
                }
            }
            if (!texture) {
                texture = cast<UTexture>(objects::newObject(cls, outer, name, flags));
            }
            if (!texture) {
                return nullptr;
            }
            for (const char* property : { "Diffuse", "Specular", "DrawScale", "Friction", "MipMult" }) {
                setValue<float>(texture, property, 1.0f);
                explicitProperties.emplace_back(property);
            }
            setValue<float>(texture, "Alpha", 0.0f);
            explicitProperties.emplace_back("Alpha");
            initTexture(texture, image.width, image.height, explicitProperties);
            postLoadTexture(texture, explicitProperties);
            texture->mips[0].data = image.pixels;
            UPalette* palette = newPalette(outer, RF_Public);
            palette->colors.clear();
            for (const Color& color : image.palette) {
                palette->colors.push_back(pack(color));
            }
            setValue<UObject*>(texture, "Palette", replaceWithExisting(palette));
            explicitProperties.emplace_back("Palette");
            std::string textureName = texture->name.str();
            if (textureName.size() >= 4) {
                size_t end = textureName.size() - 4;
                if (textureName[end] == '_' && std::toupper(static_cast<unsigned char>(textureName[end + 1])) == 'A' && std::isdigit(static_cast<unsigned char>(textureName[end + 2])) && std::isdigit(static_cast<unsigned char>(textureName[end + 3]))) {
                    int frame = std::atoi(textureName.c_str() + end + 2);
                    char suffix[16];
                    if (frame > 0) {
                        std::snprintf(suffix, sizeof(suffix), "%02i", frame - 1);
                        UObject* other = findChild(outer, Name(textureName.substr(0, end + 2) + suffix), nullptr);
                        if (other && other->isA(cls)) {
                            setValue<UObject*>(other, "AnimNext", texture);
                        }
                    }
                    if (frame < 99) {
                        std::snprintf(suffix, sizeof(suffix), "%02i", frame + 1);
                        UObject* other = findChild(outer, Name(textureName.substr(0, end + 2) + suffix), nullptr);
                        if (other && other->isA(cls)) {
                            setValue<UObject*>(texture, "AnimNext", other);
                            explicitProperties.emplace_back("AnimNext");
                        }
                    }
                }
            }
            return texture;
        }

        uint8_t bestMatch(const UPalette* palette, Color color, int first)
        {
            int bestDelta = std::numeric_limits<int>::max();
            int bestUnscaledDelta = std::numeric_limits<int>::max();
            int bestColor = first;
            for (int test = first; test < 256; test++) {
                Color candidate = unpack(palette->colors[static_cast<size_t>(test)]);
                int greenDelta = (candidate.g - color.g) * (candidate.g - color.g);
                if (greenDelta < bestUnscaledDelta) {
                    int delta = 8 * greenDelta + 4 * (candidate.r - color.r) * (candidate.r - color.r) + (candidate.b - color.b) * (candidate.b - color.b);
                    if (delta < bestDelta) {
                        bestColor = test;
                        bestDelta = delta;
                        bestUnscaledDelta = (delta + 7) >> 3;
                    }
                }
            }
            return static_cast<uint8_t>(bestColor);
        }

        void createMips(UTexture* texture, bool fullMips, std::vector<Name>& explicitProperties)
        {
            UPalette* palette = texturePalette(texture);
            std::array<Color, 256> colors;
            for (size_t i = 0; i < 256; i++) {
                colors[i] = unpack(palette->colors[i]);
            }
            Vector3 sum;
            const std::vector<uint8_t>& base = texture->mips[0].data;
            for (uint8_t index : base) {
                accumulate(sum, plane(colors[index]));
            }
            setColor(texture, "MipZero", toColor(sum, static_cast<float>(base.size())));
            explicitProperties.emplace_back("MipZero");
            texture->mips.resize(1);
            if (!fullMips) {
                return;
            }
            int ubits = getValue<uint8_t>(texture, "UBits");
            int vbits = getValue<uint8_t>(texture, "VBits");
            while (ubits - static_cast<int>(texture->mips.size()) >= 0 || vbits - static_cast<int>(texture->mips.size()) >= 0) {
                int count = static_cast<int>(texture->mips.size());
                texture->mips.push_back(makeMip(static_cast<uint32_t>(std::max(ubits - count, 0)), static_cast<uint32_t>(std::max(vbits - count, 0))));
            }
            bool masked = (polyFlags(texture) & PolyFlagMasked) != 0;
            std::vector<Color> trueSource;
            std::vector<Color> trueDest;
            for (size_t level = 1; level < texture->mips.size(); level++) {
                const TextureMip& source = texture->mips[level - 1];
                TextureMip& dest = texture->mips[level];
                uint32_t tileU = source.width;
                uint32_t tileV = source.height;
                bool haveSource = level > 1;
                trueSource = trueDest;
                trueDest.assign(static_cast<size_t>(source.width) * source.height, Color());
                uint32_t maskU = tileU - 1;
                uint32_t maskV = tileV - 1;
                for (uint32_t v = 0; v < dest.height; v++) {
                    for (uint32_t u = 0; u < dest.width; u++) {
                        size_t texCoord = static_cast<size_t>(u) * 2 + static_cast<size_t>(v) * 2 * tileU;
                        size_t out = static_cast<size_t>(v) * dest.width + u;
                        if (!masked) {
                            size_t ud = 1 & maskU;
                            size_t vd = (1 & maskV) * tileU;
                            Vector3 c;
                            for (size_t offset : { size_t(0), ud, vd, ud + vd }) {
                                accumulate(c, plane(haveSource ? trueSource[texCoord + offset] : colors[source.data[texCoord + offset]]));
                            }
                            Color texel = toColor(c, 4.0f);
                            trueDest[out] = texel;
                            dest.data[out] = bestMatch(palette, texel, 0);
                        } else {
                            int found = 0;
                            Vector3 c;
                            for (uint32_t i = 0; i < 2; i++) {
                                for (uint32_t j = 0; j < 2; j++) {
                                    size_t at = texCoord + (i & maskU) + static_cast<size_t>(j & maskV) * tileU;
                                    uint8_t value = source.data[at];
                                    if (value) {
                                        found++;
                                        accumulate(c, plane(haveSource ? trueSource[at] : colors[value]));
                                    }
                                }
                            }
                            if (found >= 2) {
                                Color texel = toColor(c, static_cast<float>(found));
                                trueDest[out] = texel;
                                dest.data[out] = bestMatch(palette, texel, 1);
                            } else {
                                trueDest[out] = Color();
                                dest.data[out] = 0;
                            }
                        }
                    }
                }
            }
        }

        void createColorRange(UTexture* texture, std::vector<Name>& explicitProperties)
        {
            UPalette* palette = texturePalette(texture);
            Color maxColor { 255, 255, 255, 255 };
            if (palette) {
                maxColor = Color();
                for (const TextureMip& mip : texture->mips) {
                    for (uint8_t index : mip.data) {
                        Color color = unpack(palette->colors[index]);
                        maxColor.r = std::max(maxColor.r, color.r);
                        maxColor.g = std::max(maxColor.g, color.g);
                        maxColor.b = std::max(maxColor.b, color.b);
                        maxColor.a = std::max(maxColor.a, color.a);
                    }
                }
            }
            setColor(texture, "MaxColor", maxColor);
            explicitProperties.emplace_back("MaxColor");
        }

        UObject* importOuter(ExecContext& context, std::string_view text, bool useGroup)
        {
            UObject* outer = context.package;
            std::string packageName;
            if (parseString(text, "Package=", packageName)) {
                outer = objects::packageObject(Name(packageName));
            }
            std::string group;
            if (useGroup && parseString(text, "GROUP=", group) && !iequals(group, "None")) {
                UClass* packageClass = objects::intrinsicClass(Name("Package"));
                UObject* groupObject = findChild(outer, Name(group), packageClass);
                if (!groupObject) {
                    groupObject = objects::newObject(packageClass, outer, Name(group), RF_Public);
                }
                outer = groupObject;
            }
            return outer;
        }

        UObject* findOuter(ExecContext& context, std::string_view text, bool useGroup)
        {
            UObject* outer = context.package;
            std::string packageName;
            if (parseString(text, "Package=", packageName)) {
                outer = objects::packageObject(Name(packageName));
            }
            std::string group;
            if (useGroup && parseString(text, "GROUP=", group) && !iequals(group, "None")) {
                outer = findChild(outer, Name(group), nullptr);
            }
            return outer;
        }

        std::string objectNameFor(std::string_view text, const std::string& file)
        {
            std::string name;
            if (parseString(text, "Name=", name)) {
                return name;
            }
            std::string path = file;
            std::replace(path.begin(), path.end(), '\\', '/');
            name = path.substr(path.find_last_of('/') == std::string::npos ? 0 : path.find_last_of('/') + 1);
            return name.substr(0, name.find('.'));
        }

        UObject* findTexture(std::string_view text)
        {
            return objects::findObject(std::string(text), textureClass());
        }

        void parseTextureReference(std::string_view text, const char* key, UObject* texture, const char* property, std::vector<Name>& explicitProperties)
        {
            std::string value;
            if (!parseString(text, key, value)) {
                return;
            }
            if (iequals(value, "None")) {
                setValue<UObject*>(texture, property, nullptr);
                explicitProperties.emplace_back(property);
                return;
            }
            UObject* found = findTexture(value);
            if (found) {
                setValue<UObject*>(texture, property, found);
                explicitProperties.emplace_back(property);
            }
        }

        bool loadImage(ExecContext& context, const std::string& file, Image& image)
        {
            std::vector<uint8_t> bytes;
            std::filesystem::path path = resolveFile(context.directory, file);
            if (!readFile(path, bytes)) {
                context.log("Warning: Can't open file " + path.string());
                return false;
            }
            std::string error;
            if (!decodeImage(bytes, image, error)) {
                context.log("Warning: " + error + " (" + path.string() + ")");
                return false;
            }
            return true;
        }

        void importTexture(ExecContext& context, std::string_view text)
        {
            std::string file;
            if (!parseString(text, "File=", file)) {
                context.log("Warning: Missing file or name");
                return;
            }
            seedCounters(context.package);
            std::string name = objectNameFor(text, file);
            bool doMips = true;
            parseBool(text, "Mips=", doMips);
            UObject* outer = importOuter(context, text, true);
            Image image;
            if (!loadImage(context, file, image)) {
                context.log("Warning: Import texture " + name + " from " + file + " failed");
                return;
            }
            Tracked entry;
            UTexture* texture = textureFactory(outer, Name(name), RF_Public | RF_Standalone, image, entry.properties);
            if (!texture) {
                context.log("Warning: Import texture " + name + " from " + file + " failed");
                return;
            }
            entry.object = texture;
            uint8_t lodSet = 0;
            if (parseByte(text, "LODSet=", lodSet)) {
                setValue<uint8_t>(texture, "LODSet", lodSet);
                entry.properties.emplace_back("LODSet");
            }
            int32_t flags = 0;
            if (parseInt(text, "FLAGS=", flags)) {
                setPolyFlags(texture, static_cast<uint32_t>(flags), entry.properties);
            }
            parseTextureReference(text, "DETAIL=", texture, "DetailTexture", entry.properties);
            parseTextureReference(text, "MTEX=", texture, "MacroTexture", entry.properties);
            parseTextureReference(text, "NEXT=", texture, "AnimNext", entry.properties);
            createMips(texture, doMips, entry.properties);
            createColorRange(texture, entry.properties);
            bool alphaTrick = false;
            parseBool(text, "ALPHATRICK=", alphaTrick);
            if (alphaTrick) {
                UPalette* palette = texturePalette(texture);
                for (uint32_t& color : palette->colors) {
                    Color value = unpack(color);
                    value.a = value.b;
                    color = pack(value);
                }
            }
            tracked().push_back(std::move(entry));
        }

        bool scanFontBox(const TextureMip& mip, int x, int y, int& xl, int& yl)
        {
            auto at = [&](int px, int py) -> int {
                long index = static_cast<long>(px) + static_cast<long>(py) * static_cast<long>(mip.width);
                if (index < 0 || index >= static_cast<long>(mip.data.size())) {
                    return 0;
                }
                return mip.data[static_cast<size_t>(index)];
            };
            int newXl = 1;
            while (at(x + newXl, y) == 255 && at(x + newXl, y + 1) != 255) {
                newXl++;
            }
            if (at(x + newXl, y) != 255) {
                return false;
            }
            int newYl = 1;
            while (at(x, y + newYl) == 255 && at(x + 1, y + newYl) != 255) {
                newYl++;
            }
            if (at(x, y + newYl) != 255) {
                return false;
            }
            xl = newXl - 1;
            yl = newYl - 1;
            return true;
        }

        std::vector<FontCharacter> scanFont(const TextureMip& mip)
        {
            std::vector<FontCharacter> characters(FontCharacters);
            int width = static_cast<int>(mip.width);
            int height = static_cast<int>(mip.height);
            int i = 32;
            int y = 0;
            do {
                int x = 0;
                while (y < height && mip.data[static_cast<size_t>(x + y * width)] != 255) {
                    x++;
                    if (x >= width) {
                        x = 0;
                        if (++y >= height) {
                            break;
                        }
                    }
                }
                if (y < height) {
                    int xl = 0;
                    int yl = 0;
                    int maxYl = 0;
                    while (i < FontCharacters && scanFontBox(mip, x, y, xl, yl)) {
                        characters[static_cast<size_t>(i)] = { x + 1, y + 1, xl, yl };
                        x += xl + 1;
                        i++;
                        maxYl = std::max(maxYl, yl);
                    }
                    y += maxYl + 1;
                }
            } while (i < FontCharacters && y < height);
            std::vector<FontCharacter> remapped(FontCharacters);
            for (size_t c = 0; c < remapped.size(); c++) {
                remapped[c] = characters[fontRemap[c]];
            }
            return remapped;
        }

        UFont* fontFactory(UObject* outer, Name name, uint32_t flags, const Image& image, Tracked& entry)
        {
            if (UObject* existing = findChild(outer, name, nullptr)) {
                existing->destroyed = true;
            }
            UFont* font = cast<UFont>(objects::newObject(objects::intrinsicClass(Name("Font")), outer, name, flags));
            if (!font) {
                return nullptr;
            }
            font->charactersPerPage = FontCharacters;
            font->pages.clear();
            font->pages.emplace_back();
            FontPage& page = font->pages.back();
            UTexture* texture = textureFactory(font, Name(), 0, image, entry.properties);
            if (!texture) {
                return nullptr;
            }
            page.texture = texture;
            entry.object = texture;
            setPolyFlags(texture, PolyFlagMasked, entry.properties);
            page.characters = scanFont(texture->mips[0]);
            for (TextureMip& mip : texture->mips) {
                std::replace(mip.data.begin(), mip.data.end(), uint8_t(255), uint8_t(0));
            }
            return font;
        }

        void importFont(ExecContext& context, std::string_view text)
        {
            std::string file;
            parseString(text, "File=", file);
            std::string name = objectNameFor(text, file);
            if (file.empty()) {
                context.log("Warning: Font import missing FILE=");
                return;
            }
            seedCounters(context.package);
            UObject* outer = importOuter(context, text, false);
            Image image;
            if (!loadImage(context, file, image)) {
                context.log("Warning: Failed factoring: FONT IMPORT " + std::string(text));
                return;
            }
            Tracked entry;
            if (!fontFactory(outer, Name(name), RF_Public | RF_Standalone, image, entry)) {
                context.log("Warning: Failed factoring: FONT IMPORT " + std::string(text));
                return;
            }
            tracked().push_back(std::move(entry));
        }

        std::filesystem::path trueTypeMetrics(const std::filesystem::path& directory, const std::string& name)
        {
            return directory / "Fonts" / (name + ".txt");
        }

        std::filesystem::path trueTypePage(const std::filesystem::path& directory, const std::string& name, size_t page)
        {
            return directory / "Fonts" / (name + "_" + std::to_string(page) + ".pcx");
        }

        void importTrueTypeFont(ExecContext& context, std::string_view text)
        {
            std::string name;
            parseString(text, "Name=", name);
            int32_t perPage = 64;
            parseInt(text, "CharactersPerPage=", perPage);
            bool antiAlias = false;
            parseBool(text, "AntiAlias=", antiAlias);
            std::ifstream metrics(trueTypeMetrics(context.directory, name));
            if (name.empty() || perPage <= 0 || !metrics) {
                context.log("Warning: TrueTypeFontFactory needs the glyphs of " + name + " in " + trueTypeMetrics(context.directory, name).string() + ", font was not created");
                return;
            }
            seedCounters(context.package);
            UObject* outer = importOuter(context, text, false);
            if (UObject* existing = findChild(outer, Name(name), nullptr)) {
                existing->destroyed = true;
            }
            UFont* font = cast<UFont>(objects::newObject(objects::intrinsicClass(Name("Font")), outer, Name(name), RF_Public | RF_Standalone));
            font->charactersPerPage = static_cast<uint32_t>(perPage);
            font->pages.clear();
            UPalette* palette = newPalette(font, RF_Public);
            palette->colors.clear();
            for (uint32_t i = 0; i < 256; i++) {
                uint32_t level = antiAlias ? i : (i ? 255u : 0u);
                palette->colors.push_back(level | (level << 8) | (level << 16) | (level << 24));
            }
            FontCharacter character;
            size_t index = 0;
            while (metrics >> character.u >> character.v >> character.width >> character.height) {
                size_t page = index / static_cast<size_t>(perPage);
                if (page >= font->pages.size()) {
                    Image image;
                    if (!loadImage(context, trueTypePage(context.directory, name, page).string(), image)) {
                        font->destroyed = true;
                        return;
                    }
                    Tracked entry;
                    UTexture* texture = cast<UTexture>(objects::newObject(textureClass(), font, uniqueObjectName(font, "Texture"), 0));
                    initTexture(texture, image.width, image.height, entry.properties);
                    postLoadTexture(texture, entry.properties);
                    texture->mips[0].data = image.pixels;
                    setPolyFlags(texture, PolyFlagMasked, entry.properties);
                    setValue<UObject*>(texture, "Palette", palette);
                    entry.properties.emplace_back("Palette");
                    entry.object = texture;
                    tracked().push_back(std::move(entry));
                    font->pages.emplace_back();
                    font->pages.back().texture = texture;
                }
                font->pages.back().characters.push_back(character);
                index++;
            }
        }

        UClass* textureClassNamed(const std::string& name)
        {
            if (name.find('.') != std::string::npos) {
                return objects::findClass(name);
            }
            for (const char* package : { "Engine.", "Fire.", "DeusEx." }) {
                if (UClass* cls = objects::findClass(package + name)) {
                    return cls;
                }
            }
            return nullptr;
        }

        void lagunaPalette(UPalette* palette)
        {
            palette->colors.clear();
            const int bases[4][3] = { { 0, 0, 0 }, { 59, 67, 100 }, { 114, 127, 197 }, { 178, 187, 230 } };
            const int steps[4][3] = { { 59, 67, 100 }, { 55, 60, 97 }, { 64, 60, 33 }, { 78, 69, 26 } };
            for (int band = 0; band < 4; band++) {
                for (int i = 0; i < 64; i++) {
                    Color color;
                    color.r = static_cast<uint8_t>(std::min(bases[band][0] + i * steps[band][0] / 64, 255));
                    color.g = static_cast<uint8_t>(std::min(bases[band][1] + i * steps[band][1] / 64, 255));
                    color.b = static_cast<uint8_t>(std::min(bases[band][2] + i * steps[band][2] / 64, 255));
                    palette->colors.push_back(pack(color));
                }
            }
        }

        void grayPalette(UPalette* palette)
        {
            palette->colors.clear();
            for (uint32_t i = 0; i < 256; i++) {
                palette->colors.push_back(pack({ static_cast<uint8_t>(i), static_cast<uint8_t>(i), static_cast<uint8_t>(i), 0 }));
            }
        }

        void initFractal(UTexture* texture, uint32_t width, uint32_t height)
        {
            setValue<uint32_t>(texture, "bParametric", 1);
            setValue<uint32_t>(texture, "bRealtime", 1);
            setValue<int32_t>(texture, "UMask", static_cast<int32_t>(width) - 1);
            setValue<int32_t>(texture, "VMask", static_cast<int32_t>(height) - 1);
            UPalette* palette = nullptr;
            if (texture->isA(Name("FireTexture")) || texture->isA(Name("WaveTexture")) || texture->isA(Name("WetTexture"))) {
                palette = newPalette(texture->outer, RF_Public);
                lagunaPalette(palette);
            } else if (texture->isA(Name("IceTexture"))) {
                palette = newPalette(texture->outer, RF_Public);
                grayPalette(palette);
            }
            if (palette) {
                setValue<UObject*>(texture, "Palette", palette);
                setColor(texture, "MipZero", unpack(palette->colors[128]));
            }
            if (texture->isA(Name("FireTexture"))) {
                setValue<uint8_t>(texture, "RenderHeat", 220);
                setValue<int32_t>(texture, "NumSparks", 0);
                setValue<uint8_t>(texture, "SparkType", 4);
                setValue<int32_t>(texture, "SparksLimit", 1024);
                for (const auto& [property, value] : { std::pair<const char*, uint8_t> { "FX_Frequency", 16 }, { "FX_Phase", 16 }, { "FX_Heat", 255 }, { "FX_Size", 96 }, { "FX_Area", 24 }, { "FX_HorizSpeed", 130 }, { "FX_VertSpeed", 142 } }) {
                    setValue<uint8_t>(texture, property, value);
                }
            }
            if (texture->isA(Name("WaterTexture"))) {
                setValue<int32_t>(texture, "NumDrops", 0);
                setValue<uint8_t>(texture, "DropType", 1);
                setValue<uint8_t>(texture, "WaveAmp", 128);
                for (const auto& [property, value] : { std::pair<const char*, uint8_t> { "FX_Frequency", 8 }, { "FX_Phase", 8 }, { "FX_Amplitude", 255 }, { "FX_Speed", 255 }, { "FX_Radius", 128 }, { "FX_Size", 30 }, { "FX_Depth", 255 }, { "FX_Time", 30 } }) {
                    setValue<uint8_t>(texture, property, value);
                }
            }
            if (texture->isA(Name("WaveTexture"))) {
                for (const auto& [property, value] : { std::pair<const char*, uint8_t> { "BumpMapAngle", 170 }, { "BumpMapLight", 50 }, { "PhongRange", 180 }, { "PhongSize", 32 } }) {
                    setValue<uint8_t>(texture, property, value);
                }
            }
            if (texture->isA(Name("IceTexture"))) {
                setValue<float>(texture, "UDisplace", 0.0f);
                setValue<float>(texture, "VDisplace", 0.0f);
                setValue<uint8_t>(texture, "HorizPanSpeed", 128);
                setValue<uint8_t>(texture, "VertPanSpeed", 128);
                setValue<uint8_t>(texture, "PanningStyle", 0);
                setValue<uint8_t>(texture, "Frequency", 11);
                setValue<uint8_t>(texture, "Amplitude", 44);
                setValue<uint32_t>(texture, "MoveIce", 1);
            }
        }

        void newTexture(ExecContext& context, std::string_view text)
        {
            std::string name;
            std::string className;
            std::string packageName;
            int32_t usize = 0;
            int32_t vsize = 0;
            if (!parseString(text, "NAME=", name) || !parseString(text, "CLASS=", className) || !parseInt(text, "USIZE=", usize) || !parseInt(text, "VSIZE=", vsize) || !parseString(text, "PACKAGE=", packageName) || iequals(packageName, "None")) {
                context.log("Warning: Bad TEXTURE NEW");
                return;
            }
            UClass* cls = textureClassNamed(className);
            if (!cls || !textureClass() || !cls->isChildOf(textureClass()) || usize <= 0 || vsize <= 0 || usize > 1024 || vsize > 1024 || !powerOfTwo(static_cast<uint32_t>(usize)) || !powerOfTwo(static_cast<uint32_t>(vsize))) {
                context.log("Warning: Bad TEXTURE NEW");
                return;
            }
            UObject* outer = importOuter(context, text, true);
            if (findChild(outer, Name(name), cls)) {
                context.log("Warning: Texture exists");
                return;
            }
            UTexture* texture = cast<UTexture>(objects::newObject(cls, outer, Name(name), RF_Public | RF_Standalone));
            if (!texture) {
                context.log("Warning: Bad TEXTURE NEW");
                return;
            }
            texture->loaded = true;
            if (!texturePalette(texture)) {
                UPalette* palette = newPalette(outer, RF_Public);
                palette->colors.assign(256, 0);
                setValue<UObject*>(texture, "Palette", palette);
            }
            std::vector<Name> explicitProperties;
            initTexture(texture, static_cast<uint32_t>(usize), static_cast<uint32_t>(vsize), explicitProperties);
            if (texture->isA(Name("FractalTexture"))) {
                initFractal(texture, static_cast<uint32_t>(usize), static_cast<uint32_t>(vsize));
            }
            if (texture->isA(Name("FireTexture"))) {
                setValue<uint32_t>(texture, "bMasked", 0);
            }
        }

        bool importCommand(ExecContext& context, const std::string& command)
        {
            std::string_view text = command;
            std::string_view texture = text;
            if (parseCommand(texture, "TEXTURE")) {
                if (parseCommand(texture, "NEW")) {
                    newTexture(context, texture);
                    return true;
                }
                if (!parseCommand(texture, "IMPORT")) {
                    return false;
                }
                importTexture(context, texture);
                return true;
            }
            std::string_view font = text;
            if (parseCommand(font, "FONT")) {
                if (!parseCommand(font, "IMPORT")) {
                    return false;
                }
                importFont(context, font);
                return true;
            }
            std::string_view factory = text;
            if (parseCommand(factory, "NEW")) {
                if (!parseCommand(factory, "STANDALONE") && !parseCommand(factory, "PUBLIC")) {
                    parseCommand(factory, "PRIVATE");
                }
                if (parseCommand(factory, "FONTFACTORY")) {
                    importFont(context, factory);
                    return true;
                }
                if (parseCommand(factory, "TRUETYPEFONTFACTORY")) {
                    importTrueTypeFont(context, factory);
                    return true;
                }
            }
            return false;
        }

        Image textureImage(UTexture* texture)
        {
            objects::ensureLoaded(texture);
            Image image;
            const TextureMip& mip = texture->mips.at(0);
            image.width = mip.width;
            image.height = mip.height;
            image.pixels = mip.data;
            UPalette* palette = texturePalette(texture);
            for (size_t i = 0; i < 256; i++) {
                image.palette[i] = palette && i < palette->colors.size() ? unpack(palette->colors[i]) : Color { static_cast<uint8_t>(i), static_cast<uint8_t>(i), static_cast<uint8_t>(i), 255 };
            }
            return image;
        }

        struct Box {
            int x = 0;
            int y = 0;
            int width = 0;
            int height = 0;
        };

        bool clearRect(const Image& image, int x0, int y0, int x1, int y1)
        {
            if (x0 < 0 || y0 < 0 || x1 >= static_cast<int>(image.width) || y1 >= static_cast<int>(image.height)) {
                return false;
            }
            for (int y = y0; y <= y1; y++) {
                for (int x = x0; x <= x1; x++) {
                    if (image.pixels[static_cast<size_t>(x + y * static_cast<int>(image.width))] != 0) {
                        return false;
                    }
                }
            }
            return true;
        }

        bool splitRun(const Image& image, int left, int right, int top, int innerHeight, int count, bool freeRight, std::vector<Box>& boxes)
        {
            int bottom = top + innerHeight + 1;
            int limit = freeRight ? static_cast<int>(image.width) - 2 : right - 1;
            int wanted = freeRight ? count : count - 1;
            std::vector<int> borders { left };
            for (int x = left + 1; x <= limit && static_cast<int>(borders.size()) <= wanted; x++) {
                if (clearRect(image, x, top, x, bottom)) {
                    borders.push_back(x);
                }
            }
            if (!freeRight) {
                borders.push_back(right);
            }
            if (static_cast<int>(borders.size()) != count + 1) {
                return false;
            }
            if (!clearRect(image, left, bottom, borders.back(), bottom) || !clearRect(image, left, top, borders.back(), top)) {
                return false;
            }
            for (int i = 0; i < count; i++) {
                boxes.push_back({ borders[static_cast<size_t>(i)], top, borders[static_cast<size_t>(i) + 1] - borders[static_cast<size_t>(i)] - 1, innerHeight });
            }
            return true;
        }

        bool fontBoxes(const Image& image, const std::vector<FontCharacter>& characters, std::vector<Box>& boxes)
        {
            std::vector<bool> known(FontCharacters, false);
            std::vector<FontCharacter> old(FontCharacters);
            for (int i = 0; i < FontCharacters; i++) {
                int j = fontRemap[i];
                const FontCharacter& character = characters[static_cast<size_t>(i)];
                if (j >= 32 && (character.u || character.v)) {
                    known[static_cast<size_t>(j)] = true;
                    old[static_cast<size_t>(j)] = character;
                }
            }
            int last = 31;
            for (int j = 32; j < FontCharacters; j++) {
                if (known[static_cast<size_t>(j)]) {
                    last = j;
                }
            }
            int j = 32;
            while (j <= last) {
                if (known[static_cast<size_t>(j)]) {
                    const FontCharacter& c = old[static_cast<size_t>(j)];
                    boxes.push_back({ c.u - 1, c.v - 1, c.width, c.height });
                    j++;
                    continue;
                }
                int end = j;
                while (!known[static_cast<size_t>(end)]) {
                    end++;
                }
                int count = end - j;
                if (j == 32) {
                    return false;
                }
                const FontCharacter& prev = old[static_cast<size_t>(j - 1)];
                const FontCharacter& next = old[static_cast<size_t>(end)];
                int prevRight = prev.u + prev.width;
                if (prev.v == next.v) {
                    if (!splitRun(image, prevRight, next.u - 1, prev.v - 1, prev.height, count, false, boxes)) {
                        return false;
                    }
                } else {
                    int lineLeft = prev.u - 1;
                    for (const Box& box : boxes) {
                        if (box.y == prev.v - 1) {
                            lineLeft = std::min(lineLeft, box.x);
                        }
                    }
                    bool placed = false;
                    for (int atStart = next.u - 1 > lineLeft ? count : 0; atStart >= 0 && !placed; atStart--) {
                        std::vector<Box> after;
                        std::vector<Box> before;
                        bool tail = atStart == count || splitRun(image, prevRight, 0, prev.v - 1, prev.height, count - atStart, true, after);
                        bool head = atStart == 0 ? next.u - 1 == lineLeft : splitRun(image, lineLeft, next.u - 1, next.v - 1, next.height, atStart, false, before);
                        if (tail && head) {
                            boxes.insert(boxes.end(), after.begin(), after.end());
                            boxes.insert(boxes.end(), before.begin(), before.end());
                            placed = true;
                        }
                    }
                    if (!placed) {
                        return false;
                    }
                }
                j = end;
            }
            return true;
        }

        void drawBox(Image& image, const Box& box)
        {
            int right = box.x + box.width + 1;
            int bottom = box.y + box.height + 1;
            for (int x = box.x; x <= right; x++) {
                image.pixels[static_cast<size_t>(x + box.y * static_cast<int>(image.width))] = 255;
                image.pixels[static_cast<size_t>(x + bottom * static_cast<int>(image.width))] = 255;
            }
            for (int y = box.y; y <= bottom; y++) {
                image.pixels[static_cast<size_t>(box.x + y * static_cast<int>(image.width))] = 255;
                image.pixels[static_cast<size_t>(right + y * static_cast<int>(image.width))] = 255;
            }
        }

        bool sameCharacters(const std::vector<FontCharacter>& a, const std::vector<FontCharacter>& b)
        {
            for (size_t i = 0; i < a.size() && i < b.size(); i++) {
                if (a[i].u != b[i].u || a[i].v != b[i].v || a[i].width != b[i].width || a[i].height != b[i].height) {
                    return false;
                }
            }
            return a.size() == b.size();
        }

        bool fontImage(UFont* font, Image& image, std::string& error)
        {
            objects::ensureLoaded(font);
            if (font->pages.size() != 1 || !font->pages[0].texture) {
                error = "font has " + std::to_string(font->pages.size()) + " pages";
                return false;
            }
            const FontPage& page = font->pages[0];
            image = textureImage(page.texture);
            if (std::find(image.pixels.begin(), image.pixels.end(), uint8_t(255)) != image.pixels.end()) {
                error = "font texture contains color 255";
                return false;
            }
            std::vector<Box> boxes;
            if (!fontBoxes(image, page.characters, boxes)) {
                error = "cannot reconstruct the glyph grid";
                return false;
            }
            for (const Box& box : boxes) {
                drawBox(image, box);
            }
            TextureMip check;
            check.width = image.width;
            check.height = image.height;
            check.data = image.pixels;
            if (!sameCharacters(scanFont(check), page.characters)) {
                error = "reconstructed glyph grid does not rescan identically";
                return false;
            }
            return true;
        }

        bool exportTrueTypeFont(ExecContext& context, std::string_view text)
        {
            std::string name;
            parseString(text, "Name=", name);
            UFont* font = cast<UFont>(findChild(context.package, Name(name), nullptr));
            if (!font) {
                context.log("Warning: font " + name + " not found");
                return false;
            }
            objects::ensureLoaded(font);
            std::string metrics;
            for (size_t page = 0; page < font->pages.size(); page++) {
                UTexture* texture = font->pages[page].texture;
                UPalette* palette = texture ? texturePalette(texture) : nullptr;
                if (!palette) {
                    context.log("Warning: font " + name + " page " + std::to_string(page) + " has no texture");
                    return false;
                }
                Image image = textureImage(texture);
                for (size_t i = 0; i < image.palette.size() && i < palette->colors.size(); i++) {
                    image.palette[i] = unpack(palette->colors[i]);
                }
                if (!writeImage(image, trueTypePage(context.directory, name, page))) {
                    return false;
                }
                for (const FontCharacter& character : font->pages[page].characters) {
                    metrics += std::to_string(character.u) + " " + std::to_string(character.v) + " " + std::to_string(character.width) + " " + std::to_string(character.height) + "\n";
                }
            }
            return writeFile(trueTypeMetrics(context.directory, name), std::vector<uint8_t>(metrics.begin(), metrics.end()));
        }

        bool exportSourceCommand(ExecContext& context, const std::string& command)
        {
            std::string_view text = command;
            std::string_view texture = text;
            std::string_view font = text;
            std::string_view factory = text;
            bool isTexture = parseCommand(texture, "TEXTURE") && parseCommand(texture, "IMPORT");
            bool isFont = parseCommand(font, "FONT") && parseCommand(font, "IMPORT");
            bool isNew = parseCommand(factory, "NEW");
            std::string_view trueType = factory;
            if (isNew && parseCommand(trueType, "TRUETYPEFONTFACTORY")) {
                return exportTrueTypeFont(context, trueType);
            }
            bool isFactory = isNew && parseCommand(factory, "FONTFACTORY");
            if (isFactory) {
                font = factory;
                isFont = true;
            }
            if (!isTexture && !isFont) {
                return false;
            }
            std::string_view parameters = isTexture ? texture : font;
            std::string file;
            if (!parseString(parameters, "File=", file)) {
                return false;
            }
            std::string name = objectNameFor(parameters, file);
            UObject* outer = findOuter(context, parameters, isTexture);
            UObject* object = outer ? findChild(outer, Name(name), nullptr) : nullptr;
            Image image;
            if (isTexture) {
                UTexture* found = cast<UTexture>(object);
                if (!found) {
                    context.log("Warning: texture " + name + " not found");
                    return false;
                }
                image = textureImage(found);
            } else {
                UFont* found = cast<UFont>(object);
                std::string error;
                if (!found || !fontImage(found, image, error)) {
                    context.log("Warning: font " + name + ": " + (found ? error : std::string("not found")));
                    return false;
                }
            }
            std::filesystem::path path = sourcePath(context.directory, file);
            if (!writeImage(image, path)) {
                context.log("Warning: cannot write " + path.string());
                return false;
            }
            return true;
        }

        bool exportTextureObject(UObject* object, const std::string& extension, const std::filesystem::path& file)
        {
            UTexture* texture = cast<UTexture>(object);
            if (!texture || (extension != "pcx" && extension != "bmp")) {
                return false;
            }
            objects::ensureLoaded(texture);
            if (texture->mips.empty() || texture->format() != 0) {
                return false;
            }
            Image image = textureImage(texture);
            return extension == "bmp" ? writeFile(file, encodeBmp(image)) : writeFile(file, encodePcx(image));
        }

        void writeMips(PackageWriter& package, Writer& out, const std::vector<TextureMip>& mips)
        {
            out.index(static_cast<int32_t>(mips.size()));
            for (const TextureMip& mip : mips) {
                size_t skip = package.beginLazyArray(out);
                out.index(static_cast<int32_t>(mip.data.size()));
                out.bytes(mip.data.data(), mip.data.size());
                package.endLazyArray(out, skip);
                out.u32(mip.width);
                out.u32(mip.height);
                out.u8(mip.ubits);
                out.u8(mip.vbits);
            }
        }

        bool serializeObject(PackageWriter& package, UObject* object, Writer& out)
        {
            if (!object->cls) {
                return false;
            }
            Name className = object->cls->name;
            if (className == Name("Palette")) {
                UPalette* palette = cast<UPalette>(object);
                if (!palette) {
                    return false;
                }
                out.index(static_cast<int32_t>(palette->colors.size()));
                for (uint32_t color : palette->colors) {
                    out.u32(color);
                }
                return true;
            }
            if (UTexture* texture = cast<UTexture>(object)) {
                if (getValue<uint32_t>(texture, "bParametric")) {
                    std::vector<TextureMip> empty = texture->mips;
                    for (TextureMip& mip : empty) {
                        mip.data.clear();
                    }
                    writeMips(package, out, empty);
                } else {
                    writeMips(package, out, texture->mips);
                }
                if (getValue<uint32_t>(texture, "bHasComp")) {
                    writeMips(package, out, texture->compressedMips);
                }
                if (UFireTexture* fire = cast<UFireTexture>(texture)) {
                    out.index(static_cast<int32_t>(fire->sparks.size()));
                    for (const FireSpark& spark : fire->sparks) {
                        for (uint8_t value : { spark.type, spark.heat, spark.x, spark.y, spark.a, spark.b, spark.c, spark.d }) {
                            out.u8(value);
                        }
                    }
                }
                return true;
            }
            if (className == Name("Font")) {
                UFont* font = cast<UFont>(object);
                if (!font) {
                    return false;
                }
                out.index(static_cast<int32_t>(font->pages.size()));
                for (const FontPage& page : font->pages) {
                    package.writeObject(out, page.texture);
                    out.index(static_cast<int32_t>(page.characters.size()));
                    for (const FontCharacter& character : page.characters) {
                        out.i32(character.u);
                        out.i32(character.v);
                        out.i32(character.width);
                        out.i32(character.height);
                    }
                }
                out.u32(font->charactersPerPage);
                return true;
            }
            return false;
        }

        void finishImports(ExecContext&)
        {
            for (const Tracked& entry : tracked()) {
                UObject* object = entry.object;
                if (!object || object->destroyed || !object->cls || !object->cls->defaultObject) {
                    continue;
                }
                std::vector<std::pair<UProperty*, std::vector<uint8_t>>> saved;
                for (Name name : entry.properties) {
                    UProperty* property = object->cls->findProperty(name);
                    if (!property) {
                        continue;
                    }
                    std::vector<uint8_t> bytes(property->totalSize());
                    std::memcpy(bytes.data(), object->data + property->offset, bytes.size());
                    saved.emplace_back(property, std::move(bytes));
                }
                object->cls->copyValue(object->data, object->cls->defaultObject->data);
                for (const auto& [property, bytes] : saved) {
                    std::memcpy(object->data + property->offset, bytes.data(), bytes.size());
                }
            }
            tracked().clear();
        }
    }

    AssetHandler textureAssets()
    {
        AssetHandler handler;
        handler.import = importCommand;
        handler.exportSource = exportSourceCommand;
        handler.exportObject = exportTextureObject;
        handler.serialize = serializeObject;
        handler.finish = finishImports;
        return handler;
    }
}
