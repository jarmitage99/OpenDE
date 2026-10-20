#pragma once

#include "core/math.h"
#include "core/object.h"

#include <functional>

namespace dx {
    class ULevel;
    class PackageWriter;
    class Writer;
    class UAIEventType;

    struct AIValues {
        float visual = 0;
        float volume = 0;
        float radius = 0;
        float smell = 0;
    };

    struct AIParams {
        UObject* bestActor = nullptr;
        float score = 0;
        float visibility = 0;
        float volume = 0;
        float smell = 0;
    };

    class UAIEvent : public UObject {
    public:
        void serialize(Reader& reader, Linker& source) override;

        UAIEventType* type = nullptr;
        UObject* actor = nullptr;
        int32_t deleted = 0;
        UAIEvent* next = nullptr;
    };

    class UAISenderEvent : public UAIEvent {
    public:
        void serialize(Reader& reader, Linker& source) override;

        float score = 0;
        AIValues slots[16];
        AIValues persist;
    };

    class UAIReceiverEvent : public UAIEvent {
    public:
        void serialize(Reader& reader, Linker& source) override;

        Name callback;
        Name scoreCallback;
        int32_t pending = 0;
        uint8_t state = 0;
        int32_t checkVisibility = 0;
        int32_t checkDirection = 0;
        int32_t checkCylinder = 0;
        int32_t checkLineOfSight = 0;
        int32_t active = 0;
        float lastScore = 0;
        UObject* lastBestActor = nullptr;
        int32_t lastSlot = 0;
        AIParams params;
        UAIReceiverEvent* procNext = nullptr;
        UAIReceiverEvent* procPrev = nullptr;
    };

    class UAIEventType : public UObject {
    public:
        void serialize(Reader& reader, Linker& source) override;

        Name eventName;
        uint32_t hash = 0;
        UAIEvent* senders = nullptr;
        UAIEvent* receivers = nullptr;
        UAIEventType* nextType = nullptr;
    };

    class UEventManager : public UObject {
    public:
        void serialize(Reader& reader, Linker& source) override;

        UAIEvent* findEvent(int32_t kind, Name eventName, UObject* actor, UAIEventType** outType, UAIEvent*** outLink);
        void setEventCallback(UObject* actor, Name eventName, Name callback, Name scoreCallback, bool checkVisibility, bool checkDirection, bool checkCylinder, bool checkLineOfSight);
        void clearEventCallback(UObject* actor, Name eventName);
        void sendEvent(UObject* actor, Name eventName, uint8_t eventType, float value, float radius, bool pulse);
        void endEvent(UObject* actor, Name eventName, uint8_t eventType);
        void clearEvent(UObject* actor, Name eventName);
        void safeDelete(UAIEvent* event);
        void process();
        void cleanupEvents();
        void destroyActor(UObject* actor);
        void tick();
        UAIReceiverEvent* firstProcess() const;

        ULevel* level = nullptr;
        int32_t refProcessing = 0;
        int32_t deleteCount = 0;
        int32_t curSlot = 0;
        UAIReceiverEvent* processHead = nullptr;
        UAIEventType* hash[256] = {};
    };

    extern bool aiTimeBudget;

    void registerAINatives();
    uint32_t aiEventHash(Name eventName);
    UEventManager* aiEventManager();
    void aiReferences(UObject* object, const std::function<void(UObject*)>& visit);
    bool aiWriteNative(PackageWriter& out, UObject* object, Writer& data);
    float aiLightLevel(UObject* self, const Vector& point);
    float aiVisibility(UObject* actor, bool includeVelocity);
    float aiCanSee(UObject* self, UObject* other, float visibility, bool checkVisibility, bool checkDirection, bool checkCylinder, bool checkLineOfSight);
    float aiCanHear(UObject* self, UObject* other, float volume, float radius);
    float aiCanSmell(UObject* self, UObject* other, float smell);
    bool lineOfSightTo(UObject* self, UObject* other, bool showSelf, bool ignoreDistance);
}
