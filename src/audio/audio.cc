#include "audio/audio.h"

#include "audio/module.h"
#include "core/config.h"
#include "core/objects.h"
#include "engine/actor.h"
#include "engine/props_engine.h"
#include "engine/resources.h"
#include "engine/world.h"

#include <miniaudio.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace dx::audio {
    namespace {
        using Ext = long double;

        constexpr uint8_t TransitionFade = 3;
        constexpr uint8_t TransitionFastFade = 4;
        constexpr uint8_t TransitionSlowFade = 5;
        constexpr uint8_t SectionSilence = 255;
        constexpr int32_t MaxPan = 32767;
        constexpr int32_t CenterPan = 16383;
        constexpr int32_t ReverbSend = 127;
        constexpr int32_t DrySend = 128;
        constexpr size_t ReverbFrames = 16384;
        constexpr uint8_t LT_None = 0;
        constexpr uint8_t LT_Pulse = 2;
        constexpr uint8_t LT_Blink = 3;
        constexpr uint8_t LT_Flicker = 4;
        constexpr uint8_t LT_Strobe = 5;
        constexpr uint8_t LT_SubtlePulse = 7;
        constexpr uint8_t LT_TexturePaletteOnce = 8;
        constexpr uint8_t LT_TexturePaletteLoop = 9;

        float (*lightRandomSource)(int32_t) = nullptr;

        struct Voice {
            int32_t id = 0;
            std::shared_ptr<const DecodedSound> pcm;
            const USound* sound = nullptr;
            size_t position = 0;
            uint32_t fraction = 0;
            uint32_t step = 65536;
            float gainLeft = 0;
            float gainRight = 0;
            int32_t dryLeft = 0;
            int32_t dryRight = 0;
            int32_t wetLeft = 0;
            int32_t wetRight = 0;
            float priority = 0;
            bool loop = false;
            bool stopping = false;
            bool done = false;
            bool ambient = false;
            bool pending = true;
            UObject* actor = nullptr;
            int32_t slot = 0;
            int32_t channel = -1;
            Vector location;
            float volume = 1;
            float radius = 1600;
            float pitch = 1;
            float doppler = 1;
            float obstruction = 0;
            int32_t level = 0;
            int32_t pan = CenterPan;
            bool attenuate = true;
            float elapsed = 0;
            float nextPhonemeTime = 0;
        };

        void fft(std::array<float, 1024>& re, std::array<float, 1024>& im)
        {
            std::array<float, 1024> input = re;
            for (int i = 0; i < 1024; i++) {
                int reversed = 0;
                int value = i;
                for (int bit = 0; bit < 10; bit++) {
                    reversed = (reversed << 1) | (value & 1);
                    value >>= 1;
                }
                re[static_cast<size_t>(reversed)] = input[static_cast<size_t>(i)];
                im[static_cast<size_t>(reversed)] = 0;
            }
            int blockEnd = 1;
            for (int blockSize = 2; blockSize <= 1024; blockSize <<= 1) {
                float delta = 6.2831854820251465f / static_cast<float>(blockSize);
                float sm2 = std::sin(-2 * delta);
                float sm1 = std::sin(-delta);
                float cm2 = std::cos(-2 * delta);
                float cm1 = std::cos(-delta);
                float w = 2 * cm1;
                for (int i = 0; i < 1024; i += blockSize) {
                    float ar2 = cm2;
                    float ar1 = cm1;
                    float ai2 = sm2;
                    float ai1 = sm1;
                    for (int j = i, n = 0; n < blockEnd; j++, n++) {
                        float ar0 = w * ar1 - ar2;
                        ar2 = ar1;
                        ar1 = ar0;
                        float ai0 = w * ai1 - ai2;
                        ai2 = ai1;
                        ai1 = ai0;
                        size_t a = static_cast<size_t>(j);
                        size_t b = static_cast<size_t>(j + blockEnd);
                        float tr = ar0 * re[b] - ai0 * im[b];
                        float ti = ar0 * im[b] + ai0 * re[b];
                        re[b] = re[a] - tr;
                        im[b] = im[a] - ti;
                        re[a] += tr;
                        im[a] += ti;
                    }
                }
                blockEnd = blockSize;
            }
        }

        struct State {
            bool active = false;
            bool offline = false;
            bool hasContext = false;
            ma_context context {};
            ma_device device {};
            uint32_t rate = 44100;
            size_t channels = 16;
            float ambientFactor = 0.7f;
            float dopplerSpeed = 6500;
            bool useReverb = true;
            bool digitalMusic = true;
            float latency = 40;
            std::mutex mutex;
            std::vector<Voice> voices;
            int32_t nextId = 1;
            Vector listener;
            Rotator listenerRotation;
            Vector viewer;
            Vector ear;
            float soundVolume = 1;
            float musicVolume = 1;
            float speechVolume = 1;
            std::unique_ptr<ModulePlayer> music;
            std::unique_ptr<ModulePlayer> pendingMusic;
            UMusic* musicObject = nullptr;
            int32_t musicSection = -1;
            bool musicPlaying = false;
            float musicFade = 1;
            float fadeSpeed = 0;
            bool requested = false;
            UMusic* requestObject = nullptr;
            int32_t requestSection = -1;
            std::vector<float> scratch;
            std::vector<float> send;
            Reverb reverb;
            ReverbSettings reverbSettings;
            std::unordered_map<const USound*, std::shared_ptr<const DecodedSound>> sounds;
            std::unordered_map<const UMusic*, std::shared_ptr<const ItModule>> modules;
        };

        State& state()
        {
            static State instance;
            return instance;
        }

        const std::array<int16_t, 32768>& panTable()
        {
            static const std::array<int16_t, 32768> table = [] {
                std::array<int16_t, 32768> values {};
                for (size_t i = 0; i < values.size(); i++) {
                    values[i] = static_cast<int16_t>(std::sqrt(static_cast<Ext>(i) * static_cast<Ext>(1.0 / 32767.0)) * 32767.0L);
                }
                return values;
            }();
            return table;
        }

        std::array<int32_t, 2> channelLevels(int32_t volume, int32_t pan, int32_t master, int32_t send)
        {
            int32_t level = (0x7e02 * (master * master * 2)) >> 15;
            for (int i = 0; i < 4; i++) {
                level = (level * 0x7fff) >> 15;
            }
            level = (level * volume) >> 15;
            const std::array<int16_t, 32768>& table = panTable();
            int32_t left = (((table[static_cast<size_t>(MaxPan - pan)] * level) >> 15) * send) >> 7;
            int32_t right = (((table[static_cast<size_t>(pan)] * level) >> 15) * send) >> 7;
            return { left, right };
        }

        uint8_t volumeByte(float volume)
        {
            return static_cast<uint8_t>(std::lround(std::clamp(volume, 0.0f, 1.0f) * 255.0f));
        }

        Ext squaredDistance(const Vector& a, const Vector& b)
        {
            Ext dx = static_cast<Ext>(a.x) - static_cast<Ext>(b.x);
            Ext dy = static_cast<Ext>(a.y) - static_cast<Ext>(b.y);
            Ext dz = static_cast<Ext>(a.z) - static_cast<Ext>(b.z);
            return dx * dx + dy * dy + dz * dz;
        }

        Ext categoryScale(uint8_t sound, uint8_t speech, bool speechSlot)
        {
            if (sound == speech) {
                return static_cast<Ext>(sound) * static_cast<Ext>(1.0f / 255.0f);
            }
            if (speechSlot && sound > speech) {
                return static_cast<Ext>(speech) / static_cast<Ext>(sound);
            }
            if (!speechSlot && sound < speech) {
                return static_cast<Ext>(sound) / static_cast<Ext>(speech);
            }
            return 1.0L;
        }

        float sinTab(int32_t angle)
        {
            return static_cast<float>(std::sin(static_cast<double>((angle >> 2) & 16383) * 6.283185307179586 / 16384.0));
        }

        float cosTab(int32_t angle)
        {
            return sinTab(angle + 16384);
        }

        Coords viewAxes(const Rotator& rotation)
        {
            float sp = sinTab(rotation.pitch);
            float cp = cosTab(rotation.pitch);
            float sy = sinTab(rotation.yaw);
            float cy = cosTab(rotation.yaw);
            float sr = sinTab(rotation.roll);
            float cr = cosTab(rotation.roll);
            Coords c;
            c.xAxis = { cp * cy, cp * sy, sp };
            c.yAxis = { sr * sp * cy - cr * sy, sr * sp * sy + cr * cy, -sr * cp };
            c.zAxis = { -(cr * sp * cy + sr * sy), cy * sr - cr * sp * sy, cr * cp };
            return c;
        }

        float ambientBrightness(UObject* actor, float brightness, float time)
        {
            uint8_t type = actor::LightType(actor);
            if (type == LT_None) {
                return brightness;
            }
            float lit = static_cast<float>(static_cast<Ext>(actor::LightBrightness(actor)) * static_cast<Ext>(brightness) * static_cast<Ext>(1.0 / 255.0));
            double cycle = static_cast<double>(time) * 35.0 * 65536.0;
            int32_t phase = actor::LightPhase(actor) << 8;
            if (type == LT_Pulse || type == LT_SubtlePulse) {
                int32_t angle = static_cast<int32_t>(cycle / std::max<int32_t>(actor::LightPeriod(actor), 1) + phase);
                double depth = type == LT_Pulse ? 0.39 : 0.09;
                double base = type == LT_Pulse ? 0.6 : 0.9;
                lit = static_cast<float>(static_cast<double>(lit) * (base + depth * static_cast<double>(sinTab(angle))));
            } else if (type == LT_Blink) {
                if (static_cast<int32_t>(cycle / (actor::LightPeriod(actor) + 1) + phase) & 1) {
                    lit = 0;
                }
            } else if (type == LT_Flicker) {
                float random = lightRandomSource ? lightRandomSource(static_cast<int32_t>(actor->handle & 0xff)) : 1.0f;
                lit = random < 0.5f ? 0.0f : lit * random;
            } else if (type == LT_Strobe) {
                static float lastUpdateTime = 0;
                static int32_t toggle = 0;
                if (lastUpdateTime != time) {
                    lastUpdateTime = time;
                    toggle ^= 1;
                }
                if (toggle) {
                    lit = 0;
                }
            } else if (type == LT_TexturePaletteOnce || type == LT_TexturePaletteLoop) {
                int32_t index = 0;
                if (type == LT_TexturePaletteOnce) {
                    float defaultLife = actor->cls && actor->cls->defaultObject && actor->cls->defaultObject->data ? actor::LifeSpan(actor->cls->defaultObject) : 0.0f;
                    double fraction = 1.0 - static_cast<double>(actor::LifeSpan(actor)) / static_cast<double>(defaultLife);
                    fraction = std::isnan(fraction) ? 0.0 : std::clamp(fraction, 0.0, 1.0);
                    index = std::clamp(static_cast<int32_t>(std::floor(static_cast<float>(fraction * 255.0))), 0, 255);
                } else {
                    double loop = static_cast<double>(time) * 35.0 / std::max<int32_t>(actor::LightPeriod(actor), 1) + actor::LightPhase(actor);
                    index = (static_cast<int32_t>(loop * 256.0) & 255) % 255;
                }
                UObject* skinObject = actor::Skin(actor);
                if (skinObject) {
                    objects::ensureLoaded(skinObject);
                }
                UTexture* skin = cast<UTexture>(skinObject);
                UPalette* palette = skin ? skin->palette() : nullptr;
                if (palette && palette->colors.size() >= 256) {
                    uint32_t entry = palette->colors[static_cast<size_t>(index)];
                    double r = static_cast<double>(entry & 0xff);
                    double g = static_cast<double>((entry >> 8) & 0xff);
                    double b = static_cast<double>((entry >> 16) & 0xff);
                    lit = static_cast<float>(static_cast<double>(lit) * ((2.0 * r + 3.0 * g + b) * (1.0 / 1536.0)) * 2.8);
                }
            }
            return std::clamp(lit, 0.0f, 1.0f);
        }

        uint32_t readU32(const uint8_t* bytes)
        {
            return static_cast<uint32_t>(bytes[0]) | (static_cast<uint32_t>(bytes[1]) << 8) | (static_cast<uint32_t>(bytes[2]) << 16) | (static_cast<uint32_t>(bytes[3]) << 24);
        }

        void readLoop(const std::vector<uint8_t>& bytes, DecodedSound& pcm)
        {
            if (bytes.size() < 12 || std::memcmp(bytes.data(), "RIFF", 4) != 0 || std::memcmp(bytes.data() + 8, "WAVE", 4) != 0) {
                return;
            }
            size_t offset = 12;
            while (offset + 8 <= bytes.size()) {
                size_t size = readU32(bytes.data() + offset + 4);
                if (std::memcmp(bytes.data() + offset, "smpl", 4) == 0) {
                    if (size < 60 || offset + 8 + size > bytes.size()) {
                        return;
                    }
                    const uint8_t* chunk = bytes.data() + offset + 8;
                    if (readU32(chunk + 28) == 0) {
                        return;
                    }
                    size_t start = readU32(chunk + 44);
                    size_t end = static_cast<size_t>(readU32(chunk + 48)) + 1;
                    end = std::min(end, pcm.frames());
                    if (end > start) {
                        pcm.looped = true;
                        pcm.loopStart = start;
                        pcm.loopEnd = end;
                    }
                    return;
                }
                offset += 8 + size + (size & 1);
            }
        }

        float priorityOf(const State& s, const Voice& voice)
        {
            if (!voice.attenuate) {
                return voice.volume;
            }
            Ext distance = static_cast<Ext>(std::sqrt(static_cast<double>(squaredDistance(voice.location, s.viewer))));
            return static_cast<float>((1.0L - distance / static_cast<Ext>(voice.radius)) * static_cast<Ext>(voice.volume));
        }

        void refreshGains(State& s, Voice& voice)
        {
            Placement placement;
            if (voice.attenuate) {
                placement = place(s.listener, s.listenerRotation, voice.location, voice.radius);
            }
            uint8_t sound = volumeByte(s.soundVolume);
            uint8_t speech = volumeByte(s.speechVolume);
            voice.level = voiceVolume(placement.attenuation, voice.obstruction, sound, speech, voice.slot == SlotTalk, voice.volume);
            voice.pan = placement.pan;
            int32_t master = masterVolume(sound, speech);
            std::array<int32_t, 2> dry = channelLevels(voice.level, voice.pan, master, DrySend);
            std::array<int32_t, 2> wet = channelLevels(voice.level, voice.pan, master, ReverbSend);
            voice.dryLeft = dry[0];
            voice.dryRight = dry[1];
            voice.wetLeft = wet[0];
            voice.wetRight = wet[1];
            Gains gains = channelGains(voice.level, voice.pan, master, DrySend);
            voice.gainLeft = gains.left;
            voice.gainRight = gains.right;
            voice.priority = priorityOf(s, voice);
            if (voice.pcm) {
                voice.step = voiceStep(voice.pcm->sampleRate, voice.pitch, voice.doppler, s.rate);
            }
        }

        bool alive(const Voice& voice)
        {
            return !voice.done && !voice.stopping;
        }

        void mixVoice(Voice& voice, float* out, float* send, size_t frames)
        {
            const DecodedSound& pcm = *voice.pcm;
            size_t total = pcm.frames();
            if (total == 0 || voice.stopping) {
                voice.done = true;
                return;
            }
            bool looping = voice.loop || pcm.looped;
            size_t loopStart = pcm.looped ? pcm.loopStart : 0;
            size_t loopEnd = pcm.looped ? pcm.loopEnd : total;
            const int16_t* data = pcm.fixed.data();
            uint32_t channels = pcm.channels;
            const float unit = 1.0f / 1073741824.0f;
            float dryLeft = static_cast<float>(voice.dryLeft) * unit;
            float dryRight = static_cast<float>(voice.dryRight) * unit;
            float wetLeft = static_cast<float>(voice.wetLeft) * unit;
            float wetRight = static_cast<float>(voice.wetRight) * unit;
            const float scale = 1.0f / 32768.0f;
            uint32_t step = voice.step & ~1u;
            for (size_t i = 0; i < frames; i++) {
                if (looping && voice.position >= loopEnd) {
                    voice.position = loopStart + (voice.position - loopStart) % (loopEnd - loopStart);
                }
                if (voice.position >= total) {
                    voice.done = true;
                    return;
                }
                size_t index = voice.position;
                size_t next = index + 1 < total ? index + 1 : index;
                int32_t later = static_cast<int32_t>(voice.fraction >> 1);
                int32_t earlier = 32767 - later;
                size_t right = channels >= 2 ? 1 : 0;
                float left = static_cast<float>(data[index * channels] * earlier + data[next * channels] * later);
                float other = static_cast<float>(data[index * channels + right] * earlier + data[next * channels + right] * later);
                out[i * 2] += left * dryLeft * scale;
                out[i * 2 + 1] += other * dryRight * scale;
                send[i * 2] += left * wetLeft * scale;
                send[i * 2 + 1] += other * wetRight * scale;
                uint32_t fraction = voice.fraction + (step & 0xffff);
                voice.position += (step >> 16) + (fraction >> 16);
                voice.fraction = fraction & 0xffff;
            }
        }

        void startRequestedMusic(State& s)
        {
            if (s.requestObject != s.musicObject) {
                s.music = std::move(s.pendingMusic);
            }
            s.pendingMusic.reset();
            s.musicObject = s.requestObject;
            s.musicSection = s.requestSection;
            s.musicFade = 1;
            s.fadeSpeed = 0;
            s.requested = false;
            s.musicPlaying = s.music && s.musicSection != SectionSilence;
            if (s.musicPlaying) {
                s.music->restart(s.musicSection);
            }
        }

        void mixMusic(State& s, float* out, size_t frames)
        {
            if (!s.music || !s.musicPlaying) {
                return;
            }
            if (s.scratch.size() < frames * 2) {
                s.scratch.resize(frames * 2);
            }
            s.music->render(s.scratch.data(), frames);
            for (size_t i = 0; i < frames; i++) {
                if (s.requested) {
                    s.musicFade -= s.fadeSpeed / static_cast<float>(s.rate);
                }
                int32_t master = static_cast<int32_t>(static_cast<Ext>(std::max(s.musicFade, 0.0f)) * static_cast<Ext>(s.musicVolume) * 127.0L);
                float gain = static_cast<float>(master * master) / 16129.0f;
                out[i * 2] += s.scratch[i * 2] * gain;
                out[i * 2 + 1] += s.scratch[i * 2 + 1] * gain;
            }
            if (s.requested && s.musicFade < -s.fadeSpeed * 0.002f * s.latency) {
                startRequestedMusic(s);
            }
        }

        void mixAll(State& s, float* out, size_t frames)
        {
            std::fill(out, out + frames * 2, 0.0f);
            if (s.send.size() < frames * 2) {
                s.send.resize(frames * 2);
            }
            std::fill(s.send.begin(), s.send.begin() + static_cast<std::ptrdiff_t>(frames * 2), 0.0f);
            for (Voice& voice : s.voices) {
                if (!voice.done && (!voice.pending || voice.stopping)) {
                    mixVoice(voice, out, s.send.data(), frames);
                }
            }
            s.reverb.process(s.send.data(), out, frames);
            s.voices.erase(std::remove_if(s.voices.begin(), s.voices.end(), [](const Voice& voice) {
                return voice.done;
            }),
                s.voices.end());
            mixMusic(s, out, frames);
            for (size_t i = 0; i < frames * 2; i++) {
                out[i] = std::clamp(out[i], -1.0f, 1.0f);
            }
        }

        void dataCallback(ma_device* device, void* output, const void*, ma_uint32 frameCount)
        {
            State& s = *static_cast<State*>(device->pUserData);
            std::lock_guard<std::mutex> lock(s.mutex);
            mixAll(s, static_cast<float*>(output), frameCount);
        }

        Voice makeVoice(State& s, std::shared_ptr<const DecodedSound> pcm, USound* sound, UObject* actor, int32_t slot, const Vector& location, float volume, float radius, float pitch, bool attenuate)
        {
            Voice voice;
            voice.pcm = std::move(pcm);
            voice.sound = sound;
            voice.actor = actor;
            voice.slot = slot;
            voice.location = location;
            voice.volume = std::max(volume, 0.0f);
            voice.radius = radius > 0 ? radius : 1600.0f;
            voice.pitch = pitch;
            voice.attenuate = attenuate;
            refreshGains(s, voice);
            return voice;
        }

        Voice* channelVoice(State& s, int32_t channel)
        {
            for (Voice& voice : s.voices) {
                if (alive(voice) && voice.channel == channel) {
                    return &voice;
                }
            }
            return nullptr;
        }

        int32_t addVoice(State& s, Voice voice, bool noOverride)
        {
            std::lock_guard<std::mutex> lock(s.mutex);
            int32_t index = -1;
            float best = voice.priority;
            for (int32_t channel = 0; channel < static_cast<int32_t>(s.channels); channel++) {
                Voice* other = channelVoice(s, channel);
                if (other && voice.actor && voice.slot != SlotNone && other->actor == voice.actor && other->slot == voice.slot) {
                    if (noOverride) {
                        return 0;
                    }
                    index = channel;
                    break;
                }
                float priority = other ? other->priority : 0.0f;
                if (priority <= best) {
                    index = channel;
                    best = priority;
                }
            }
            if (index < 0) {
                return 0;
            }
            Voice* replaced = channelVoice(s, index);
            if (replaced) {
                replaced->stopping = true;
                replaced->channel = -1;
            }
            voice.channel = index;
            voice.id = s.nextId++;
            int32_t id = voice.id;
            s.voices.push_back(std::move(voice));
            return id;
        }

        void stopVoice(State& s, int32_t id)
        {
            std::lock_guard<std::mutex> lock(s.mutex);
            for (Voice& voice : s.voices) {
                if (voice.id == id) {
                    voice.stopping = true;
                }
            }
        }

        bool voiceAlive(State& s, int32_t id)
        {
            std::lock_guard<std::mutex> lock(s.mutex);
            for (const Voice& voice : s.voices) {
                if (voice.id == id && alive(voice)) {
                    return true;
                }
            }
            return false;
        }

        bool ambientPlaying(State& s, UObject* actor)
        {
            std::lock_guard<std::mutex> lock(s.mutex);
            for (const Voice& voice : s.voices) {
                if (alive(voice) && voice.ambient && voice.actor == actor) {
                    return true;
                }
            }
            return false;
        }

        void updateAmbient(State& s, World* world, bool realtime)
        {
            if (realtime) {
                for (UObject* actor : world->actors) {
                    if (!actor || actor->destroyed || !actor->data) {
                        continue;
                    }
                    USound* sound = cast<USound>(actor::AmbientSound(actor));
                    if (!sound) {
                        continue;
                    }
                    float radius = ambientRadius(actor::SoundRadius(actor));
                    if (squaredDistance(actor::Location(actor), s.viewer) > static_cast<Ext>(radius) * static_cast<Ext>(radius) || ambientPlaying(s, actor)) {
                        continue;
                    }
                    objects::ensureLoaded(sound);
                    std::shared_ptr<const DecodedSound> pcm = decode(sound);
                    if (!pcm) {
                        continue;
                    }
                    float volume = static_cast<float>(static_cast<Ext>(actor::SoundVolume(actor)) * static_cast<Ext>(s.ambientFactor) * static_cast<Ext>(1.0 / 255.0));
                    float pitch = static_cast<float>(actor::SoundPitch(actor)) / 64.0f;
                    Voice voice = makeVoice(s, pcm, sound, actor, SlotAmbient, actor::Location(actor), volume, radius, pitch, true);
                    voice.loop = true;
                    voice.ambient = true;
                    addVoice(s, std::move(voice), false);
                }
            }
            float time = world->levelInfo && world->levelInfo->data ? levelinfo::TimeSeconds(world->levelInfo) : world->timeSeconds;
            std::lock_guard<std::mutex> lock(s.mutex);
            for (Voice& voice : s.voices) {
                if (!alive(voice) || !voice.ambient) {
                    continue;
                }
                UObject* actor = voice.actor;
                if (!actor || actor->destroyed || !actor->data || !realtime) {
                    voice.stopping = true;
                    continue;
                }
                float radius = ambientRadius(actor::SoundRadius(actor));
                if (squaredDistance(actor::Location(actor), s.viewer) > static_cast<Ext>(radius) * static_cast<Ext>(radius) || actor::AmbientSound(actor) != voice.sound) {
                    voice.stopping = true;
                    continue;
                }
                float brightness = static_cast<float>(2.0L * (static_cast<Ext>(actor::SoundVolume(actor)) * static_cast<Ext>(s.ambientFactor) * static_cast<Ext>(1.0 / 255.0)));
                voice.volume = ambientBrightness(actor, brightness, time);
                voice.radius = radius;
                voice.pitch = static_cast<float>(actor::SoundPitch(actor)) / 64.0f;
            }
        }

        void applyMusicRequest(World* world)
        {
            UObject* player = world ? world->player() : nullptr;
            if (!player || player->destroyed || !player->data || !isPlayerPawn(player)) {
                return;
            }
            uint8_t& transition = playerpawn::Transition(player);
            if (transition == 0) {
                return;
            }
            uint8_t request = transition;
            transition = 0;
            UMusic* music = cast<UMusic>(playerpawn::Song(player));
            playMusic(music, playerpawn::SongSection(player), request);
        }

        void readSettings(State& s)
        {
            std::string section = config::system().get("Engine.Engine", "AudioDevice").value_or("Galaxy.GalaxyAudioSubsystem");
            std::optional<std::string> channels = config::system().get(section, "EffectsChannels");
            if (channels) {
                s.channels = static_cast<size_t>(std::clamp(std::atoi(channels->c_str()), 1, 64));
            }
            std::optional<std::string> factor = config::system().get(section, "AmbientFactor");
            if (factor) {
                s.ambientFactor = static_cast<float>(std::atof(factor->c_str()));
            }
            std::optional<std::string> doppler = config::system().get(section, "DopplerSpeed");
            if (doppler) {
                s.dopplerSpeed = static_cast<float>(std::atof(doppler->c_str()));
            }
            std::optional<std::string> reverb = config::system().get(section, "UseReverb");
            if (reverb) {
                s.useReverb = reverb->size() > 0 && (reverb->front() == 'T' || reverb->front() == 't' || reverb->front() == '1');
            }
            std::optional<std::string> digital = config::system().get(section, "UseDigitalMusic");
            if (digital) {
                s.digitalMusic = digital->size() > 0 && (digital->front() == 'T' || digital->front() == 't' || digital->front() == '1');
            }
            std::optional<std::string> latency = config::system().get(section, "Latency");
            if (latency) {
                s.latency = static_cast<float>(std::atof(latency->c_str()));
            }
        }
    }

    size_t DecodedSound::frames() const
    {
        return channels == 0 ? 0 : samples.size() / channels;
    }

    float DecodedSound::duration() const
    {
        return sampleRate == 0 ? 0.0f : static_cast<float>(static_cast<double>(frames()) / static_cast<double>(sampleRate));
    }

    char speechPhoneme(const DecodedSound& pcm, size_t position, size_t end)
    {
        std::array<float, 1024> re {};
        std::array<float, 1024> im {};
        size_t frames = pcm.frames();
        for (size_t i = 0; i < 1024; i++) {
            float sample = position + i < std::min(frames, end) ? pcm.samples[(position + i) * pcm.channels] : 0.0f;
            float clamped = std::clamp(std::nearbyint(sample * 32768.0f), -32768.0f, 32767.0f);
            re[i] = clamped * (1.0f / 32768.0f) * (static_cast<float>(i) / 1024.0f);
        }
        fft(re, im);
        float peak = -99999.0f;
        size_t bin = 0;
        for (size_t k = 0; k < 512; k++) {
            float value = std::fabs(re[k]);
            if (peak < value) {
                peak = value;
                bin = k;
            }
        }
        if (peak <= 5.0f) {
            return 'X';
        }
        float frequency = static_cast<float>(bin) * static_cast<float>(pcm.sampleRate) * (1.0f / 1024.0f);
        if (frequency > 2000) {
            return 'T';
        }
        if (frequency > 1500) {
            return 'F';
        }
        if (frequency > 600) {
            return 'A';
        }
        if (frequency > 400) {
            return 'O';
        }
        if (frequency > 250) {
            return 'U';
        }
        return 'E';
    }

    float ambientRadius(uint8_t soundRadius)
    {
        return 25.0f * (static_cast<float>(soundRadius) + 1.0f);
    }

    std::shared_ptr<const DecodedSound> decodeBytes(const std::vector<uint8_t>& bytes)
    {
        if (bytes.empty()) {
            return nullptr;
        }
        ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);
        ma_decoder decoder;
        if (ma_decoder_init_memory(bytes.data(), bytes.size(), &config, &decoder) != MA_SUCCESS) {
            return nullptr;
        }
        auto result = std::make_shared<DecodedSound>();
        result->sampleRate = decoder.outputSampleRate;
        result->channels = decoder.outputChannels;
        std::vector<float> chunk(4096 * std::max<size_t>(result->channels, 1));
        while (true) {
            ma_uint64 read = 0;
            ma_result status = ma_decoder_read_pcm_frames(&decoder, chunk.data(), 4096, &read);
            result->samples.insert(result->samples.end(), chunk.begin(), chunk.begin() + static_cast<std::ptrdiff_t>(read * result->channels));
            if (status != MA_SUCCESS || read == 0) {
                break;
            }
        }
        ma_decoder_uninit(&decoder);
        if (result->channels == 0 || result->sampleRate == 0) {
            return nullptr;
        }
        readLoop(bytes, *result);
        result->compressed = bytes.size() < 4 || std::memcmp(bytes.data(), "RIFF", 4) != 0;
        result->fixed.resize(result->samples.size());
        for (size_t i = 0; i < result->samples.size(); i++) {
            result->fixed[i] = static_cast<int16_t>(std::clamp(std::nearbyint(result->samples[i] * 32768.0f), -32768.0f, 32767.0f));
        }
        return result;
    }

    std::shared_ptr<const DecodedSound> decode(USound* sound)
    {
        if (!sound) {
            return nullptr;
        }
        State& s = state();
        auto it = s.sounds.find(sound);
        if (it != s.sounds.end()) {
            return it->second;
        }
        std::shared_ptr<const DecodedSound> result = decodeBytes(sound->bytes);
        s.sounds.emplace(sound, result);
        if (result) {
            sound->duration = result->duration();
        }
        return result;
    }

    std::shared_ptr<const ItModule> loadModule(UMusic* music)
    {
        if (!music) {
            return nullptr;
        }
        State& s = state();
        auto it = s.modules.find(music);
        if (it != s.modules.end()) {
            return it->second;
        }
        auto module = std::make_shared<ItModule>();
        std::shared_ptr<const ItModule> result;
        if (module->load(music->bytes.data(), music->bytes.size())) {
            result = module;
        }
        s.modules.emplace(music, result);
        return result;
    }

    Placement place(const Vector& listenerLocation, const Rotator& listenerRotation, const Vector& source, float radius)
    {
        Coords axes = viewAxes(listenerRotation);
        Ext dx = static_cast<Ext>(source.x) - static_cast<Ext>(listenerLocation.x);
        Ext dy = static_cast<Ext>(source.y) - static_cast<Ext>(listenerLocation.y);
        Ext dz = static_cast<Ext>(source.z) - static_cast<Ext>(listenerLocation.z);
        auto project = [&](const Vector& axis) {
            return static_cast<float>(dx * static_cast<Ext>(axis.x) + dy * static_cast<Ext>(axis.y) + dz * static_cast<Ext>(axis.z));
        };
        float right = project(axes.yAxis);
        float up = project(axes.zAxis);
        float forward = project(axes.xAxis);
        Ext angle = static_cast<Ext>(static_cast<float>(std::atan2(static_cast<double>(right), static_cast<double>(std::fabs(forward)))));
        float center = static_cast<float>(0.1L * static_cast<Ext>(radius));
        Ext sum = static_cast<Ext>(up) * static_cast<Ext>(up) + static_cast<Ext>(right) * static_cast<Ext>(right) + static_cast<Ext>(forward) * static_cast<Ext>(forward);
        float sizeSquared = static_cast<float>(sum);
        Ext size = static_cast<Ext>(std::sqrt(static_cast<double>(sum)));
        if (static_cast<Ext>(center) * static_cast<Ext>(center) > static_cast<Ext>(sizeSquared)) {
            angle = size / static_cast<Ext>(center) * angle;
        }
        Ext pan = angle * static_cast<Ext>(28671.125f) * static_cast<Ext>(0.3183098861837907) + 16383.0L;
        Placement placement;
        placement.pan = std::clamp(static_cast<int32_t>(pan), 0, MaxPan);
        placement.attenuation = static_cast<float>(std::clamp(1.0L - size / static_cast<Ext>(radius), 0.0L, 1.0L));
        return placement;
    }

    int32_t masterVolume(uint8_t sound, uint8_t speech)
    {
        float soundLevel = static_cast<float>(static_cast<Ext>(sound) * static_cast<Ext>(1.0 / 255.0));
        Ext speechLevel = static_cast<Ext>(speech) * static_cast<Ext>(1.0 / 255.0);
        Ext loudest = static_cast<Ext>(soundLevel) < speechLevel ? speechLevel : static_cast<Ext>(soundLevel);
        return static_cast<int32_t>(loudest * 127.0L);
    }

    int32_t voiceVolume(float attenuation, float obstruction, uint8_t sound, uint8_t speech, bool speechSlot, float volume)
    {
        Ext clear = std::max(1.0L - static_cast<Ext>(obstruction + obstruction), static_cast<Ext>(0.33));
        Ext level = clear * static_cast<Ext>(attenuation) * categoryScale(sound, speech, speechSlot) * static_cast<Ext>(volume) * 32767.0L;
        int32_t value = std::clamp(static_cast<int32_t>(level), 0, 32767);
        return std::max(value, 128);
    }

    Gains channelGains(int32_t volume, int32_t pan, int32_t master, int32_t send)
    {
        std::array<int32_t, 2> levels = channelLevels(volume, pan, master, send);
        int32_t left = levels[0];
        int32_t right = levels[1];
        const float scale = 32767.0f / 1073741824.0f;
        Gains gains;
        gains.left = static_cast<float>(left) * scale;
        gains.right = static_cast<float>(right) * scale;
        return gains;
    }

    float dopplerShift(const Vector& velocity, const Vector& source, const Vector& listener, float speed)
    {
        Vector direction = (source - listener).normal();
        Ext along = static_cast<Ext>(direction.z) * static_cast<Ext>(velocity.z) + static_cast<Ext>(direction.y) * static_cast<Ext>(velocity.y) + static_cast<Ext>(direction.x) * static_cast<Ext>(velocity.x);
        return static_cast<float>(std::clamp(1.0L - along / static_cast<Ext>(speed), 0.5L, 2.0L));
    }

    uint32_t voiceStep(uint32_t sampleRate, float pitch, float doppler, uint32_t outputRate)
    {
        uint32_t frequency = static_cast<uint32_t>(static_cast<int32_t>(static_cast<Ext>(sampleRate) * static_cast<Ext>(pitch) * static_cast<Ext>(doppler)));
        return ((frequency / outputRate) << 16) + (((frequency % outputRate) << 16) / outputRate);
    }

    ReverbSettings zoneReverb(UObject* zone)
    {
        ReverbSettings settings;
        if (!zone || !zone->data || !zoneinfo::bReverbZone(zone)) {
            return settings;
        }
        settings.volume = static_cast<float>(static_cast<Ext>(zoneinfo::MasterGain(zone)) * static_cast<Ext>(1.0 / 255.0));
        settings.cutoff = static_cast<float>(std::clamp(zoneinfo::CutoffHz(zone), 0, 44100));
        for (int i = 0; i < 6; i++) {
            Ext delay = static_cast<Ext>(zoneinfo::Delay(zone, i)) * static_cast<Ext>(0.002f);
            Ext gain = static_cast<Ext>(zoneinfo::Gain(zone, i)) * static_cast<Ext>(1.0f / 255.0f);
            settings.delay[static_cast<size_t>(i)] = static_cast<float>(std::clamp(delay, static_cast<Ext>(0.001f), static_cast<Ext>(0.34f)));
            settings.gain[static_cast<size_t>(i)] = static_cast<float>(std::clamp(gain, static_cast<Ext>(0.001f), static_cast<Ext>(0.999f)));
        }
        return settings;
    }

    void Reverb::configure(const ReverbSettings& settings, uint32_t rate)
    {
        enabled = settings.volume != 0;
        if (!enabled) {
            lines.clear();
            return;
        }
        Ext bend = 1.0L - std::cos(static_cast<Ext>(settings.cutoff) * static_cast<Ext>(6.282f) / static_cast<Ext>(rate));
        smoothing = static_cast<float>(std::sqrt((bend + 2.0L) * bend) - bend);
        keep = static_cast<float>(1.0L - static_cast<Ext>(smoothing));
        volume = settings.volume;
        for (size_t i = 0; i < 6; i++) {
            float delay = settings.delay[i] == 0.0f ? 0.001f : settings.delay[i];
            Ext amount = 1.0L - static_cast<Ext>(settings.gain[i]);
            if (amount == 1.0L) {
                amount = static_cast<Ext>(0.999f);
            }
            delays[i] = static_cast<size_t>(static_cast<Ext>(rate) * static_cast<Ext>(delay));
            direct[i] = static_cast<float>(-amount);
            feedback[i] = static_cast<float>(amount);
            through[i] = static_cast<float>(1.0L - amount * amount);
        }
        lines.assign(3 * ReverbFrames * 2, 0.0f);
        lowpass.fill(0.0f);
        last.fill(0.0f);
        position = 0;
    }

    void Reverb::process(const float* send, float* stereo, size_t frames)
    {
        if (!enabled) {
            return;
        }
        const size_t mask = ReverbFrames - 1;
        for (size_t n = 0; n < frames; n++) {
            std::array<float, 2> wave = { (send[n * 2] + last[1]) * 0.5f, (send[n * 2 + 1] + last[0]) * 0.5f };
            for (size_t stage = 0; stage < 3; stage++) {
                float* line = lines.data() + stage * ReverbFrames * 2;
                for (size_t channel = 0; channel < 2; channel++) {
                    size_t tap = stage * 2 + channel;
                    size_t read = (position + ReverbFrames - delays[tap]) & mask;
                    float delayed = line[read * 2 + channel];
                    float out = wave[channel] * direct[tap];
                    lowpass[tap] = delayed * smoothing + lowpass[tap] * keep;
                    line[position * 2 + channel] = wave[channel] + lowpass[tap] * feedback[tap];
                    wave[channel] = out + delayed * through[tap];
                }
            }
            last = wave;
            stereo[n * 2] += wave[0] * volume;
            stereo[n * 2 + 1] += wave[1] * volume;
            position = (position + 1) & mask;
        }
    }

    bool Reverb::active() const
    {
        return enabled;
    }

    bool initialize()
    {
        State& s = state();
        if (s.active) {
            return true;
        }
        readSettings(s);
        ma_device_config config = ma_device_config_init(ma_device_type_playback);
        config.playback.format = ma_format_f32;
        config.playback.channels = 2;
        config.sampleRate = s.rate;
        config.dataCallback = dataCallback;
        config.pUserData = &s;
        if (ma_device_init(nullptr, &config, &s.device) != MA_SUCCESS) {
            ma_backend backend = ma_backend_null;
            if (ma_context_init(&backend, 1, nullptr, &s.context) != MA_SUCCESS) {
                return false;
            }
            s.hasContext = true;
            if (ma_device_init(&s.context, &config, &s.device) != MA_SUCCESS) {
                ma_context_uninit(&s.context);
                s.hasContext = false;
                return false;
            }
        }
        s.rate = s.device.sampleRate;
        if (ma_device_start(&s.device) != MA_SUCCESS) {
            ma_device_uninit(&s.device);
            if (s.hasContext) {
                ma_context_uninit(&s.context);
                s.hasContext = false;
            }
            return false;
        }
        s.active = true;
        s.offline = false;
        return true;
    }

    bool initializeOffline(uint32_t sampleRate)
    {
        State& s = state();
        if (s.active) {
            shutdown();
        }
        readSettings(s);
        s.sounds.clear();
        s.modules.clear();
        s.rate = sampleRate;
        s.active = true;
        s.offline = true;
        return true;
    }

    void shutdown()
    {
        State& s = state();
        if (!s.active) {
            return;
        }
        if (!s.offline) {
            ma_device_uninit(&s.device);
            if (s.hasContext) {
                ma_context_uninit(&s.context);
                s.hasContext = false;
            }
        }
        s.active = false;
        s.offline = false;
        s.voices.clear();
        s.reverb.configure({}, s.rate);
        s.reverbSettings = {};
        s.music.reset();
        s.pendingMusic.reset();
        s.musicObject = nullptr;
        s.musicSection = -1;
        s.musicPlaying = false;
        s.musicFade = 1;
        s.fadeSpeed = 0;
        s.requested = false;
        s.requestObject = nullptr;
        s.requestSection = -1;
        s.listener = {};
        s.listenerRotation = {};
        s.viewer = {};
        s.ear = {};
        s.sounds.clear();
        s.modules.clear();
    }

    void render(float* stereo, size_t frames)
    {
        State& s = state();
        std::lock_guard<std::mutex> lock(s.mutex);
        mixAll(s, stereo, frames);
    }

    void update(const Vector& listenerLocation, const Rotator& listenerRotation, float deltaSeconds)
    {
        State& s = state();
        if (!s.active) {
            return;
        }
        float delta = std::clamp(deltaSeconds, 0.0f, 1.0f);
        s.listener = listenerLocation;
        s.listenerRotation = listenerRotation;
        s.viewer = listenerLocation;
        s.ear = listenerLocation;
        World* world = World::current;
        applyMusicRequest(world);
        UObject* zone = nullptr;
        std::unordered_set<UObject*> present;
        if (world) {
            UObject* player = world->player();
            if (player && !player->destroyed && player->data) {
                UObject* view = isPlayerPawn(player) ? playerpawn::ViewTarget(player) : nullptr;
                if (!view || view->destroyed || !view->data) {
                    view = player;
                }
                s.viewer = actor::Location(view);
                s.ear = actor::Location(player);
                if (isPawn(player)) {
                    s.ear.z = pawn::EyeHeight(player) + s.ear.z;
                }
                zone = region(view).zone;
            }
            present.insert(world->actors.begin(), world->actors.end());
            bool realtime = !world->levelInfo || !world->levelInfo->data || levelinfo::Pauser(world->levelInfo).empty();
            updateAmbient(s, world, realtime);
        } else {
            std::lock_guard<std::mutex> lock(s.mutex);
            for (Voice& voice : s.voices) {
                if (voice.ambient) {
                    voice.stopping = true;
                }
            }
        }
        ReverbSettings settings = s.useReverb ? zoneReverb(zone) : ReverbSettings {};
        std::lock_guard<std::mutex> lock(s.mutex);
        for (Voice& voice : s.voices) {
            if (!alive(voice)) {
                continue;
            }
            bool live = voice.actor && present.count(voice.actor) && !voice.actor->destroyed && voice.actor->data;
            if (live) {
                voice.location = actor::Location(voice.actor);
            }
            voice.elapsed += delta;
            bool obstructed = live && voice.slot != SlotTalk && !world->fastLineCheck(s.ear, voice.location);
            voice.obstruction = obstructed ? std::min(voice.obstruction + delta, 0.5f) : std::max(voice.obstruction - delta, 0.0f);
            voice.doppler = live && voice.ambient ? dopplerShift(actor::Velocity(voice.actor), voice.location, s.viewer, s.dopplerSpeed) : 1.0f;
            refreshGains(s, voice);
            voice.pending = false;
            static const Name namePawn("Pawn");
            if (live && voice.slot == SlotTalk && voice.pcm && voice.elapsed >= voice.nextPhonemeTime && voice.actor->isA(namePawn) && pawn::bIsSpeaking(voice.actor)) {
                size_t start = voice.position;
                size_t end = voice.position - voice.position % 32767 + 32767;
                if (!voice.pcm->compressed) {
                    Ext rate = static_cast<Ext>(voice.pcm->sampleRate);
                    Ext played = static_cast<Ext>(static_cast<int64_t>(rate * static_cast<Ext>(voice.elapsed)));
                    start = static_cast<size_t>(played + rate * static_cast<Ext>(0.1));
                    end = voice.pcm->frames();
                }
                pawn::nextPhoneme(voice.actor) = std::string(1, speechPhoneme(*voice.pcm, start, end));
                voice.nextPhonemeTime += 0.1f;
            }
        }
        if (settings != s.reverbSettings) {
            s.reverb.configure(settings, s.rate);
            s.reverbSettings = settings;
        }
    }

    int32_t playSound(USound* sound, UObject* actor, int32_t slot, const Vector& location, float volume, float radius, float pitch, bool attenuate, bool noOverride)
    {
        State& s = state();
        if (!s.active || !sound) {
            return 0;
        }
        std::shared_ptr<const DecodedSound> pcm = decode(sound);
        if (!pcm) {
            return 0;
        }
        return addVoice(s, makeVoice(s, pcm, sound, actor, slot, location, volume, radius, pitch, attenuate), noOverride);
    }

    void actorDestroyed(UObject* actor)
    {
        State& s = state();
        if (!s.active || !actor) {
            return;
        }
        std::lock_guard<std::mutex> lock(s.mutex);
        for (Voice& voice : s.voices) {
            if (voice.actor != actor) {
                continue;
            }
            if (voice.ambient) {
                voice.stopping = true;
            }
            voice.actor = nullptr;
        }
    }

    void stopSound(int32_t id)
    {
        State& s = state();
        if (!s.active || id == 0) {
            return;
        }
        stopVoice(s, id);
    }

    bool isPlaying(int32_t id)
    {
        State& s = state();
        if (!s.active || id == 0) {
            return false;
        }
        return voiceAlive(s, id);
    }

    float duration(USound* sound)
    {
        std::shared_ptr<const DecodedSound> pcm = decode(sound);
        return pcm ? pcm->duration() : 0.0f;
    }

    void playMusic(UMusic* music, uint8_t section, uint8_t transition)
    {
        State& s = state();
        if (!s.active) {
            return;
        }
        std::unique_ptr<ModulePlayer> player;
        if (music && music != s.musicObject && s.digitalMusic) {
            objects::ensureLoaded(music);
            std::shared_ptr<const ItModule> module = loadModule(music);
            if (module) {
                player = std::make_unique<ModulePlayer>(module, s.rate);
            }
        }
        float speed = transition == TransitionFade ? 1.0f : (transition == TransitionFastFade ? 3.0f : (transition == TransitionSlowFade ? 0.2f : 0.0f));
        std::lock_guard<std::mutex> lock(s.mutex);
        s.requestObject = music;
        s.requestSection = section;
        s.pendingMusic = std::move(player);
        s.requested = true;
        if (speed == 0 || !s.music || !s.musicPlaying) {
            startRequestedMusic(s);
            return;
        }
        s.fadeSpeed = speed;
    }

    void setVolumes(float sound, float music, float speech)
    {
        State& s = state();
        std::lock_guard<std::mutex> lock(s.mutex);
        s.soundVolume = std::clamp(sound, 0.0f, 1.0f);
        s.musicVolume = std::clamp(music, 0.0f, 1.0f);
        s.speechVolume = std::clamp(speech, 0.0f, 1.0f);
    }

    Volumes volumes()
    {
        State& s = state();
        std::lock_guard<std::mutex> lock(s.mutex);
        Volumes result;
        result.sound = s.soundVolume;
        result.music = s.musicVolume;
        result.speech = s.speechVolume;
        return result;
    }

    MusicStatus musicStatus()
    {
        State& s = state();
        std::lock_guard<std::mutex> lock(s.mutex);
        MusicStatus status;
        status.music = s.musicObject;
        status.section = s.musicSection;
        if (s.music && s.musicPlaying) {
            status.order = s.music->order();
            status.row = s.music->row();
        }
        status.fade = s.music && s.musicPlaying ? s.musicFade : 0.0f;
        status.pending = s.requested;
        return status;
    }

    std::vector<VoiceStatus> voices()
    {
        State& s = state();
        std::lock_guard<std::mutex> lock(s.mutex);
        std::vector<VoiceStatus> result;
        for (const Voice& voice : s.voices) {
            if (!alive(voice)) {
                continue;
            }
            VoiceStatus status;
            status.id = voice.id;
            status.sound = voice.sound;
            status.actor = voice.actor;
            status.slot = voice.slot;
            status.gainLeft = voice.gainLeft;
            status.gainRight = voice.gainRight;
            status.priority = voice.priority;
            status.ambient = voice.ambient;
            status.channel = voice.channel;
            status.volume = voice.level;
            status.pan = voice.pan;
            status.obstruction = voice.obstruction;
            status.step = static_cast<double>(voice.step) / 65536.0;
            result.push_back(status);
        }
        return result;
    }

    ReverbSettings reverbSettings()
    {
        State& s = state();
        std::lock_guard<std::mutex> lock(s.mutex);
        return s.reverbSettings;
    }

    void setLightRandom(float (*random)(int32_t index))
    {
        lightRandomSource = random;
    }
}
