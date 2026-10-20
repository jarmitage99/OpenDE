#pragma once

#include "core/math.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace dx {
    class UObject;
    class USound;
    class UMusic;

    namespace audio {
        struct ItModule;

        struct DecodedSound {
            uint32_t sampleRate = 0;
            uint32_t channels = 0;
            std::vector<float> samples;
            bool looped = false;
            size_t loopStart = 0;
            size_t loopEnd = 0;
            bool compressed = false;
            std::vector<int16_t> fixed;

            size_t frames() const;
            float duration() const;
        };

        struct Gains {
            float left = 0;
            float right = 0;
        };

        struct Volumes {
            float sound = 1;
            float music = 1;
            float speech = 1;
        };

        struct MusicStatus {
            const UMusic* music = nullptr;
            int32_t section = -1;
            int32_t order = -1;
            int32_t row = -1;
            float fade = 0;
            bool pending = false;
        };

        struct VoiceStatus {
            int32_t id = 0;
            const USound* sound = nullptr;
            const UObject* actor = nullptr;
            int32_t slot = 0;
            float gainLeft = 0;
            float gainRight = 0;
            float priority = 0;
            bool ambient = false;
            int32_t channel = -1;
            int32_t volume = 0;
            int32_t pan = 0;
            float obstruction = 0;
            double step = 0;
        };

        struct Placement {
            int32_t pan = 16383;
            float attenuation = 1;
        };

        struct ReverbSettings {
            float volume = 0;
            float cutoff = 0;
            std::array<float, 6> delay {};
            std::array<float, 6> gain {};

            bool operator==(const ReverbSettings&) const = default;
        };

        class Reverb {
        public:
            void configure(const ReverbSettings& settings, uint32_t rate);
            void process(const float* send, float* stereo, size_t frames);
            bool active() const;

        private:
            bool enabled = false;
            size_t position = 0;
            std::array<size_t, 6> delays {};
            std::array<float, 6> direct {};
            std::array<float, 6> feedback {};
            std::array<float, 6> through {};
            std::array<float, 6> lowpass {};
            std::array<float, 2> last {};
            float smoothing = 0;
            float keep = 0;
            float volume = 0;
            std::vector<float> lines;
        };

        constexpr int32_t SlotNone = 0;
        constexpr int32_t SlotAmbient = 4;
        constexpr int32_t SlotTalk = 5;

        bool initialize();
        bool initializeOffline(uint32_t sampleRate);
        void shutdown();
        void update(const Vector& listenerLocation, const Rotator& listenerRotation, float deltaSeconds);
        void render(float* stereo, size_t frames);
        int32_t playSound(USound* sound, UObject* actor, int32_t slot, const Vector& location, float volume, float radius, float pitch, bool attenuate, bool noOverride = false);
        void actorDestroyed(UObject* actor);
        void stopSound(int32_t id);
        bool isPlaying(int32_t id);
        float duration(USound* sound);
        void playMusic(UMusic* music, uint8_t section, uint8_t transition);
        void setVolumes(float sound, float music, float speech);
        Volumes volumes();
        MusicStatus musicStatus();
        std::vector<VoiceStatus> voices();

        std::shared_ptr<const DecodedSound> decode(USound* sound);
        std::shared_ptr<const DecodedSound> decodeBytes(const std::vector<uint8_t>& bytes);
        std::shared_ptr<const ItModule> loadModule(UMusic* music);
        Placement place(const Vector& listenerLocation, const Rotator& listenerRotation, const Vector& source, float radius);
        int32_t masterVolume(uint8_t sound, uint8_t speech);
        int32_t voiceVolume(float attenuation, float obstruction, uint8_t sound, uint8_t speech, bool speechSlot, float volume);
        Gains channelGains(int32_t volume, int32_t pan, int32_t master, int32_t send);
        float dopplerShift(const Vector& velocity, const Vector& source, const Vector& listener, float speed);
        uint32_t voiceStep(uint32_t sampleRate, float pitch, float doppler, uint32_t outputRate);
        ReverbSettings zoneReverb(UObject* zone);
        ReverbSettings reverbSettings();
        float ambientRadius(uint8_t soundRadius);
        char speechPhoneme(const DecodedSound& pcm, size_t position, size_t end = SIZE_MAX);
        void setLightRandom(float (*random)(int32_t index));
    }
}
