#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace dx::audio {
    constexpr int16_t NoteNone = -1;
    constexpr int16_t NoteOff = 255;

    struct ItSample {
        std::string name;
        uint8_t globalVolume = 64;
        uint8_t flags = 0;
        uint8_t volume = 64;
        uint8_t defaultPan = 32;
        uint32_t loopBegin = 0;
        uint32_t loopEnd = 0;
        uint32_t c5speed = 8363;
        uint32_t sustainBegin = 0;
        uint32_t sustainEnd = 0;
        uint8_t vibratoSpeed = 0;
        uint8_t vibratoDepth = 0;
        uint8_t vibratoRate = 0;
        uint8_t vibratoType = 0;
        std::vector<float> data;
    };

    struct ItEnvelope {
        uint8_t flags = 0;
        uint8_t loopBegin = 0;
        uint8_t loopEnd = 0;
        uint8_t sustainBegin = 0;
        uint8_t sustainEnd = 0;
        std::vector<std::pair<int8_t, uint16_t>> nodes;
    };

    struct ItInstrument {
        uint8_t newNoteAction = 0;
        uint16_t fadeOut = 0;
        uint8_t globalVolume = 128;
        uint8_t defaultPan = 0x80;
        std::array<uint8_t, 120> noteMap {};
        std::array<uint8_t, 120> sampleMap {};
        ItEnvelope volumeEnvelope;
        ItEnvelope panEnvelope;
        ItEnvelope pitchEnvelope;
    };

    struct ItCell {
        int16_t note = NoteNone;
        uint8_t instrument = 0;
        uint8_t volume = 255;
        uint8_t command = 0;
        uint8_t param = 0;
    };

    struct ItPattern {
        uint16_t rows = 64;
        std::vector<ItCell> cells;
    };

    struct ItModule {
        std::string title;
        uint16_t flags = 0;
        uint8_t globalVolume = 128;
        uint8_t mixVolume = 48;
        uint8_t speed = 6;
        uint8_t tempo = 125;
        uint8_t separation = 128;
        std::array<uint8_t, 64> channelPan {};
        std::array<uint8_t, 64> channelVolume {};
        std::vector<uint8_t> orders;
        std::vector<ItSample> samples;
        std::vector<ItInstrument> instruments;
        std::vector<ItPattern> patterns;

        bool load(const uint8_t* bytes, size_t size);
        bool useInstruments() const;
    };

    class ModulePlayer {
    public:
        ModulePlayer(std::shared_ptr<const ItModule> module, uint32_t sampleRate);

        void setPosition(int32_t order);
        void restart(int32_t order);
        void render(float* stereo, size_t frames);
        int32_t order() const;
        int32_t row() const;
        const ItModule& module() const;

    private:
        struct Channel {
            const ItSample* sample = nullptr;
            const ItInstrument* instrument = nullptr;
            uint8_t instrumentIndex = 0;
            int16_t lastNote = 60;
            double position = 0;
            int8_t direction = 1;
            bool active = false;
            bool keyOff = false;
            double frequency = 0;
            double targetFrequency = 0;
            double outputFrequency = 0;
            int32_t volume = 0x7fff;
            int32_t pan = 0x4000;
            float leftGain = 0;
            float rightGain = 0;
            float currentLeft = 0;
            float currentRight = 0;
            uint8_t command = 0;
            uint8_t param = 0;
            uint8_t volumeCommand = 255;
            uint8_t portaMemory = 0;
            uint8_t vibratoSpeed = 0;
            uint8_t vibratoDepth = 0;
            uint8_t vibratoPosition = 0;
            uint8_t vibratoWave = 0;
            int32_t vibratoOffset = 0;
            uint8_t offsetMemory = 0;
            int32_t noteCutTick = -1;
            int32_t noteDelayTick = -1;
            ItCell delayed;
            int32_t loopRow = 0;
            int32_t loopCount = 0;
            int32_t envelopeTick[3] = {};
            int32_t fade = 0x7fff;
            bool fading = false;
        };

        void tick();
        void processRow();
        void processCell(Channel& channel, const ItCell& cell);
        void triggerNote(Channel& channel, const ItCell& cell);
        void tickZeroEffects(Channel& channel);
        void tickEffects(Channel& channel);
        void volumeColumnTickZero(Channel& channel);
        void volumeSlide(Channel& channel, uint8_t param, bool firstTick);
        void retrigger(Channel& channel);
        void slidePitch(Channel& channel, int32_t units);
        void tonePortamento(Channel& channel, int32_t speed);
        void updateOutput(Channel& channel);
        int32_t envelopeValue(const ItEnvelope& envelope, Channel& channel, int index, int32_t fallback, int32_t scale);
        void advanceRow();
        void mixChannel(Channel& channel, float* stereo, size_t frames);
        double noteFrequency(const ItSample* sample, int32_t note) const;
        const ItPattern* currentPattern() const;
        void normalizeOrder();
        void resetSong();

        std::shared_ptr<const ItModule> song;
        uint32_t rate;
        std::vector<Channel> channels;
        int32_t currentOrder = 0;
        int32_t currentRow = 0;
        int32_t tickIndex = 0;
        int32_t speed = 6;
        int32_t tempo = 125;
        int32_t patternDelay = 0;
        int32_t rowTicks = 6;
        int32_t breakRow = -1;
        int32_t jumpOrder = -1;
        int32_t loopTarget = -1;
        double tickRemaining = 0;
    };
}
