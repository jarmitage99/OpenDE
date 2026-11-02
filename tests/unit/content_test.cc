#include "audio/audio.h"
#include "compiler/asset_package.h"
#include "compiler/package_dump.h"
#include "compiler/script_package.h"
#include "core/config.h"
#include "core/objects.h"
#include "engine/mesh.h"
#include "engine/package.h"
#include "engine/resources.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

namespace {
    std::string lowerText(std::string text)
    {
        for (char& c : text) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return text;
    }

    std::filesystem::path scratch(const std::string& name)
    {
        std::filesystem::path directory = dx::config::userDirectory() / "content-test" / name;
        std::filesystem::create_directories(directory);
        return directory;
    }

    void writeFile(const std::filesystem::path& file, const std::vector<uint8_t>& bytes)
    {
        std::ofstream out(file, std::ios::binary);
        out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }

    std::vector<uint8_t> readFile(const std::filesystem::path& file)
    {
        std::ifstream in(file, std::ios::binary);
        return std::vector<uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }

    void put16(std::vector<uint8_t>& bytes, uint32_t value)
    {
        bytes.push_back(static_cast<uint8_t>(value));
        bytes.push_back(static_cast<uint8_t>(value >> 8));
    }

    void put32(std::vector<uint8_t>& bytes, uint32_t value)
    {
        put16(bytes, value & 0xffff);
        put16(bytes, value >> 16);
    }

    std::vector<uint8_t> bitmap(uint32_t size, uint8_t (*pixel)(uint32_t x, uint32_t y))
    {
        std::vector<uint8_t> bytes = { 'B', 'M' };
        put32(bytes, 54 + 1024 + size * size);
        put32(bytes, 0);
        put32(bytes, 54 + 1024);
        put32(bytes, 40);
        put32(bytes, size);
        put32(bytes, size);
        put16(bytes, 1);
        put16(bytes, 8);
        put32(bytes, 0);
        put32(bytes, size * size);
        put32(bytes, 2835);
        put32(bytes, 2835);
        put32(bytes, 256);
        put32(bytes, 0);
        for (uint32_t i = 0; i < 256; i++) {
            bytes.push_back(static_cast<uint8_t>(255 - i));
            bytes.push_back(static_cast<uint8_t>(i / 2));
            bytes.push_back(static_cast<uint8_t>(i));
            bytes.push_back(0);
        }
        for (uint32_t row = 0; row < size; row++) {
            uint32_t y = size - 1 - row;
            for (uint32_t x = 0; x < size; x++) {
                bytes.push_back(pixel(x, y));
            }
        }
        return bytes;
    }

    uint8_t checker(uint32_t x, uint32_t y)
    {
        return ((x / 4 + y / 4) % 2) ? 200 : 17;
    }

    uint8_t gradient(uint32_t x, uint32_t y)
    {
        return static_cast<uint8_t>(x * 16 + y);
    }

    void discard(dx::UObject* package)
    {
        for (dx::UObject* object : dx::compiler::packageContents(package)) {
            object->destroyed = true;
        }
    }

    dx::UObject* reload(const std::string& packageName, const std::filesystem::path& file, const char* objectName, const char* className)
    {
        dx::Linker* linker = dx::objects::loadPackageFile(dx::Name(packageName), file, false);
        REQUIRE(linker != nullptr);
        int32_t ref = linker->findExport(dx::Name(objectName), dx::Name(className), "*");
        REQUIRE(ref != 0);
        dx::UObject* object = linker->object(ref);
        dx::objects::ensureLoaded(object);
        dx::objects::flushLoads();
        return object;
    }

    template <typename T>
    T propertyValue(dx::UObject* object, const char* property)
    {
        T value {};
        uint8_t* address = object->propertyAddress(dx::Name(property));
        REQUIRE(address != nullptr);
        std::memcpy(&value, address, sizeof(T));
        return value;
    }

    bool run(dx::UObject* package, const std::filesystem::path& directory, const std::string& command)
    {
        std::vector<std::string> log;
        bool ok = dx::compiler::runAssetCommand(package, directory, command, log);
        for (const std::string& line : log) {
            UNSCOPED_INFO(line);
        }
        return ok;
    }
}

TEST_CASE("imported BMP textures with mips, masking and detail survive a utx save and reload", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    std::filesystem::path directory = scratch("textures");
    writeFile(directory / "Checker.bmp", bitmap(16, checker));
    writeFile(directory / "Ramp.bmp", bitmap(16, gradient));
    dx::UObject* package = dx::objects::packageObject(dx::Name("ContentTexTest"));
    REQUIRE(run(package, directory, "TEXTURE IMPORT FILE=Ramp.bmp NAME=Ramp PACKAGE=ContentTexTest GROUP=Detail MIPS=0"));
    REQUIRE(run(package, directory, "TEXTURE IMPORT FILE=Checker.bmp NAME=Checker PACKAGE=ContentTexTest GROUP=Walls MIPS=1 FLAGS=2 DETAIL=ContentTexTest.Detail.Ramp"));
    std::filesystem::path file = dx::compiler::assetPackagePath(package);
    CHECK(file.extension() == ".utx");
    CHECK(file.parent_path().filename() == "Textures");
    std::string error;
    REQUIRE(dx::compiler::saveAssetPackage(package, file, &error));
    discard(package);

    dx::UTexture* checkerTexture = dx::cast<dx::UTexture>(reload("ContentTexTest", file, "Checker", "Texture"));
    REQUIRE(checkerTexture != nullptr);
    CHECK(checkerTexture->outer->name.str() == "Walls");
    REQUIRE(checkerTexture->mips.size() == 5);
    CHECK(checkerTexture->mips[0].width == 16);
    CHECK(checkerTexture->mips[4].width == 1);
    for (uint32_t y = 0; y < 16; y++) {
        for (uint32_t x = 0; x < 16; x++) {
            REQUIRE(checkerTexture->mips[0].data[y * 16 + x] == checker(x, y));
        }
    }
    CHECK(propertyValue<uint32_t>(checkerTexture, "bMasked") != 0);
    dx::UObject* detail = propertyValue<dx::UObject*>(checkerTexture, "DetailTexture");
    REQUIRE(detail != nullptr);
    CHECK(detail->name.str() == "Ramp");
    dx::UPalette* palette = checkerTexture->palette();
    REQUIRE(palette != nullptr);
    REQUIRE(palette->colors.size() == 256);
    std::vector<uint8_t> rgba = checkerTexture->rgba(0);
    REQUIRE(rgba.size() == 16 * 16 * 4);
    CHECK(rgba[0] == 17);
    CHECK(rgba[1] == 8);
    CHECK(rgba[2] == 238);
    CHECK(rgba[4 * 4] == 200);
    CHECK(rgba[4 * 4 + 1] == 100);
    CHECK(rgba[4 * 4 + 2] == 55);

    dx::UTexture* ramp = dx::cast<dx::UTexture>(reload("ContentTexTest", file, "Ramp", "Texture"));
    REQUIRE(ramp != nullptr);
    CHECK(ramp->mips.size() == 1);
    CHECK(ramp->mips[0].data[3 * 16 + 5] == 83);
}

TEST_CASE("TEXTURE NEW creates fire and wave textures that save and reload with their fractal settings", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::UObject* package = dx::objects::packageObject(dx::Name("ContentFireTest"));
    REQUIRE(run(package, {}, "TEXTURE NEW NAME=Flames CLASS=FireTexture PACKAGE=ContentFireTest GROUP=Fx USIZE=128 VSIZE=64"));
    REQUIRE(run(package, {}, "TEXTURE NEW NAME=Ripples CLASS=Fire.WaveTexture PACKAGE=ContentFireTest USIZE=64 VSIZE=64"));
    REQUIRE(run(package, {}, "TEXTURE NEW NAME=Plain CLASS=Texture PACKAGE=ContentFireTest USIZE=32 VSIZE=32"));
    CHECK_FALSE(run(package, {}, "TEXTURE NEW NAME=Flames CLASS=FireTexture PACKAGE=ContentFireTest GROUP=Fx USIZE=128 VSIZE=64"));
    CHECK_FALSE(run(package, {}, "TEXTURE NEW NAME=Odd CLASS=FireTexture PACKAGE=ContentFireTest USIZE=100 VSIZE=64"));
    CHECK_FALSE(run(package, {}, "TEXTURE NEW NAME=Actor CLASS=Actor PACKAGE=ContentFireTest USIZE=64 VSIZE=64"));
    std::filesystem::path file = scratch("fire") / "ContentFireTest.utx";
    std::string error;
    REQUIRE(dx::compiler::saveAssetPackage(package, file, &error));
    discard(package);

    dx::UTexture* fire = dx::cast<dx::UTexture>(reload("ContentFireTest", file, "Flames", "FireTexture"));
    REQUIRE(fire != nullptr);
    CHECK(dx::cast<dx::UFireTexture>(fire) != nullptr);
    CHECK(fire->outer->name.str() == "Fx");
    REQUIRE(fire->mips.size() == 1);
    CHECK(fire->mips[0].width == 128);
    CHECK(fire->mips[0].height == 64);
    CHECK(fire->mips[0].data.empty());
    CHECK(propertyValue<int32_t>(fire, "USize") == 128);
    CHECK(propertyValue<uint8_t>(fire, "UBits") == 7);
    CHECK(propertyValue<uint8_t>(fire, "VBits") == 6);
    CHECK(propertyValue<uint8_t>(fire, "RenderHeat") == 220);
    CHECK(propertyValue<int32_t>(fire, "SparksLimit") == 1024);
    CHECK(propertyValue<uint8_t>(fire, "SparkType") == 4);
    CHECK(propertyValue<uint8_t>(fire, "FX_VertSpeed") == 142);
    CHECK(propertyValue<uint32_t>(fire, "bParametric") != 0);
    dx::UPalette* palette = fire->palette();
    REQUIRE(palette != nullptr);
    REQUIRE(palette->colors.size() == 256);
    std::vector<uint8_t> expected = { 114, 127, 197, 178 + 63 * 78 / 64, 187 + 63 * 69 / 64, 230 + 63 * 26 / 64 };
    const uint8_t* band = reinterpret_cast<const uint8_t*>(&palette->colors[128]);
    const uint8_t* top = reinterpret_cast<const uint8_t*>(&palette->colors[255]);
    CHECK(std::vector<uint8_t>({ band[0], band[1], band[2], top[0], top[1], top[2] }) == expected);

    dx::UObject* wave = reload("ContentFireTest", file, "Ripples", "WaveTexture");
    CHECK(propertyValue<uint8_t>(wave, "BumpMapAngle") == 170);
    CHECK(propertyValue<uint8_t>(wave, "PhongSize") == 32);
    CHECK(propertyValue<uint8_t>(wave, "WaveAmp") == 128);

    dx::UTexture* plain = dx::cast<dx::UTexture>(reload("ContentFireTest", file, "Plain", "Texture"));
    REQUIRE(plain != nullptr);
    REQUIRE(plain->mips.size() == 1);
    CHECK(plain->mips[0].data.size() == 32 * 32);
}

TEST_CASE("saving shipped texture and sound packages reproduces every export", "[data]")
{
    auto [folder, packageName, className, minimum] = GENERATE(table<std::string, std::string, std::string, int>({
        { "Textures", "Effects.utx", "FireTexture", 30 },
        { "Textures", "Effects.utx", "WetTexture", 1 },
        { "Textures", "Effects.utx", "Texture", 20 },
        { "Textures", "DXFonts.utx", "Font", 2 },
        { "Sounds", "MoverSFX.uax", "Sound", 60 },
    }));
    dx::engine::initialize(DEUSEX_GAME_DATA);
    std::string stem = std::filesystem::path(packageName).stem().string();
    dx::UObject* package = dx::objects::packageObject(dx::Name(stem));
    REQUIRE(dx::objects::loadPackage(dx::Name(stem)) != nullptr);
    std::filesystem::path file = scratch("shipped") / packageName;
    std::string error;
    REQUIRE(dx::compiler::saveAssetPackage(package, file, &error));
    std::filesystem::path system = std::filesystem::path(DEUSEX_GAME_DATA) / "System";
    auto expected = dx::compiler::dumpPackage(std::filesystem::path(DEUSEX_GAME_DATA) / folder / packageName, system);
    std::map<std::string, dx::compiler::DumpedExport> actual;
    for (auto& [path, entry] : dx::compiler::dumpPackage(file, system)) {
        actual[lowerText(path)] = entry;
    }
    CHECK(actual.size() == expected.size());
    int matching = 0;
    std::vector<std::string> mismatches;
    for (auto& [path, entry] : expected) {
        auto it = actual.find(lowerText(path));
        if (entry.className == className) {
            matching++;
        }
        if (it == actual.end() || it->second.className != entry.className || lowerText(it->second.body) != lowerText(entry.body)) {
            mismatches.push_back(path);
        }
    }
    CHECK(matching >= minimum);
    for (size_t i = 0; i < std::min<size_t>(mismatches.size(), 3); i++) {
        UNSCOPED_INFO(mismatches[i] + "\n" + expected[mismatches[i]].body.substr(0, 600));
    }
    CHECK(mismatches.empty());
}

TEST_CASE("imported WAV sounds survive a uax save and reload", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    std::filesystem::path directory = scratch("sounds");
    std::vector<uint8_t> wave = { 'R', 'I', 'F', 'F' };
    put32(wave, 36 + 200);
    for (char c : std::string("WAVEfmt ")) {
        wave.push_back(static_cast<uint8_t>(c));
    }
    put32(wave, 16);
    put16(wave, 1);
    put16(wave, 1);
    put32(wave, 11025);
    put32(wave, 22050);
    put16(wave, 2);
    put16(wave, 16);
    for (char c : std::string("data")) {
        wave.push_back(static_cast<uint8_t>(c));
    }
    put32(wave, 200);
    for (uint32_t i = 0; i < 100; i++) {
        put16(wave, (i % 2) ? 8000 : 0xE0C0);
    }
    writeFile(directory / "Beep.wav", wave);
    dx::UObject* package = dx::objects::packageObject(dx::Name("ContentSndTest"));
    REQUIRE(run(package, directory, "AUDIO IMPORT FILE=Beep.wav NAME=Beep PACKAGE=ContentSndTest GROUP=Alerts"));
    std::filesystem::path file = dx::compiler::assetPackagePath(package);
    CHECK(file.extension() == ".uax");
    CHECK(file.parent_path().filename() == "Sounds");
    std::string error;
    REQUIRE(dx::compiler::saveAssetPackage(package, file, &error));
    discard(package);

    dx::USound* sound = dx::cast<dx::USound>(reload("ContentSndTest", file, "Beep", "Sound"));
    REQUIRE(sound != nullptr);
    CHECK(sound->outer->name.str() == "Alerts");
    CHECK(sound->format == dx::Name("WAV"));
    CHECK(sound->bytes == wave);
    std::shared_ptr<const dx::audio::DecodedSound> pcm = dx::audio::decodeBytes(sound->bytes);
    REQUIRE(pcm != nullptr);
    CHECK(pcm->sampleRate == 11025);
    CHECK(pcm->frames() == 100);
    CHECK(dx::objects::loadPackage(dx::Name("ContentSndTest")) != nullptr);
}

TEST_CASE("imported _a.3d and _d.3d meshes with sequences survive a package save and reload", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    std::filesystem::path directory = scratch("meshes");
    const int16_t corners[4][3] = { { 0, 0, 0 }, { 100, 0, 0 }, { 0, 100, 0 }, { 0, 0, 100 } };
    std::vector<uint8_t> aniv;
    put16(aniv, 2);
    put16(aniv, 4 * 8);
    for (int frame = 0; frame < 2; frame++) {
        for (const auto& corner : corners) {
            for (int16_t value : corner) {
                put16(aniv, static_cast<uint16_t>(value * (frame + 1)));
            }
            put16(aniv, 0);
        }
    }
    const uint16_t faces[4][3] = { { 0, 1, 2 }, { 0, 1, 3 }, { 0, 2, 3 }, { 1, 2, 3 } };
    std::vector<uint8_t> data;
    put16(data, 4);
    put16(data, 4);
    data.resize(48, 0);
    for (const auto& face : faces) {
        for (uint16_t vertex : face) {
            put16(data, vertex);
        }
        data.push_back(0);
        data.push_back(0);
        for (int k = 0; k < 3; k++) {
            data.push_back(static_cast<uint8_t>(face[k] * 40));
            data.push_back(static_cast<uint8_t>(face[k] * 20));
        }
        data.push_back(0);
        data.push_back(0);
    }
    writeFile(directory / "Tetra_a.3d", aniv);
    writeFile(directory / "Tetra_d.3d", data);
    dx::UObject* package = dx::objects::packageObject(dx::Name("ContentMeshTest"));
    REQUIRE(run(package, directory, "MESH IMPORT MESH=Tetra ANIVFILE=Tetra_a.3d DATAFILE=Tetra_d.3d MLOD=0"));
    REQUIRE(run(package, directory, "MESH SEQUENCE MESH=Tetra SEQ=All STARTFRAME=0 NUMFRAMES=2"));
    REQUIRE(run(package, directory, "MESH SEQUENCE MESH=Tetra SEQ=Grow STARTFRAME=1 NUMFRAMES=1 RATE=10"));
    REQUIRE(run(package, directory, "MESHMAP SCALE MESHMAP=Tetra X=0.5 Y=0.25 Z=2"));
    REQUIRE(run(package, directory, "MESH IMPORT MESH=TetraLod ANIVFILE=Tetra_a.3d DATAFILE=Tetra_d.3d MLOD=1"));
    std::filesystem::path file = dx::compiler::assetPackagePath(package);
    CHECK(file.extension() == ".u");
    std::string error;
    REQUIRE(dx::compiler::saveAssetPackage(package, file, &error));
    discard(package);

    dx::UMesh* mesh = dx::cast<dx::UMesh>(reload("ContentMeshTest", file, "Tetra", "Mesh"));
    REQUIRE(mesh != nullptr);
    CHECK(mesh->frameVerts == 4);
    CHECK(mesh->animFrames == 2);
    REQUIRE(mesh->verts.size() == 8);
    CHECK(mesh->verts[7].z == Catch::Approx(200.0f).margin(1.0f));
    CHECK(mesh->verts[5].x == Catch::Approx(200.0f).margin(1.0f));
    REQUIRE(mesh->tris.size() == 4);
    std::vector<std::vector<uint16_t>> loaded;
    for (const dx::MeshTri& tri : mesh->tris) {
        std::vector<uint16_t> vertices(tri.vertex, tri.vertex + 3);
        std::sort(vertices.begin(), vertices.end());
        loaded.push_back(vertices);
    }
    std::sort(loaded.begin(), loaded.end());
    CHECK(loaded == std::vector<std::vector<uint16_t>>({ { 0, 1, 2 }, { 0, 1, 3 }, { 0, 2, 3 }, { 1, 2, 3 } }));
    REQUIRE(mesh->sequences.size() == 2);
    const dx::AnimSequence* grow = mesh->findSequence(dx::Name("Grow"));
    REQUIRE(grow != nullptr);
    CHECK(grow->startFrame == 1);
    CHECK(grow->numFrames == 1);
    CHECK(grow->rate == Catch::Approx(10.0f));
    CHECK(mesh->scale.x == Catch::Approx(0.5f));
    CHECK(mesh->scale.y == Catch::Approx(0.25f));
    CHECK(mesh->scale.z == Catch::Approx(2.0f));

    dx::ULodMesh* lod = dx::cast<dx::ULodMesh>(reload("ContentMeshTest", file, "TetraLod", "LodMesh"));
    REQUIRE(lod != nullptr);
    CHECK(lod->faces.size() == 4);
    CHECK(lod->triangles().size() == 4);
}
