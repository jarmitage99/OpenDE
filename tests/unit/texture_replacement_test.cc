#include "core/objects.h"
#include "engine/resources.h"
#include "engine/world.h"
#include "render/procedural.h"
#include "render/texture_replacement.h"

#include <catch_amalgamated.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {
    uint32_t crc32(const uint8_t* data, size_t size)
    {
        uint32_t crc = 0xffffffffu;
        for (size_t i = 0; i < size; i++) {
            crc ^= data[i];
            for (int bit = 0; bit < 8; bit++) {
                crc = (crc & 1u) ? (crc >> 1) ^ 0xedb88320u : crc >> 1;
            }
        }
        return crc ^ 0xffffffffu;
    }

    void putBig(std::vector<uint8_t>& out, uint32_t value)
    {
        out.push_back(static_cast<uint8_t>(value >> 24));
        out.push_back(static_cast<uint8_t>(value >> 16));
        out.push_back(static_cast<uint8_t>(value >> 8));
        out.push_back(static_cast<uint8_t>(value));
    }

    void putChunk(std::vector<uint8_t>& out, const char* type, const std::vector<uint8_t>& data)
    {
        putBig(out, static_cast<uint32_t>(data.size()));
        std::vector<uint8_t> body(type, type + 4);
        body.insert(body.end(), data.begin(), data.end());
        out.insert(out.end(), body.begin(), body.end());
        putBig(out, crc32(body.data(), body.size()));
    }

    std::vector<uint8_t> encodePng(uint32_t width, uint32_t height, const std::vector<uint8_t>& rgba)
    {
        std::vector<uint8_t> raw;
        for (uint32_t y = 0; y < height; y++) {
            raw.push_back(0);
            raw.insert(raw.end(), rgba.begin() + static_cast<std::ptrdiff_t>(y * width * 4), rgba.begin() + static_cast<std::ptrdiff_t>((y + 1) * width * 4));
        }
        std::vector<uint8_t> zlib = { 0x78, 0x01, 0x01 };
        uint16_t length = static_cast<uint16_t>(raw.size());
        zlib.push_back(static_cast<uint8_t>(length));
        zlib.push_back(static_cast<uint8_t>(length >> 8));
        zlib.push_back(static_cast<uint8_t>(~length));
        zlib.push_back(static_cast<uint8_t>(static_cast<uint16_t>(~length) >> 8));
        zlib.insert(zlib.end(), raw.begin(), raw.end());
        uint32_t a = 1;
        uint32_t b = 0;
        for (uint8_t byte : raw) {
            a = (a + byte) % 65521u;
            b = (b + a) % 65521u;
        }
        putBig(zlib, (b << 16) | a);
        std::vector<uint8_t> png = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n' };
        std::vector<uint8_t> header;
        putBig(header, width);
        putBig(header, height);
        header.insert(header.end(), { 8, 6, 0, 0, 0 });
        putChunk(png, "IHDR", header);
        putChunk(png, "IDAT", zlib);
        putChunk(png, "IEND", {});
        return png;
    }

    std::vector<uint8_t> solid(uint32_t width, uint32_t height, std::array<uint8_t, 4> color)
    {
        std::vector<uint8_t> rgba(static_cast<size_t>(width) * height * 4);
        for (size_t i = 0; i < rgba.size(); i++) {
            rgba[i] = color[i % 4];
        }
        return rgba;
    }

    void writePng(const std::filesystem::path& path, uint32_t width, uint32_t height, const std::vector<uint8_t>& rgba)
    {
        std::filesystem::create_directories(path.parent_path());
        std::vector<uint8_t> png = encodePng(width, height, rgba);
        std::ofstream file(path, std::ios::binary);
        file.write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size()));
    }

    dx::UTexture* loadTexture(const char* path, const char* className)
    {
        dx::engine::initialize(DEUSEX_GAME_DATA);
        std::string text(path);
        REQUIRE(dx::objects::loadPackage(dx::Name(text.substr(0, text.find('.')))) != nullptr);
        dx::UObject* object = dx::objects::findObject(path, dx::objects::findClass(className));
        REQUIRE(object != nullptr);
        dx::objects::ensureLoaded(object);
        dx::objects::flushLoads();
        dx::UTexture* texture = dx::cast<dx::UTexture>(object);
        REQUIRE(texture != nullptr);
        return texture;
    }

    std::filesystem::path freshRoot(const char* name)
    {
        std::filesystem::path root = std::filesystem::temp_directory_path() / name;
        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root);
        return root;
    }
}

TEST_CASE("an HD replacement PNG decodes to its exact RGBA pixels and size")
{
    std::vector<uint8_t> pixels = {
        255, 0, 0, 255, 0, 255, 0, 128, 0, 0, 255, 0,
        10, 20, 30, 40, 250, 240, 230, 220, 1, 2, 3, 4
    };
    dx::ReplacementImage image;
    REQUIRE(dx::decodePng(encodePng(3, 2, pixels), image));
    CHECK(image.width == 3);
    CHECK(image.height == 2);
    CHECK(image.rgba == pixels);
    dx::ReplacementImage broken;
    CHECK_FALSE(dx::decodePng({ 1, 2, 3, 4, 5 }, broken));
    CHECK(broken.rgba.empty());
}

TEST_CASE("HD replacements are found by package group and name, ignoring case, with the user directory first", "[data]")
{
    dx::UTexture* texture = loadTexture("CoreTexMetal.Metal.StanGrayMetal_A", "Engine.Texture");
    REQUIRE(texture->outer->name == dx::Name("Metal"));
    uint32_t width = texture->width();
    uint32_t height = texture->height();
    std::filesystem::path root = freshRoot("dx-hd-replacement-test");
    std::filesystem::path user = root / "user";
    std::filesystem::path game = root / "game";
    dx::TextureReplacements replacements({ user, game });
    CHECK(replacements.find(texture).empty());

    std::filesystem::path grouped = game / "TEXTURES" / "hd" / "coretexmetal" / "METAL" / "stanGrayMetal_a.PNG";
    std::vector<uint8_t> green = solid(8, 4, { 0, 255, 0, 255 });
    writePng(grouped, 8, 4, green);
    CHECK(replacements.find(texture).empty());
    replacements.rescan();
    CHECK(replacements.find(texture) == grouped);
    dx::ReplacementImage image;
    REQUIRE(replacements.load(texture, image));
    CHECK(image.width == 8);
    CHECK(image.height == 4);
    CHECK(image.rgba == green);

    std::filesystem::path ungrouped = user / "Textures" / "HD" / "CoreTexMetal" / "StanGrayMetal_A.png";
    std::vector<uint8_t> blue = solid(2, 2, { 0, 0, 255, 128 });
    writePng(ungrouped, 2, 2, blue);
    replacements.rescan();
    CHECK(replacements.find(texture) == ungrouped);
    REQUIRE(replacements.load(texture, image));
    CHECK(image.width == 2);
    CHECK(image.rgba == blue);
    CHECK(texture->width() == width);
    CHECK(texture->height() == height);

    std::filesystem::remove(ungrouped);
    replacements.rescan();
    CHECK(replacements.find(texture) == grouped);
    std::filesystem::remove_all(root);
}

TEST_CASE("procedural textures and font pages are never replaced by HD images", "[data]")
{
    dx::UTexture* fire = loadTexture("Effects.LaserSpot2", "Fire.FireTexture");
    REQUIRE(dx::ProceduralTextures::isProcedural(fire));
    dx::UFont* font = dx::cast<dx::UFont>(dx::objects::findObject("Engine.SmallFont", nullptr));
    REQUIRE(font != nullptr);
    dx::objects::ensureLoaded(font);
    dx::objects::flushLoads();
    REQUIRE_FALSE(font->pages.empty());
    dx::UTexture* page = font->pages[0].texture;
    REQUIRE(page != nullptr);
    REQUIRE(page->outer == font);
    dx::UTexture* plain = loadTexture("CoreTexMetal.Metal.StanGrayMetal_A", "Engine.Texture");

    std::filesystem::path root = freshRoot("dx-hd-excluded-test");
    std::filesystem::path hd = root / "Textures" / "HD";
    std::vector<uint8_t> red = solid(4, 4, { 255, 0, 0, 255 });
    writePng(hd / "Effects" / (fire->outer->name.str()) / "LaserSpot2.png", 4, 4, red);
    writePng(hd / "Effects" / "LaserSpot2.png", 4, 4, red);
    writePng(hd / "Engine" / "SmallFont" / (page->name.str() + ".png"), 4, 4, red);
    writePng(hd / "Engine" / (page->name.str() + ".png"), 4, 4, red);
    writePng(hd / "CoreTexMetal" / "StanGrayMetal_A.png", 4, 4, red);
    dx::TextureReplacements replacements({ root });

    CHECK_FALSE(dx::TextureReplacements::replaceable(fire));
    CHECK_FALSE(dx::TextureReplacements::replaceable(page));
    CHECK(dx::TextureReplacements::replaceable(plain));
    CHECK(replacements.find(fire).empty());
    CHECK(replacements.find(page).empty());
    dx::ReplacementImage image;
    CHECK_FALSE(replacements.load(fire, image));
    CHECK_FALSE(replacements.load(page, image));
    CHECK(replacements.find(plain) == hd / "CoreTexMetal" / "StanGrayMetal_A.png");
    std::filesystem::remove_all(root);
}
