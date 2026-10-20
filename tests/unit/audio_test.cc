#include "audio/audio.h"
#include "audio/module.h"
#include "core/objects.h"
#include "core/vm.h"
#include "engine/actor.h"
#include "engine/console.h"
#include "engine/props_engine.h"
#include "engine/package.h"
#include "engine/resources.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

namespace {
    std::vector<uint8_t> resourceBytes(const std::string& path, const char* className, const char* objectName)
    {
        dx::Package package(path);
        int32_t ref = package.findExport(className, objectName);
        REQUIRE(ref != 0);
        dx::Reader reader = package.load(ref).native();
        reader.index();
        reader.u32();
        int32_t size = reader.index();
        std::vector<uint8_t> bytes(static_cast<size_t>(size));
        reader.bytes(bytes.data(), bytes.size());
        return bytes;
    }

    void loadSound(dx::USound& sound, const char* name)
    {
        sound.bytes = resourceBytes(DEUSEX_GAME_DATA "/System/DeusExSounds.u", "Sound", name);
    }

    std::vector<uint8_t> sampleWave(const std::vector<int16_t>& samples, uint32_t rate)
    {
        uint32_t frames = static_cast<uint32_t>(samples.size());
        std::vector<uint8_t> bytes;
        auto u32 = [&bytes](uint32_t v) {
            for (int i = 0; i < 4; i++) {
                bytes.push_back(static_cast<uint8_t>(v >> (8 * i)));
            }
        };
        auto u16 = [&bytes](uint16_t v) {
            bytes.push_back(static_cast<uint8_t>(v));
            bytes.push_back(static_cast<uint8_t>(v >> 8));
        };
        bytes.insert(bytes.end(), { 'R', 'I', 'F', 'F' });
        u32(36 + frames * 2);
        bytes.insert(bytes.end(), { 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ' });
        u32(16);
        u16(1);
        u16(1);
        u32(rate);
        u32(rate * 2);
        u16(2);
        u16(16);
        bytes.insert(bytes.end(), { 'd', 'a', 't', 'a' });
        u32(frames * 2);
        for (int16_t sample : samples) {
            u16(static_cast<uint16_t>(sample));
        }
        return bytes;
    }

    std::vector<uint8_t> loopWave(const std::vector<int16_t>& samples, uint32_t rate, uint32_t start, uint32_t last)
    {
        std::vector<uint8_t> bytes = sampleWave(samples, rate);
        std::vector<uint32_t> chunk(15, 0);
        chunk[7] = 1;
        chunk[11] = start;
        chunk[12] = last;
        bytes.insert(bytes.end(), { 's', 'm', 'p', 'l', 60, 0, 0, 0 });
        for (uint32_t value : chunk) {
            for (int i = 0; i < 4; i++) {
                bytes.push_back(static_cast<uint8_t>(value >> (8 * i)));
            }
        }
        uint32_t riff = static_cast<uint32_t>(bytes.size() - 8);
        for (int i = 0; i < 4; i++) {
            bytes[4 + static_cast<size_t>(i)] = static_cast<uint8_t>(riff >> (8 * i));
        }
        return bytes;
    }

    std::vector<uint8_t> constantWave(int16_t value, uint32_t rate, uint32_t frames)
    {
        return sampleWave(std::vector<int16_t>(frames, value), rate);
    }

    void renderFrames(size_t frames)
    {
        std::vector<float> buffer(4096 * 2);
        while (frames > 0) {
            size_t chunk = std::min<size_t>(frames, 4096);
            dx::audio::render(buffer.data(), chunk);
            frames -= chunk;
        }
    }

    std::shared_ptr<const dx::audio::ItModule> trainingMusic()
    {
        std::vector<uint8_t> bytes = resourceBytes(DEUSEX_GAME_DATA "/Music/Training_Music.umx", "Music", "Training_Music");
        auto module = std::make_shared<dx::audio::ItModule>();
        REQUIRE(module->load(bytes.data(), bytes.size()));
        return module;
    }

    struct TrackerCell {
        int channel = 0;
        int note = -1;
        int instrument = 0;
        int volume = -1;
        int command = 0;
        int param = 0;
    };

    struct TrackerSample {
        std::vector<int16_t> data;
        uint8_t flags = 0x13;
        uint32_t loopBegin = 0;
        uint32_t loopEnd = 0;
        uint32_t sustainBegin = 0;
        uint32_t sustainEnd = 0;
        uint8_t globalVolume = 64;
        uint8_t volume = 64;
    };

    std::vector<uint8_t> trackerModule(const std::vector<std::vector<TrackerCell>>& rows, const TrackerSample& sample)
    {
        std::vector<uint8_t> bytes(0xC0, 0);
        auto put16 = [&bytes](size_t at, uint32_t v) {
            bytes[at] = static_cast<uint8_t>(v);
            bytes[at + 1] = static_cast<uint8_t>(v >> 8);
        };
        auto put32 = [&bytes](size_t at, uint32_t v) {
            for (size_t i = 0; i < 4; i++) {
                bytes[at + i] = static_cast<uint8_t>(v >> (8 * i));
            }
        };
        std::memcpy(bytes.data(), "IMPM", 4);
        put16(0x20, 2);
        put16(0x22, 0);
        put16(0x24, 1);
        put16(0x26, 1);
        put16(0x28, 0x217);
        put16(0x2A, 0x200);
        put16(0x2C, 0x09);
        bytes[0x30] = 128;
        bytes[0x31] = 48;
        bytes[0x32] = 6;
        bytes[0x33] = 125;
        bytes[0x34] = 128;
        for (size_t i = 0; i < 64; i++) {
            bytes[0x40 + i] = 32;
            bytes[0x80 + i] = 64;
        }
        bytes.push_back(0);
        bytes.push_back(255);
        size_t sampleTable = bytes.size();
        bytes.resize(bytes.size() + 8, 0);
        size_t sampleHeader = bytes.size();
        bytes.resize(bytes.size() + 0x50, 0);
        std::memcpy(bytes.data() + sampleHeader, "IMPS", 4);
        bytes[sampleHeader + 0x11] = sample.globalVolume;
        bytes[sampleHeader + 0x12] = sample.flags;
        bytes[sampleHeader + 0x13] = sample.volume;
        bytes[sampleHeader + 0x2E] = 1;
        bytes[sampleHeader + 0x2F] = 32;
        put32(sampleHeader + 0x30, static_cast<uint32_t>(sample.data.size()));
        put32(sampleHeader + 0x34, sample.loopBegin);
        put32(sampleHeader + 0x38, sample.loopEnd);
        put32(sampleHeader + 0x3C, 44100);
        put32(sampleHeader + 0x40, sample.sustainBegin);
        put32(sampleHeader + 0x44, sample.sustainEnd);
        size_t pattern = bytes.size();
        bytes.resize(bytes.size() + 8, 0);
        for (const std::vector<TrackerCell>& row : rows) {
            for (const TrackerCell& cell : row) {
                uint8_t mask = static_cast<uint8_t>((cell.note >= 0 ? 1 : 0) | (cell.instrument > 0 ? 2 : 0) | (cell.volume >= 0 ? 4 : 0) | (cell.command > 0 ? 8 : 0));
                bytes.push_back(static_cast<uint8_t>((cell.channel + 1) | 0x80));
                bytes.push_back(mask);
                if (cell.note >= 0) {
                    bytes.push_back(static_cast<uint8_t>(cell.note));
                }
                if (cell.instrument > 0) {
                    bytes.push_back(static_cast<uint8_t>(cell.instrument));
                }
                if (cell.volume >= 0) {
                    bytes.push_back(static_cast<uint8_t>(cell.volume));
                }
                if (cell.command > 0) {
                    bytes.push_back(static_cast<uint8_t>(cell.command));
                    bytes.push_back(static_cast<uint8_t>(cell.param));
                }
            }
            bytes.push_back(0);
        }
        for (size_t i = rows.size(); i < 64; i++) {
            bytes.push_back(0);
        }
        put16(pattern, static_cast<uint32_t>(bytes.size() - pattern - 8));
        put16(pattern + 2, 64);
        size_t data = bytes.size();
        for (int16_t value : sample.data) {
            bytes.push_back(static_cast<uint8_t>(value));
            bytes.push_back(static_cast<uint8_t>(static_cast<uint16_t>(value) >> 8));
        }
        put32(sampleTable, static_cast<uint32_t>(sampleHeader));
        put32(sampleTable + 4, static_cast<uint32_t>(pattern));
        put32(sampleHeader + 0x48, static_cast<uint32_t>(data));
        return bytes;
    }

    std::shared_ptr<const dx::audio::ItModule> trackerSong(const std::vector<std::vector<TrackerCell>>& rows, const TrackerSample& sample)
    {
        std::vector<uint8_t> bytes = trackerModule(rows, sample);
        auto module = std::make_shared<dx::audio::ItModule>();
        REQUIRE(module->load(bytes.data(), bytes.size()));
        return module;
    }

    TrackerSample constantSample(int16_t value)
    {
        TrackerSample sample;
        sample.data.assign(64, value);
        sample.loopEnd = 64;
        return sample;
    }

    std::vector<float> renderSong(std::shared_ptr<const dx::audio::ItModule> module, size_t frames)
    {
        dx::audio::ModulePlayer player(module, 44100);
        std::vector<float> out(frames * 2);
        player.render(out.data(), frames);
        return out;
    }
}

TEST_CASE("PCM wav sound decodes with header format and length", "[data]")
{
    std::vector<uint8_t> bytes = resourceBytes(DEUSEX_GAME_DATA "/System/DeusExSounds.u", "Sound", "AlarmUnitHum");
    auto pcm = dx::audio::decodeBytes(bytes);
    REQUIRE(pcm);
    CHECK(pcm->sampleRate == 22050);
    CHECK(pcm->channels == 1);
    CHECK(pcm->frames() == 76944);
    CHECK(pcm->samples[0] == Catch::Approx(-453.0 / 32768.0).margin(1e-7));
    CHECK(pcm->duration() == Catch::Approx(76944.0 / 22050.0).margin(1e-6));
}

TEST_CASE("mp3 speech decodes every mpeg frame", "[data]")
{
    std::vector<uint8_t> bytes = resourceBytes(DEUSEX_GAME_DATA "/System/DeusExConAudioMission01.u", "Sound", "ConAudioMission01_0");
    dx::USound sound;
    sound.bytes = bytes;
    auto pcm = dx::audio::decode(&sound);
    REQUIRE(pcm);
    CHECK(pcm->sampleRate == 44100);
    CHECK(pcm->channels == 1);
    CHECK(pcm->frames() == 71 * 1152);
    CHECK(dx::audio::duration(&sound) == Catch::Approx(81792.0 / 44100.0).margin(1e-6));
    CHECK(sound.duration == Catch::Approx(81792.0 / 44100.0).margin(1e-6));
}

TEST_CASE("garbage bytes do not decode")
{
    std::vector<uint8_t> bytes(64, 0x42);
    CHECK_FALSE(dx::audio::decodeBytes(bytes));
}

TEST_CASE("Galaxy pans by the view space angle over seven eighths of the range and attenuates linearly")
{
    dx::Vector listener;
    dx::Rotator facingX;
    dx::audio::Placement right = dx::audio::place(listener, facingX, { 0, 100, 0 }, 400.0f);
    CHECK(right.pan == 30718);
    CHECK(right.attenuation == Catch::Approx(0.75f));

    dx::audio::Placement left = dx::audio::place(listener, facingX, { 0, -100, 0 }, 400.0f);
    CHECK(left.pan == 2047);

    dx::audio::Placement front = dx::audio::place(listener, facingX, { 100, 0, 0 }, 200.0f);
    CHECK(front.pan == 16383);
    CHECK(front.attenuation == Catch::Approx(0.5f));

    dx::audio::Placement behind = dx::audio::place(listener, facingX, { -100, 0, 0 }, 400.0f);
    CHECK(behind.pan == 16383);

    dx::audio::Placement close = dx::audio::place(listener, facingX, { 0, 10, 0 }, 400.0f);
    CHECK(close.pan == 19966);

    dx::audio::Placement outside = dx::audio::place(listener, facingX, { 500, 0, 0 }, 200.0f);
    CHECK(outside.attenuation == 0.0f);

    dx::Rotator facingY;
    facingY.yaw = 16384;
    CHECK(dx::audio::place(listener, facingY, { 0, 100, 0 }, 400.0f).pan == 16383);
    CHECK(dx::audio::place(listener, facingY, { -100, 0, 0 }, 400.0f).pan == 30718);
}

TEST_CASE("Galaxy master volume takes the louder of sound and speech volume on a 127 scale")
{
    CHECK(dx::audio::masterVolume(255, 255) == 127);
    CHECK(dx::audio::masterVolume(204, 255) == 126);
    CHECK(dx::audio::masterVolume(255, 204) == 127);
    CHECK(dx::audio::masterVolume(0, 0) == 0);
}

TEST_CASE("Galaxy voice volume scales by category, obstruction and attenuation with a floor of 128")
{
    CHECK(dx::audio::voiceVolume(0.75f, 0.0f, 204, 255, false, 1.0f) == 19660);
    CHECK(dx::audio::voiceVolume(0.75f, 0.0f, 204, 255, true, 1.0f) == 24575);
    CHECK(dx::audio::voiceVolume(1.0f, 0.5f, 255, 255, false, 1.0f) == 10813);
    CHECK(dx::audio::voiceVolume(1.0f, 0.25f, 255, 255, false, 1.0f) == 16383);
    CHECK(dx::audio::voiceVolume(0.0f, 0.0f, 255, 255, false, 1.0f) == 128);
    CHECK(dx::audio::voiceVolume(0.75f, 0.0f, 204, 255, true, 2.0f) == 32767);
}

TEST_CASE("Galaxy channel gains use the square root pan table and the fixed point volume chain")
{
    dx::audio::Gains center = dx::audio::channelGains(32767, 16383, 127, 128);
    CHECK(center.left == Catch::Approx(22450.0 * 32767.0 / 1073741824.0).epsilon(1e-6));
    CHECK(center.right == Catch::Approx(22449.0 * 32767.0 / 1073741824.0).epsilon(1e-6));

    dx::audio::Gains send = dx::audio::channelGains(32767, 16383, 127, 127);
    CHECK(send.left == Catch::Approx(22274.0 * 32767.0 / 1073741824.0).epsilon(1e-6));

    dx::audio::Gains panned = dx::audio::channelGains(19660, 30718, 126, 128);
    CHECK(panned.left == Catch::Approx(4688.0 * 32767.0 / 1073741824.0).epsilon(1e-6));
    CHECK(panned.right == Catch::Approx(18154.0 * 32767.0 / 1073741824.0).epsilon(1e-6));
}

TEST_CASE("Galaxy Doppler shifts ambient pitch by the approach speed over DopplerSpeed")
{
    dx::Vector listener;
    dx::Vector source { 1000, 0, 0 };
    CHECK(dx::audio::dopplerShift({ 650, 0, 0 }, source, listener, 6500.0f) == Catch::Approx(0.9f));
    CHECK(dx::audio::dopplerShift({ -650, 0, 0 }, source, listener, 6500.0f) == Catch::Approx(1.1f));
    CHECK(dx::audio::dopplerShift({ -13000, 0, 0 }, source, listener, 6500.0f) == 2.0f);
    CHECK(dx::audio::dopplerShift({ 13000, 0, 0 }, source, listener, 6500.0f) == 0.5f);
    CHECK(dx::audio::dopplerShift({ 0, 650, 0 }, source, listener, 6500.0f) == 1.0f);

    CHECK(dx::audio::voiceStep(44100, 1.0f, 1.0f, 44100) == 65536);
    CHECK(dx::audio::voiceStep(11025, 1.0f, 1.0f, 44100) == 16384);
    CHECK(dx::audio::voiceStep(22050, 1.0f, 0.9f, 44100) == 29489);
    CHECK(dx::audio::voiceStep(22050, 1.0f, 1.1f, 44100) == 36044);
}

TEST_CASE("a centered full volume voice starts at the next update at the Galaxy constant power level", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    REQUIRE(dx::audio::initializeOffline(44100));
    dx::audio::setVolumes(1.0f, 1.0f, 1.0f);
    dx::USound tone;
    tone.bytes = constantWave(16384, 44100, 8000);
    int32_t id = dx::audio::playSound(&tone, nullptr, dx::audio::SlotNone, {}, 1.0f, 1000.0f, 1.0f, false);
    REQUIRE(id != 0);
    std::vector<float> buffer(4000 * 2);
    dx::audio::render(buffer.data(), 4000);
    CHECK(dx::audio::isPlaying(id));
    CHECK(*std::max_element(buffer.begin(), buffer.end()) == 0.0f);
    dx::audio::update({}, {}, 0.0f);
    dx::audio::render(buffer.data(), 4000);
    CHECK(buffer[0] == Catch::Approx(0.5 * 22450.0 * 32767.0 / 1073741824.0).epsilon(1e-6));
    CHECK(buffer[3999 * 2] == Catch::Approx(0.5 * 22450.0 * 32767.0 / 1073741824.0).epsilon(1e-5));
    CHECK(buffer[3999 * 2 + 1] == Catch::Approx(0.5 * 22449.0 * 32767.0 / 1073741824.0).epsilon(1e-5));
    dx::audio::shutdown();
}

TEST_CASE("voices advance in 16.16 fixed point with 15 bit interpolation weights and no pitch clamp", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    REQUIRE(dx::audio::initializeOffline(44100));
    dx::audio::setVolumes(1.0f, 1.0f, 1.0f);
    dx::USound ramp;
    ramp.bytes = sampleWave({ 0, 20000, 0, 0 }, 22050);
    REQUIRE(dx::audio::playSound(&ramp, nullptr, dx::audio::SlotNone, {}, 1.0f, 1000.0f, 1.0f, false) != 0);
    dx::audio::update({}, {}, 0.0f);
    std::vector<float> buffer(8 * 2);
    dx::audio::render(buffer.data(), 8);
    const double unit = 22450.0 / 35184372088832.0;
    CHECK(buffer[0] == 0.0f);
    CHECK(buffer[2] == Catch::Approx(20000.0 * 16384.0 * unit).epsilon(1e-7));
    CHECK(buffer[4] == Catch::Approx(20000.0 * 32767.0 * unit).epsilon(1e-7));
    CHECK(buffer[6] == Catch::Approx(20000.0 * 16383.0 * unit).epsilon(1e-7));
    dx::audio::shutdown();

    REQUIRE(dx::audio::initializeOffline(44100));
    dx::USound tone;
    tone.bytes = constantWave(1000, 22050, 4000);
    int32_t fast = dx::audio::playSound(&tone, nullptr, dx::audio::SlotNone, {}, 1.0f, 1000.0f, 8.0f, false);
    REQUIRE(fast != 0);
    CHECK(dx::audio::voices().front().step == 4.0);
    dx::audio::shutdown();
}

TEST_CASE("the interpolation at a loop end reads the raw next sample before wrapping", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    REQUIRE(dx::audio::initializeOffline(44100));
    dx::audio::setVolumes(1.0f, 1.0f, 1.0f);
    dx::USound looped;
    looped.bytes = loopWave({ 0, 0, 20000, 0 }, 22050, 0, 1);
    REQUIRE(dx::audio::playSound(&looped, nullptr, dx::audio::SlotNone, {}, 1.0f, 1000.0f, 1.0f, false) != 0);
    dx::audio::update({}, {}, 0.0f);
    std::vector<float> buffer(8 * 2);
    dx::audio::render(buffer.data(), 8);
    const double unit = 22450.0 / 35184372088832.0;
    CHECK(buffer[2 * 2] == 0.0f);
    CHECK(buffer[3 * 2] == Catch::Approx(20000.0 * 16384.0 * unit).epsilon(1e-7));
    CHECK(buffer[4 * 2] == 0.0f);
    CHECK(buffer[7 * 2] == Catch::Approx(20000.0 * 16384.0 * unit).epsilon(1e-7));
    dx::audio::shutdown();
}

TEST_CASE("ambient light sources scale their ambient sound by the flicker and strobe light functions", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
    REQUIRE(dx::audio::initializeOffline(44100));
    dx::audio::setVolumes(1.0f, 1.0f, 1.0f);
    {
        dx::World world;
        REQUIRE(world.loadMap("01_NYC_UNATCOIsland"));
        dx::UObject* player = world.player();
        REQUIRE(player != nullptr);
        dx::audio::update(dx::actor::Location(player), {}, 0.0f);
        const dx::UObject* chosen = nullptr;
        for (const dx::audio::VoiceStatus& voice : dx::audio::voices()) {
            if (voice.ambient && voice.volume > 2000) {
                chosen = voice.actor;
            }
        }
        REQUIRE(chosen != nullptr);
        dx::UObject* source = const_cast<dx::UObject*>(chosen);
        dx::actor::SoundVolume(source) = 128;
        dx::actor::LightBrightness(source) = 255;
        auto level = [&]() {
            dx::audio::update(dx::actor::Location(player), {}, 0.0f);
            for (const dx::audio::VoiceStatus& voice : dx::audio::voices()) {
                if (voice.actor == source) {
                    return voice.volume;
                }
            }
            return 0;
        };
        const uint8_t steady = 1;
        const uint8_t flicker = 4;
        const uint8_t strobe = 5;
        dx::actor::LightType(source) = steady;
        int32_t full = level();
        REQUIRE(full > 1000);
        dx::audio::setLightRandom([](int32_t) {
            return 0.75f;
        });
        dx::actor::LightType(source) = flicker;
        CHECK(level() == Catch::Approx(full * 0.75).margin(1.0));
        dx::audio::setLightRandom([](int32_t) {
            return 0.4f;
        });
        CHECK(level() == 128);
        dx::actor::LightType(source) = strobe;
        std::vector<int32_t> levels;
        for (int i = 0; i < 4; i++) {
            dx::levelinfo::TimeSeconds(world.levelInfo) += 0.1f;
            levels.push_back(level());
        }
        CHECK(std::min(levels[0], levels[1]) == 128);
        CHECK(std::max(levels[0], levels[1]) == full);
        CHECK(levels[2] == levels[0]);
        CHECK(levels[3] == levels[1]);
        dx::audio::setLightRandom(nullptr);
        dx::actor::LightType(source) = 0;
        dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
    }
    dx::audio::shutdown();
}

TEST_CASE("a speaking pawn takes its phoneme from the window a tenth of a second past the elapsed time", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
    REQUIRE(dx::audio::initializeOffline(44100));
    {
        dx::World world;
        REQUIRE(world.loadMap("01_NYC_UNATCOIsland"));
        dx::UObject* player = world.player();
        REQUIRE(player != nullptr);
        dx::UObject* speaker = nullptr;
        for (dx::UObject* actor : world.actors) {
            if (actor && actor->data && actor != player && dx::isPawn(actor)) {
                speaker = actor;
                break;
            }
        }
        REQUIRE(speaker != nullptr);
        std::vector<int16_t> samples(20000, 0);
        for (size_t n = 13230; n < 13230 + 1024; n++) {
            samples[n] = static_cast<int16_t>(16000.0 * std::cos(2.0 * 3.14159265358979 * 20.0 * static_cast<double>(n - 13230) / 1024.0));
        }
        dx::USound speech;
        speech.bytes = sampleWave(samples, 22050);
        dx::pawn::bIsSpeaking(speaker) = 1;
        REQUIRE(dx::audio::playSound(&speech, speaker, dx::audio::SlotTalk, dx::actor::Location(speaker), 1.0f, 100000.0f, 1.0f, true) != 0);
        dx::audio::update(dx::actor::Location(player), {}, 0.5f);
        CHECK(dx::pawn::nextPhoneme(speaker) == "O");
        dx::pawn::bIsSpeaking(speaker) = 0;
        dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
    }
    dx::audio::shutdown();
}

TEST_CASE("the reverb network is three allpass stages with a crossed half gain feedback")
{
    dx::audio::ReverbSettings settings;
    settings.volume = 1.0f;
    settings.cutoff = 0.0f;
    settings.delay = { 441.5f / 44100.0f, 13230.5f / 44100.0f, 882.5f / 44100.0f, 13230.5f / 44100.0f, 1764.5f / 44100.0f, 13230.5f / 44100.0f };
    settings.gain = { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };
    dx::audio::Reverb reverb;
    reverb.configure(settings, 44100);
    REQUIRE(reverb.active());
    std::vector<float> send(4000 * 2, 0.0f);
    send[0] = 1.0f;
    std::vector<float> out(4000 * 2, 0.0f);
    reverb.process(send.data(), out.data(), 4000);
    auto left = [&](size_t n) {
        return out[n * 2];
    };
    CHECK(left(0) == Catch::Approx(-0.0625).margin(1e-7));
    CHECK(left(441) == Catch::Approx(0.09375).margin(1e-7));
    CHECK(left(882) == Catch::Approx(0.09375).margin(1e-7));
    CHECK(left(1323) == Catch::Approx(-0.140625).margin(1e-7));
    CHECK(left(1764) == Catch::Approx(0.09375).margin(1e-7));
    CHECK(left(2205) == Catch::Approx(-0.140625).margin(1e-7));
    CHECK(left(2646) == Catch::Approx(-0.140625).margin(1e-7));
    CHECK(left(3087) == Catch::Approx(0.2109375).margin(1e-7));
    CHECK(left(1) == 0.0f);
    CHECK(left(100) == 0.0f);
    CHECK(left(2) == Catch::Approx(-0.000244140625).margin(1e-9));
    CHECK(out[1] == 0.0f);
    CHECK(out[3] == Catch::Approx(0.00390625).margin(1e-9));

    settings.volume = 0.0f;
    reverb.configure(settings, 44100);
    CHECK_FALSE(reverb.active());
}

TEST_CASE("the reverb cutoff lowpasses the recirculated delay line")
{
    dx::audio::ReverbSettings settings;
    settings.volume = 1.0f;
    settings.cutoff = static_cast<float>(std::acos(0.5) * 44100.0 / 6.282);
    settings.delay = { 441.5f / 44100.0f, 13230.5f / 44100.0f, 1500.5f / 44100.0f, 13230.5f / 44100.0f, 3000.5f / 44100.0f, 13230.5f / 44100.0f };
    settings.gain = { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };
    dx::audio::Reverb reverb;
    reverb.configure(settings, 44100);
    std::vector<float> send(1000 * 2, 0.0f);
    send[0] = 1.0f;
    std::vector<float> out(1000 * 2, 0.0f);
    reverb.process(send.data(), out.data(), 1000);
    const double k = std::sqrt(1.25) - 0.5;
    CHECK(out[441 * 2] == Catch::Approx(0.09375).margin(1e-7));
    CHECK(out[882 * 2] == Catch::Approx(0.046875 * k).margin(1e-5));
    CHECK(out[883 * 2] == Catch::Approx(0.046875 * k * (1.0 - k)).margin(1e-5));
}

TEST_CASE("the view actor zone selects the Galaxy reverb and a tail follows the dry sound", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::unloadPackage(dx::Name("06_HongKong_WanChai_Garage"));
    REQUIRE(dx::audio::initializeOffline(44100));
    dx::audio::setVolumes(1.0f, 1.0f, 1.0f);
    {
        dx::World world;
        REQUIRE(world.loadMap("06_HongKong_WanChai_Garage"));
        dx::UObject* player = world.player();
        REQUIRE(player != nullptr);
        dx::UObject* reverbZone = nullptr;
        for (dx::UObject* actor : world.actors) {
            if (actor && actor->data && actor->name == dx::Name("ZoneInfo0")) {
                reverbZone = actor;
            }
        }
        REQUIRE(reverbZone != nullptr);
        dx::audio::setVolumes(1.0f, 0.0f, 1.0f);
        for (dx::UObject* actor : world.actors) {
            if (actor && actor->data) {
                dx::actor::AmbientSound(actor) = nullptr;
            }
        }
        dx::USound tone;
        tone.bytes = constantWave(8000, 44100, 2000);
        auto tailPeak = [&]() {
            int32_t id = dx::audio::playSound(&tone, nullptr, dx::audio::SlotNone, {}, 1.0f, 1000.0f, 1.0f, false);
            REQUIRE(id != 0);
            dx::audio::update(dx::actor::Location(player), {}, 0.0f);
            std::vector<float> buffer(4410 * 2);
            dx::audio::render(buffer.data(), 2100);
            CHECK_FALSE(dx::audio::isPlaying(id));
            dx::audio::render(buffer.data(), 4410);
            float peak = 0;
            for (float sample : buffer) {
                peak = std::max(peak, std::fabs(sample));
            }
            return peak;
        };

        REQUIRE(dx::region(player).zone == reverbZone);
        dx::audio::update(dx::actor::Location(player), {}, 0.0f);
        dx::audio::ReverbSettings settings = dx::audio::reverbSettings();
        CHECK(settings.volume == Catch::Approx(100.0 / 255.0).epsilon(1e-6));
        CHECK(settings.cutoff == 1000.0f);
        CHECK(settings.delay[0] == Catch::Approx(0.08).epsilon(1e-6));
        CHECK(settings.delay[1] == Catch::Approx(0.14).epsilon(1e-6));
        CHECK(settings.delay[2] == 0.001f);
        CHECK(settings.delay[5] == 0.001f);
        CHECK(settings.gain[0] == Catch::Approx(44.0 / 255.0).epsilon(1e-6));
        CHECK(settings.gain[1] == Catch::Approx(140.0 / 255.0).epsilon(1e-6));
        CHECK(settings.gain[2] == 0.001f);
        CHECK(tailPeak() > 0.01f);

        dx::zoneinfo::bReverbZone(reverbZone) = 0;
        dx::audio::update(dx::actor::Location(player), {}, 0.0f);
        CHECK(dx::audio::reverbSettings().volume == 0.0f);
        CHECK(tailPeak() == 0.0f);
        dx::objects::unloadPackage(dx::Name("06_HongKong_WanChai_Garage"));
    }
    dx::audio::shutdown();
}

TEST_CASE("an obstructed sound fades to a third over half a second and recovers when clear", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
    REQUIRE(dx::audio::initializeOffline(44100));
    dx::audio::setVolumes(1.0f, 1.0f, 1.0f);
    {
        dx::World world;
        REQUIRE(world.loadMap("01_NYC_UNATCOIsland"));
        dx::UObject* player = world.player();
        REQUIRE(player != nullptr);
        dx::UObject* source = nullptr;
        for (dx::UObject* actor : world.actors) {
            if (actor && actor->data && actor != player && dx::isPawn(actor)) {
                source = actor;
                break;
            }
        }
        REQUIRE(source != nullptr);
        dx::Vector eye = dx::actor::Location(player);
        eye.z += dx::pawn::EyeHeight(player);
        dx::actor::Location(source) = { eye.x, eye.y, eye.z - 4000.0f };
        REQUIRE_FALSE(world.fastLineCheck(eye, dx::actor::Location(source)));
        dx::USound sound;
        loadSound(sound, "TechGogglesLoop");
        const int32_t slotMisc = 1;
        int32_t id = dx::audio::playSound(&sound, source, slotMisc, dx::actor::Location(source), 1.0f, 100000.0f, 1.0f, true);
        REQUIRE(id != 0);
        auto status = [id]() {
            for (const dx::audio::VoiceStatus& voice : dx::audio::voices()) {
                if (voice.id == id) {
                    return voice;
                }
            }
            return dx::audio::VoiceStatus {};
        };
        dx::audio::update(eye, {}, 0.2f);
        CHECK(status().obstruction == Catch::Approx(0.2f));
        for (int i = 0; i < 3; i++) {
            dx::audio::update(eye, {}, 0.2f);
        }
        CHECK(status().obstruction == 0.5f);
        CHECK(status().volume == 10380);

        dx::actor::Location(source) = { eye.x + 10.0f, eye.y, eye.z };
        REQUIRE(world.fastLineCheck(eye, dx::actor::Location(source)));
        dx::audio::update(eye, {}, 0.2f);
        CHECK(status().obstruction == Catch::Approx(0.3f));
        dx::audio::update(eye, {}, 0.5f);
        CHECK(status().obstruction == 0.0f);
        dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
    }
    dx::audio::shutdown();
}

TEST_CASE("ambient sounds stop while the game is paused and moving ambient sources are Doppler shifted", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
    REQUIRE(dx::audio::initializeOffline(44100));
    {
        dx::World world;
        REQUIRE(world.loadMap("01_NYC_UNATCOIsland"));
        dx::UObject* player = world.player();
        REQUIRE(player != nullptr);
        auto ambient = []() {
            std::vector<dx::audio::VoiceStatus> result;
            for (const dx::audio::VoiceStatus& voice : dx::audio::voices()) {
                if (voice.ambient) {
                    result.push_back(voice);
                }
            }
            return result;
        };
        dx::audio::update(dx::actor::Location(player), {}, 0.0f);
        std::vector<dx::audio::VoiceStatus> playing = ambient();
        REQUIRE_FALSE(playing.empty());

        dx::levelinfo::Pauser(world.levelInfo) = "Player";
        dx::audio::update(dx::actor::Location(player), {}, 0.0f);
        CHECK(ambient().empty());
        dx::levelinfo::Pauser(world.levelInfo).clear();
        dx::audio::update(dx::actor::Location(player), {}, 0.0f);
        playing = ambient();
        REQUIRE_FALSE(playing.empty());

        dx::UObject* mover = const_cast<dx::UObject*>(playing.front().actor);
        double still = playing.front().step;
        dx::Vector away = dx::actor::Location(mover) - dx::actor::Location(player);
        dx::actor::Velocity(mover) = away.normal() * 650.0f;
        dx::audio::update(dx::actor::Location(player), {}, 0.0f);
        double receding = 0;
        for (const dx::audio::VoiceStatus& voice : ambient()) {
            if (voice.actor == mover) {
                receding = voice.step;
            }
        }
        CHECK(receding / still == Catch::Approx(0.9).epsilon(1e-3));
        dx::actor::Velocity(mover) = {};
        dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
    }
    dx::audio::shutdown();
}

TEST_CASE("impulse tracker header of training music", "[data]")
{
    auto module = trainingMusic();
    CHECK(module->title == "Training Room ");
    REQUIRE(module->orders.size() == 16);
    CHECK(module->orders[0] == 0);
    CHECK(module->orders[1] == 1);
    CHECK(module->orders[2] == 10);
    CHECK(module->orders[15] == 255);
    CHECK(module->samples.size() == 14);
    CHECK(module->patterns.size() == 13);
    CHECK(module->instruments.empty());
    CHECK_FALSE(module->useInstruments());
    CHECK(module->speed == 6);
    CHECK(module->tempo == 116);
    CHECK(module->globalVolume == 128);
    CHECK(module->mixVolume == 48);
    CHECK(module->patterns[0].rows == 64);
    const dx::audio::ItCell& first = module->patterns[0].cells[0];
    CHECK(first.note == 60);
    CHECK(first.instrument == 2);
    const dx::audio::ItCell& cut = module->patterns[0].cells[4];
    CHECK(cut.note == dx::audio::NoteNone);
    CHECK(cut.volume == 0);
    const dx::audio::ItCell& reused = module->patterns[0].cells[5 * 64 + 3];
    CHECK(reused.volume == 0);
    CHECK(reused.note == dx::audio::NoteNone);
}

TEST_CASE("module player advances rows at the tempo rate and renders deterministic audio", "[data]")
{
    auto module = trainingMusic();
    dx::audio::ModulePlayer a(module, 44100);
    dx::audio::ModulePlayer b(module, 44100);
    std::vector<float> first(44100 * 2);
    std::vector<float> second(44100 * 2);
    a.render(first.data(), 44100);
    b.render(second.data(), 44100);
    CHECK(first == second);
    CHECK(a.order() == 0);
    CHECK(a.row() == 7);

    double energy = 0;
    float peak = 0;
    for (float value : first) {
        energy += static_cast<double>(value) * static_cast<double>(value);
        peak = std::max(peak, std::fabs(value));
    }
    CHECK(std::sqrt(energy / static_cast<double>(first.size())) > 0.001);
    CHECK(std::isfinite(peak));
}

TEST_CASE("module player starts at the requested song section", "[data]")
{
    auto module = trainingMusic();
    dx::audio::ModulePlayer player(module, 44100);
    player.setPosition(2);
    CHECK(player.order() == 2);
    CHECK(player.row() == 0);
    player.setPosition(15);
    CHECK(player.order() == 0);
}

TEST_CASE("wav loop points come from the smpl chunk", "[data]")
{
    auto looped = dx::audio::decodeBytes(resourceBytes(DEUSEX_GAME_DATA "/System/DeusExSounds.u", "Sound", "TechGogglesLoop"));
    REQUIRE(looped);
    CHECK(looped->frames() == 55295);
    CHECK(looped->looped);
    CHECK(looped->loopStart == 13);
    CHECK(looped->loopEnd == 55252);

    auto oneShot = dx::audio::decodeBytes(resourceBytes(DEUSEX_GAME_DATA "/System/DeusExSounds.u", "Sound", "GlassBreakSmall"));
    REQUIRE(oneShot);
    CHECK_FALSE(oneShot->looped);
}

TEST_CASE("ambient sound radius follows the actor sound radius")
{
    CHECK(dx::audio::ambientRadius(0) == 25.0f);
    CHECK(dx::audio::ambientRadius(32) == 825.0f);
    CHECK(dx::audio::ambientRadius(255) == 6400.0f);
}

TEST_CASE("a looped sound keeps playing after a one shot sound has ended", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    REQUIRE(dx::audio::initializeOffline(44100));
    dx::USound looped;
    loadSound(looped, "TechGogglesLoop");
    dx::USound oneShot;
    loadSound(oneShot, "GlassBreakSmall");
    float loopedLength = dx::audio::duration(&looped);
    REQUIRE(dx::audio::duration(&oneShot) < loopedLength);
    int32_t loopedId = dx::audio::playSound(&looped, nullptr, dx::audio::SlotNone, {}, 1.0f, 1000.0f, 1.0f, false);
    int32_t oneShotId = dx::audio::playSound(&oneShot, nullptr, dx::audio::SlotNone, {}, 1.0f, 1000.0f, 1.0f, false);
    REQUIRE(loopedId != 0);
    REQUIRE(oneShotId != 0);
    dx::audio::update({}, {}, 0.0f);
    renderFrames(static_cast<size_t>(loopedLength * 2.0f * 44100.0f));
    CHECK(dx::audio::isPlaying(loopedId));
    CHECK_FALSE(dx::audio::isPlaying(oneShotId));
    dx::audio::shutdown();
}

TEST_CASE("a sound on an occupied actor slot replaces the earlier one unless it is no override", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    REQUIRE(dx::audio::initializeOffline(44100));
    dx::USound sound;
    loadSound(sound, "TechGogglesLoop");
    dx::UObject actor;
    const int32_t slotMisc = 1;
    int32_t first = dx::audio::playSound(&sound, &actor, slotMisc, {}, 1.0f, 1000.0f, 1.0f, true);
    int32_t second = dx::audio::playSound(&sound, &actor, slotMisc, {}, 1.0f, 1000.0f, 1.0f, true);
    REQUIRE(first != 0);
    REQUIRE(second != 0);
    CHECK_FALSE(dx::audio::isPlaying(first));
    CHECK(dx::audio::isPlaying(second));

    int32_t blocked = dx::audio::playSound(&sound, &actor, slotMisc, {}, 1.0f, 1000.0f, 1.0f, true, true);
    CHECK(blocked == 0);
    CHECK(dx::audio::isPlaying(second));

    int32_t a = dx::audio::playSound(&sound, &actor, dx::audio::SlotNone, {}, 1.0f, 1000.0f, 1.0f, true);
    int32_t b = dx::audio::playSound(&sound, &actor, dx::audio::SlotNone, {}, 1.0f, 1000.0f, 1.0f, true);
    CHECK(dx::audio::isPlaying(a));
    CHECK(dx::audio::isPlaying(b));
    CHECK(dx::audio::isPlaying(second));
    dx::audio::shutdown();
}

TEST_CASE("the voice limit steals the quietest sound and rejects quieter ones", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    REQUIRE(dx::audio::initializeOffline(44100));
    dx::USound sound;
    loadSound(sound, "TechGogglesLoop");
    std::vector<int32_t> distant;
    for (int i = 0; i < 16; i++) {
        int32_t id = dx::audio::playSound(&sound, nullptr, dx::audio::SlotNone, { 500, 0, 0 }, 1.0f, 1000.0f, 1.0f, true);
        REQUIRE(id != 0);
        distant.push_back(id);
    }
    CHECK(dx::audio::voices().size() == 16);
    CHECK(dx::audio::voices().front().priority == Catch::Approx(0.5f));

    int32_t near = dx::audio::playSound(&sound, nullptr, dx::audio::SlotNone, { 100, 0, 0 }, 1.0f, 1000.0f, 1.0f, true);
    CHECK(near != 0);
    CHECK(dx::audio::isPlaying(near));
    size_t stillPlaying = static_cast<size_t>(std::count_if(distant.begin(), distant.end(), dx::audio::isPlaying));
    CHECK(stillPlaying == 15);
    CHECK(dx::audio::voices().size() == 16);

    int32_t far = dx::audio::playSound(&sound, nullptr, dx::audio::SlotNone, { 900, 0, 0 }, 1.0f, 1000.0f, 1.0f, true);
    CHECK(far == 0);
    CHECK(dx::audio::voices().size() == 16);
    dx::audio::shutdown();
}

TEST_CASE("a fast fade drops three per second and switches two latencies after silence", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    REQUIRE(dx::audio::initializeOffline(44100));
    dx::UMusic music;
    music.bytes = resourceBytes(DEUSEX_GAME_DATA "/Music/LibertyIsland_Music.umx", "Music", "LibertyIsland_Music");
    const uint8_t instant = 1;
    const uint8_t fade = 3;
    const uint8_t fastFade = 4;
    dx::audio::playMusic(&music, 0, instant);
    dx::audio::MusicStatus status = dx::audio::musicStatus();
    CHECK(status.music == &music);
    CHECK(status.section == 0);
    CHECK(status.order == 0);
    CHECK_FALSE(status.pending);

    renderFrames(4410);
    dx::audio::playMusic(&music, 3, fastFade);
    status = dx::audio::musicStatus();
    CHECK(status.section == 0);
    CHECK(status.pending);

    renderFrames(7000);
    status = dx::audio::musicStatus();
    CHECK(status.pending);
    CHECK(status.order == 0);
    CHECK(status.fade == Catch::Approx(0.5238f).margin(0.002f));

    renderFrames(8000);
    status = dx::audio::musicStatus();
    CHECK(status.pending);
    CHECK(status.order == 0);
    CHECK(status.fade == Catch::Approx(-0.0204f).margin(0.002f));

    renderFrames(4000);
    status = dx::audio::musicStatus();
    CHECK_FALSE(status.pending);
    CHECK(status.section == 3);
    CHECK(status.order == 3);
    CHECK(status.row == 0);
    CHECK(status.fade == 1.0f);

    dx::audio::playMusic(&music, 255, fade);
    CHECK(dx::audio::musicStatus().order == 3);
    CHECK(dx::audio::musicStatus().pending);
    renderFrames(44100 + 4096);
    status = dx::audio::musicStatus();
    CHECK(status.music == &music);
    CHECK(status.section == 255);
    CHECK(status.order == -1);
    CHECK_FALSE(status.pending);
    dx::audio::shutdown();
}

TEST_CASE("a segue or a repeated section restarts the section at once and keeps the song tempo", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    REQUIRE(dx::audio::initializeOffline(44100));
    dx::UMusic music;
    music.bytes = resourceBytes(DEUSEX_GAME_DATA "/Music/Training_Music.umx", "Music", "Training_Music");
    const uint8_t instant = 1;
    const uint8_t segue = 2;
    dx::audio::playMusic(&music, 7, instant);
    renderFrames(44100);
    dx::audio::MusicStatus status = dx::audio::musicStatus();
    CHECK(status.order == 7);
    CHECK(status.row == 5);

    dx::audio::playMusic(&music, 7, instant);
    status = dx::audio::musicStatus();
    CHECK(status.order == 7);
    CHECK(status.row == 0);

    dx::audio::playMusic(&music, 1, segue);
    status = dx::audio::musicStatus();
    CHECK_FALSE(status.pending);
    CHECK(status.section == 1);
    CHECK(status.order == 1);
    CHECK(status.row == 0);
    renderFrames(44100);
    CHECK(dx::audio::musicStatus().row == 5);

    dx::audio::playMusic(&music, 0, instant);
    renderFrames(44100);
    CHECK(dx::audio::musicStatus().order == 0);
    CHECK(dx::audio::musicStatus().row == 7);
    dx::audio::shutdown();
}

TEST_CASE("Galaxy ignores the impulse tracker mix volume", "[data]")
{
    auto module = trainingMusic();
    auto louder = std::make_shared<dx::audio::ItModule>(*module);
    louder->mixVolume = 96;
    dx::audio::ModulePlayer a(module, 44100);
    dx::audio::ModulePlayer b(louder, 44100);
    std::vector<float> first(44100 * 2);
    std::vector<float> second(44100 * 2);
    a.render(first.data(), 44100);
    b.render(second.data(), 44100);
    CHECK(first == second);
    float peak = 0;
    for (float value : first) {
        peak = std::max(peak, std::fabs(value));
    }
    CHECK(peak > 0.01f);
}

TEST_CASE("Galaxy music voices run the fixed point gain chain through the square root pan table")
{
    std::vector<float> column = renderSong(trackerSong({ { { 0, 60, 1, 64 } } }, constantSample(16384)), 4096);
    CHECK(column[4000 * 2] == Catch::Approx(0.5 * 13808.0 / 32768.0).epsilon(1e-5));
    CHECK(column[4000 * 2 + 1] == Catch::Approx(0.5 * 13809.0 / 32768.0).epsilon(1e-5));
    std::vector<float> preset = renderSong(trackerSong({ { { 0, 60, 1 } } }, constantSample(16384)), 4096);
    CHECK(preset[4000 * 2] == Catch::Approx(0.5 * 13917.0 / 32768.0).epsilon(1e-5));
    CHECK(preset[4000 * 2 + 1] == Catch::Approx(0.5 * 13917.0 / 32768.0).epsilon(1e-5));
}

TEST_CASE("Galaxy drops channel volume, panbrello and the sample global volume")
{
    std::vector<float> plain = renderSong(trackerSong({ { { 0, 60, 1, 64 } }, {} }, constantSample(16384)), 12000);
    TrackerSample quiet = constantSample(16384);
    quiet.globalVolume = 16;
    std::vector<float> dropped = renderSong(trackerSong({ { { 0, 60, 1, 64, 13, 0x20 } }, { { 0, -1, 0, -1, 25, 0x44 } } }, quiet), 12000);
    CHECK(dropped == plain);
    CHECK(plain[4000 * 2] == Catch::Approx(0.5 * 13808.0 / 32768.0).epsilon(1e-5));
}

TEST_CASE("Galaxy ignores sustain loops and a note off cuts the sample at once")
{
    TrackerSample sustained;
    sustained.data.assign(2000, 16384);
    sustained.flags = 0x23;
    sustained.sustainBegin = 0;
    sustained.sustainEnd = 2000;
    std::vector<float> once = renderSong(trackerSong({ { { 0, 60, 1, 64 } } }, sustained), 4000);
    CHECK(once[1000 * 2] == Catch::Approx(0.5 * 13808.0 / 32768.0).epsilon(1e-5));
    CHECK(once[3000 * 2] == 0.0f);
    std::vector<float> released = renderSong(trackerSong({ { { 0, 60, 1, 64 } }, { { 0, 255 } } }, constantSample(16384)), 6000);
    CHECK(released[5000 * 2] == Catch::Approx(0.5 * 13808.0 / 32768.0).epsilon(1e-5));
    CHECK(released[5292 * 2 + 200] == 0.0f);
}

TEST_CASE("S9x centres the channel and inverts its right side for Galaxy surround")
{
    std::vector<float> out = renderSong(trackerSong({ { { 0, 60, 1, 64, 24, 0x00 } }, { { 0, -1, 0, -1, 19, 0x91 } } }, constantSample(16384)), 9000);
    CHECK(out[4000 * 2] == Catch::Approx(0.5 * 19528.0 / 32768.0).epsilon(1e-5));
    CHECK(out[4000 * 2 + 1] == 0.0f);
    CHECK(out[8800 * 2] == Catch::Approx(0.5 * 13808.0 / 32768.0).epsilon(1e-5));
    CHECK(out[8800 * 2 + 1] == Catch::Approx(-0.5 * 13809.0 / 32768.0).epsilon(1e-5));
}

TEST_CASE("Q keeps the volume and retriggers on every tick divisible by its interval")
{
    TrackerSample ramp;
    for (int i = 0; i < 2048; i++) {
        ramp.data.push_back(static_cast<int16_t>(i * 16));
    }
    ramp.flags = 0x03;
    std::vector<float> out = renderSong(trackerSong({ { { 0, 60, 1, 64, 17, 0x32 } } }, ramp), 4000);
    CHECK(out[1500 * 2] == Catch::Approx(1500.0 * 16.0 / 32768.0 * 13808.0 / 32768.0).epsilon(1e-5));
    CHECK(out[1774 * 2] == Catch::Approx(10.0 * 16.0 / 32768.0 * 13808.0 / 32768.0).epsilon(1e-5));
    CHECK(out[3538 * 2] == Catch::Approx(10.0 * 16.0 / 32768.0 * 13808.0 / 32768.0).epsilon(1e-5));
}

TEST_CASE("vibrato bends by the Galaxy sine table times depth in 256ths of a semitone")
{
    TrackerSample ramp;
    for (int i = 0; i < 4000; i++) {
        ramp.data.push_back(static_cast<int16_t>(i * 8));
    }
    ramp.flags = 0x03;
    std::vector<float> out = renderSong(trackerSong({ { { 0, 60, 1, 64, 8, 0x48 } } }, ramp), 2000);
    double position = 882.0 + 882.0 * std::pow(2.0, 97.0 / 3072.0);
    CHECK(out[1764 * 2] == Catch::Approx(position * 8.0 / 32768.0 * 13808.0 / 32768.0).epsilon(1e-4));
}

TEST_CASE("the loader resolves effect memory per channel and drops what Galaxy drops")
{
    auto module = trackerSong({ { { 0, 60, 1, 32, 15, 0x10 } }, { { 0, -1, 0, -1, 4, 0x00 } }, { { 0, -1, 0, -1, 25, 0x44 } }, { { 0, -1, 0, -1, 1, 0x40 } }, { { 0, -1, 0, -1, 17, 0x32 } } }, constantSample(16384));
    const std::vector<dx::audio::ItCell>& cells = module->patterns[0].cells;
    CHECK(cells[64].command == 4);
    CHECK(cells[64].param == 0x10);
    CHECK(cells[2 * 64].command == 0);
    CHECK(cells[3 * 64].param == 0x20);
    CHECK(cells[4 * 64].param == 0x02);
}

TEST_CASE("the music volume is squared into the Galaxy master level", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    REQUIRE(dx::audio::initializeOffline(44100));
    dx::UMusic music;
    music.bytes = trackerModule({ { { 0, 60, 1, 64 } } }, constantSample(16384));
    dx::audio::setVolumes(1.0f, 0.5f, 1.0f);
    dx::audio::playMusic(&music, 0, 1);
    std::vector<float> out(4096 * 2);
    dx::audio::render(out.data(), 4096);
    CHECK(out[4000 * 2] == Catch::Approx(0.5 * 13808.0 / 32768.0 * 3969.0 / 16129.0).epsilon(1e-5));
    dx::audio::setVolumes(1.0f, 1.0f, 1.0f);
    dx::audio::shutdown();
}

TEST_CASE("level music starts at login and an attacking enemy switches to the combat section", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
    REQUIRE(dx::audio::initializeOffline(44100));
    {
        dx::World world;
        REQUIRE(world.loadMap("01_NYC_UNATCOIsland"));
        dx::UObject* player = world.player();
        REQUIRE(player != nullptr);
        dx::audio::update(dx::actor::Location(player), {}, 0.0f);
        dx::audio::MusicStatus status = dx::audio::musicStatus();
        REQUIRE(status.music != nullptr);
        CHECK(status.music->name == dx::Name("LibertyIsland_Music"));
        CHECK(status.section == 0);

        dx::console::execute(player, "teleport -1900 -653 -95 32768");
        bool combat = false;
        for (int i = 0; i < 450 && !combat; i++) {
            world.tick(1.0f / 30.0f);
            dx::audio::update(dx::actor::Location(player), {}, 1.0f / 30.0f);
            renderFrames(1470);
            combat = dx::audio::musicStatus().section == 3;
        }
        CHECK(combat);

        dx::audio::setVolumes(1.0f, 1.0f, 1.0f);
        dx::vm::Call music(player, dx::Name("SetInstantMusicVolume"));
        REQUIRE(music.valid());
        music.byte(51).run();
        CHECK(dx::audio::volumes().music == Catch::Approx(0.2f));
        CHECK(dx::audio::volumes().sound == 1.0f);
        CHECK(dx::audio::volumes().speech == 1.0f);
        dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
    }
    dx::audio::shutdown();
}

TEST_CASE("speech phonemes come from the strongest FFT bin of the window")
{
    auto tone = [](int bin) {
        dx::audio::DecodedSound pcm;
        pcm.sampleRate = 44100;
        pcm.channels = 1;
        pcm.samples.resize(4096);
        for (size_t n = 0; n < pcm.samples.size(); n++) {
            pcm.samples[n] = bin < 0 ? 0.0f : 0.5f * static_cast<float>(std::cos(2.0 * 3.14159265358979 * bin * static_cast<double>(n) / 1024.0));
        }
        return dx::audio::speechPhoneme(pcm, 0);
    };
    CHECK(tone(-1) == 'X');
    CHECK(tone(3) == 'E');
    CHECK(tone(8) == 'U');
    CHECK(tone(12) == 'O');
    CHECK(tone(20) == 'A');
    CHECK(tone(40) == 'F');
    CHECK(tone(60) == 'T');
}
