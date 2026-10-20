#include "engine/ai.h"

#include "core/objects.h"
#include "core/package_writer.h"
#include "core/vm.h"
#include "engine/actor.h"
#include "engine/reader.h"
#include "engine/resources.h"
#include "engine/world.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <map>
#include <random>
#include <vector>

namespace dx {
    namespace {
        constexpr uint8_t StateBegin = 0;
        constexpr uint8_t StateEnd = 1;
        constexpr uint8_t StatePulse = 2;
        constexpr uint8_t StateChangeBest = 3;
        constexpr int32_t MaxCandidates = 256;

        UAISenderEvent* candidates[MaxCandidates];

        World& world()
        {
            return *World::current;
        }

        float randomFloat()
        {
            static std::mt19937 generator(777);
            return std::uniform_real_distribution<float>(0.0f, 1.0f)(generator);
        }

        bool alive(UObject* a)
        {
            return a && !a->destroyed;
        }

        bool deleting(UObject* a)
        {
            return !a || actor::bDeleteMe(a) != 0;
        }

        UClass* eventClass(const char* name)
        {
            return objects::intrinsicClass(Name(name));
        }

        const uint32_t* crcTable()
        {
            static uint32_t table[256];
            static bool built = false;
            if (!built) {
                for (uint32_t i = 0; i < 256; i++) {
                    uint32_t value = i << 24;
                    for (int bit = 0; bit < 8; bit++) {
                        value = (value & 0x80000000u) ? (value << 1) ^ 0x04C11DB7u : value << 1;
                    }
                    table[i] = value;
                }
                built = true;
            }
            return table;
        }

        int compareEventNames(Name a, Name b)
        {
            if (a == b) {
                return 0;
            }
            const char* x = a.c_str();
            const char* y = b.c_str();
            while (true) {
                int cx = std::toupper(static_cast<unsigned char>(*x));
                int cy = std::toupper(static_cast<unsigned char>(*y));
                if (cx != cy || cx == 0) {
                    return cx - cy;
                }
                x++;
                y++;
            }
        }

        template <typename F>
        void forEachType(UEventManager& manager, F visit)
        {
            for (UAIEventType* bucket : manager.hash) {
                for (UAIEventType* type = bucket; type; type = type->nextType) {
                    visit(type);
                }
            }
        }

        void setValue(float& slot, float& persist, bool pulse, float value)
        {
            if (slot < value) {
                slot = value;
            }
            if (!pulse) {
                persist = value;
            }
        }

        void writeSender(UAISenderEvent* sender, int32_t slot, uint8_t eventType, bool pulse, float value, float radius)
        {
            AIValues& current = sender->slots[slot];
            if (eventType == 1) {
                setValue(current.volume, sender->persist.volume, pulse, value);
                setValue(current.radius, sender->persist.radius, pulse, radius);
            } else if (eventType == 0) {
                setValue(current.visual, sender->persist.visual, pulse, value);
            } else if (eventType == 2) {
                setValue(current.smell, sender->persist.smell, pulse, value);
            }
        }

        AIValues maxOverSlots(const UAISenderEvent* sender, int32_t from, int32_t to)
        {
            AIValues result;
            int32_t i = from;
            while (true) {
                const AIValues& value = sender->slots[i];
                result.visual = std::max(result.visual, value.visual);
                result.volume = std::max(result.volume, value.volume);
                result.radius = std::max(result.radius, value.radius);
                result.smell = std::max(result.smell, value.smell);
                if (i == to) {
                    break;
                }
                i = (i + 1) & 15;
            }
            return result;
        }

        bool emptySlots(const UAISenderEvent* sender)
        {
            for (const AIValues& value : sender->slots) {
                if (value.visual > 0 || value.volume > 0 || value.smell > 0) {
                    return false;
                }
            }
            return true;
        }

        float computeScore(UObject* receiver, UObject* sender, Name scoreCallback)
        {
            float score = (actor::Location(receiver) - actor::Location(sender)).sizeSquared() + 1.0f;
            if (scoreCallback.none()) {
                return score;
            }
            UFunction* function = vm::findFunction(receiver, scoreCallback);
            if (!function) {
                vm::fatal("Failed to find function " + scoreCallback.str() + " in " + receiver->fullName());
                return score;
            }
            vm::Call call(receiver, function);
            call.obj(receiver).obj(sender).f(score).run();
            uint8_t* value = call.returnValue();
            return value ? *reinterpret_cast<float*>(value) : 0.0f;
        }

        void senseDetection(const UEventManager& manager, UAIReceiverEvent* receiver, UAISenderEvent* sender, float& visibility, float& volume, float& smell)
        {
            UObject* self = receiver->actor;
            UObject* pawn = isPawn(self) ? self : nullptr;
            float sizeScale = 1.0f;
            UObject* source = sender->actor;
            if (source->isA(engineClasses().inventory) && actor::Owner(source)) {
                float area = actor::CollisionHeight(source) * actor::CollisionRadius(source);
                source = actor::Owner(source);
                float ownerArea = actor::CollisionHeight(source) * actor::CollisionRadius(source);
                if (ownerArea != 0) {
                    area /= ownerArea;
                }
                sizeScale = std::sqrt(area);
            }
            AIValues values = maxOverSlots(sender, receiver->lastSlot, manager.curSlot);
            visibility = values.visual * sizeScale;
            if (visibility > 0) {
                if (pawn) {
                    visibility = aiCanSee(pawn, source, visibility, receiver->checkVisibility != 0, receiver->checkDirection != 0, receiver->checkCylinder != 0, receiver->checkLineOfSight != 0);
                } else {
                    CheckResult hit;
                    if (world().lineCheck(hit, actor::Location(self), actor::Location(sender->actor), {}, self, TRACE_Movers | TRACE_Level)) {
                        visibility = 0;
                    }
                }
            }
            volume = values.volume;
            if (volume > 0) {
                if (pawn) {
                    volume = aiCanHear(pawn, source, volume, values.radius);
                } else {
                    float distSq = (actor::Location(sender->actor) - actor::Location(self)).sizeSquared();
                    volume = distSq < values.radius * values.radius ? values.volume : 0.0f;
                }
            }
            smell = values.smell;
            if (smell > 0) {
                smell = pawn ? aiCanSmell(pawn, source, smell) : 0.0f;
            }
        }

        void deliver(UObject* self, Name eventName, uint8_t eventState, const AIParams& params, Name callback)
        {
            if (!self || eventName.none()) {
                return;
            }
            if (callback.none()) {
                callback = Name("AIEvent");
            }
            UFunction* function = vm::findFunction(self, callback);
            if (!function) {
                return;
            }
            vm::Call call(self, function);
            if (!call.valid()) {
                return;
            }
            call.n(eventName).byte(eventState);
            uint8_t* memory = call.parameter(2);
            if (memory && function->params.size() > 2 && function->params[2]->structType) {
                UStruct* layout = function->params[2]->structType;
                auto set = [&](const char* field, auto value) {
                    UProperty* property = layout->findProperty(Name(field));
                    if (property) {
                        *reinterpret_cast<decltype(value)*>(memory + property->offset) = value;
                    }
                };
                set("BestActor", params.bestActor);
                set("Score", params.score);
                set("Visibility", params.visibility);
                set("Volume", params.volume);
                set("Smell", params.smell);
            }
            call.run();
        }

        void writeEventBase(PackageWriter& out, UAIEvent* event, Writer& data)
        {
            out.writeObject(data, event->type);
            out.writeObject(data, event->actor);
            data.i32(event->deleted);
            out.writeObject(data, event->next);
        }

        void writeValues(const AIValues& values, Writer& data)
        {
            data.f32(values.visual);
            data.f32(values.volume);
            data.f32(values.radius);
            data.f32(values.smell);
        }

        AIValues readValues(Reader& reader)
        {
            AIValues values;
            values.visual = reader.f32();
            values.volume = reader.f32();
            values.radius = reader.f32();
            values.smell = reader.f32();
            return values;
        }

        void nativeGetLightLevel(UObject* self, Frame& stack, void* result)
        {
            Vector location = vm::get<Vector>(stack);
            vm::finish(stack);
            vm::ret<float>(result, aiLightLevel(self, location));
        }

        void nativeVisibility(UObject* self, Frame& stack, void* result)
        {
            bool includeVelocity = vm::getBoolOpt(stack, true);
            vm::finish(stack);
            vm::ret<float>(result, aiVisibility(self, includeVelocity));
        }

        void nativeSetEventCallback(UObject* self, Frame& stack, void*)
        {
            Name event = vm::get<Name>(stack);
            Name callback = vm::get<Name>(stack);
            Name score = vm::getOpt<Name>(stack, Name());
            bool checkVisibility = vm::getBoolOpt(stack, true);
            bool checkDirection = vm::getBoolOpt(stack, true);
            bool checkCylinder = vm::getBoolOpt(stack, false);
            bool checkLineOfSight = vm::getBoolOpt(stack, true);
            vm::finish(stack);
            UEventManager* manager = aiEventManager();
            if (manager) {
                manager->setEventCallback(self, event, callback, score, checkVisibility, checkDirection, checkCylinder, checkLineOfSight);
            }
        }

        void nativeClearEventCallback(UObject* self, Frame& stack, void*)
        {
            Name event = vm::get<Name>(stack);
            vm::finish(stack);
            UEventManager* manager = aiEventManager();
            if (manager) {
                manager->clearEventCallback(self, event);
            }
        }

        void nativeSendEvent(UObject* self, Frame& stack, void*)
        {
            Name eventName = vm::get<Name>(stack);
            uint8_t type = vm::getOpt<uint8_t>(stack, 0);
            float value = vm::getOpt<float>(stack, 1.0f);
            float radius = vm::getOpt<float>(stack, 800.0f);
            vm::finish(stack);
            UEventManager* manager = aiEventManager();
            if (manager) {
                manager->sendEvent(self, eventName, type, value, radius, true);
            }
        }

        void nativeStartEvent(UObject* self, Frame& stack, void*)
        {
            Name eventName = vm::get<Name>(stack);
            uint8_t type = vm::getOpt<uint8_t>(stack, 0);
            float value = vm::getOpt<float>(stack, 1.0f);
            float radius = vm::getOpt<float>(stack, 800.0f);
            vm::finish(stack);
            UEventManager* manager = aiEventManager();
            if (manager) {
                manager->sendEvent(self, eventName, type, value, radius, false);
            }
        }

        void nativeEndEvent(UObject* self, Frame& stack, void*)
        {
            Name eventName = vm::get<Name>(stack);
            uint8_t type = vm::get<uint8_t>(stack);
            vm::finish(stack);
            UEventManager* manager = aiEventManager();
            if (manager) {
                manager->endEvent(self, eventName, type);
            }
        }

        void nativeClearEvent(UObject* self, Frame& stack, void*)
        {
            Name eventName = vm::get<Name>(stack);
            vm::finish(stack);
            UEventManager* manager = aiEventManager();
            if (manager) {
                manager->clearEvent(self, eventName);
            }
        }

        void nativeCanSee(UObject* self, Frame& stack, void* result)
        {
            UObject* other = vm::get<UObject*>(stack);
            float visibility = vm::getOpt<float>(stack, 1.0f);
            bool checkVisibility = vm::getBoolOpt(stack, true);
            bool checkDirection = vm::getBoolOpt(stack, true);
            bool checkCylinder = vm::getBoolOpt(stack, false);
            bool checkLineOfSight = vm::getBoolOpt(stack, true);
            vm::finish(stack);
            vm::ret<float>(result, aiCanSee(self, other, visibility, checkVisibility, checkDirection, checkCylinder, checkLineOfSight));
        }

        void nativeCanHear(UObject* self, Frame& stack, void* result)
        {
            UObject* other = vm::get<UObject*>(stack);
            float volume = vm::getOpt<float>(stack, 1.0f);
            float radius = vm::getOpt<float>(stack, -1.0f);
            vm::finish(stack);
            vm::ret<float>(result, aiCanHear(self, other, volume, radius));
        }

        void nativeCanSmell(UObject* self, Frame& stack, void*)
        {
            UObject* other = vm::get<UObject*>(stack);
            float smell = vm::getOpt<float>(stack, 1.0f);
            vm::finish(stack);
            aiCanSmell(self, other, smell);
        }
    }

    float aiLightLevel(UObject* self, const Vector& point)
    {
        auto intensity = [](uint8_t brightness, uint8_t saturation) {
            return (static_cast<double>(saturation) + 127.5) * (1.0 / 382.5) * (static_cast<double>(brightness) * (1.0 / 255.0));
        };
        UObject* zone = *reinterpret_cast<UObject**>(actor::Region(self));
        double light = 0.0;
        if (actor::bUnlit(self)) {
            light = 1.0;
        } else {
            for (UObject* a : world().actors) {
                if (!alive(a) || actor::LightType(a) == 0 || actor::LightBrightness(a) == 0 || a == self) {
                    continue;
                }
                float radius = 25.0f * (static_cast<float>(actor::LightRadius(a)) + 1.0f);
                float radiusSquared = radius * radius;
                float distanceSquared = (point - actor::Location(a)).sizeSquared();
                if (!(radiusSquared > distanceSquared)) {
                    continue;
                }
                CheckResult hit;
                if (actor::bStatic(a) && world().lineCheck(hit, actor::Location(a), point, {}, self, TRACE_Movers | TRACE_Level)) {
                    continue;
                }
                double attenuation = 1.0 - std::sqrt(static_cast<double>(distanceSquared / radiusSquared));
                light += intensity(actor::LightBrightness(a), actor::LightSaturation(a)) * attenuation;
            }
            light *= 0.5;
        }
        if (zone) {
            light += 2.0 * intensity(zoneinfo::AmbientBrightness(zone), zoneinfo::AmbientSaturation(zone));
        }
        return std::clamp(static_cast<float>(light), 0.0f, 1.0f);
    }

    float aiVisibility(UObject* a, bool includeVelocity)
    {
        const float interval = 0.25f;
        float now = levelinfo::TimeSeconds(world().levelInfo);
        float delta = now - actor::VisUpdateTime(a);
        float visibility = 0;
        if (delta > 2.0f * interval) {
            actor::VisUpdateTime(a) = now;
            visibility = aiLightLevel(a, actor::Location(a));
            actor::CurrentVisibility(a) = visibility;
            actor::LastVisibility(a) = visibility;
        } else if (delta > interval) {
            actor::LastVisibility(a) = actor::CurrentVisibility(a);
            actor::VisUpdateTime(a) = now;
            actor::CurrentVisibility(a) = aiLightLevel(a, actor::Location(a));
            visibility = actor::LastVisibility(a);
        } else {
            visibility = (actor::CurrentVisibility(a) - actor::LastVisibility(a)) * (delta / interval) + actor::LastVisibility(a);
        }
        if (includeVelocity) {
            float t = std::clamp((std::sqrt(actor::Velocity(a).sizeSquared()) - 30.0f) / (200.0f - 30.0f), 0.0f, 1.0f);
            visibility = static_cast<float>(static_cast<double>(t * visibility) * 0.5 + static_cast<double>(visibility));
        }
        return std::clamp(visibility, 0.0f, 1.0f);
    }

    namespace {
        float fovFactor(float angle, float fov, float blur)
        {
            while (angle <= -180.0f) {
                angle += 360.0f;
            }
            while (angle > 180.0f) {
                angle -= 360.0f;
            }
            if (angle < 0.0f) {
                angle = -angle;
            }
            float half = static_cast<float>(static_cast<double>(fov - blur) * 0.5);
            if (angle > half + blur) {
                return 0.0f;
            }
            if (angle <= half) {
                return 1.0f;
            }
            return static_cast<float>(1.0 - static_cast<double>((angle - half) / blur));
        }

        bool aiLineClear(UObject* viewer, UObject* target, const Vector& end, const Vector& start)
        {
            std::vector<CheckResult> hits = world().multiLineCheck(start, end, {}, nullptr, TRACE_AllColliding, 0x04);
            for (const CheckResult& hit : hits) {
                UObject* hitActor = hit.actor;
                bool owned = false;
                for (UObject* a = viewer; a; a = actor::Owner(a)) {
                    if (hitActor == a) {
                        owned = true;
                        break;
                    }
                }
                if (owned || hitActor == target) {
                    continue;
                }
                if (hitActor == world().levelInfo) {
                    return false;
                }
                if (actor::bBlockSight(hitActor) && !actor::bHidden(hitActor)) {
                    return false;
                }
            }
            return true;
        }
    }

    float aiCanSee(UObject* self, UObject* other, float visibility, bool checkVisibility, bool checkDirection, bool checkCylinder, bool checkLineOfSight)
    {
        if (!other || !(visibility > 0.0f) || !actor::bDetectable(other)) {
            return 0.0f;
        }
        Vector eye = actor::Location(self) + Vector { 0, 0, pawn::BaseEyeHeight(self) };
        Vector dir = actor::Location(other) - eye;
        double distanceSquared = static_cast<double>(dir.x) * static_cast<double>(dir.x) + static_cast<double>(dir.y) * static_cast<double>(dir.y) + static_cast<double>(dir.z) * static_cast<double>(dir.z);
        if (distanceSquared < 1.0) {
            distanceSquared = 1.0;
        }
        double radiusSquared = static_cast<double>(actor::CollisionRadius(other)) * static_cast<double>(actor::CollisionRadius(other));
        double heightSquared = static_cast<double>(actor::CollisionHeight(other)) * static_cast<double>(actor::CollisionHeight(other));
        double size = (heightSquared + radiusSquared) / distanceSquared;
        if (size <= static_cast<double>(pawn::MinAngularSize(self))) {
            return 0.0f;
        }
        if (size < 0.0003046792916483) {
            checkCylinder = false;
        }
        visibility = static_cast<float>(static_cast<double>(visibility) * size * 64.0);
        if (checkDirection && visibility > 0.0f) {
            float horizontalFov = pawn::AIHorizontalFov(self);
            float aspect = pawn::AspectRatio(self);
            float verticalFov = aspect > 0.0f ? horizontalFov / aspect : 0.0f;
            float horizontalBlur = static_cast<float>(std::atan(std::sqrt(radiusSquared / distanceSquared)) * 114.59155902616465);
            float verticalBlur = static_cast<float>(std::atan(std::sqrt(heightSquared / distanceSquared)) * 114.59155902616465);
            Rotator base = actor::Rotation(self);
            if (engineClasses().playerPawn && self->isA(engineClasses().playerPawn)) {
                base = pawn::ViewRotation(self);
            }
            const Rotator& add = pawn::AIAddViewRotation(self);
            Rotator viewRotation { base.pitch + add.pitch, base.yaw + add.yaw, base.roll + add.roll };
            Vector local = transformVector(rotationAxes(viewRotation), dir);
            Rotator angles = vectorRotation(local);
            float horizontal = fovFactor(static_cast<float>(static_cast<double>(angles.yaw) * 360.0 * (1.0 / 65536.0)), horizontalFov, horizontalBlur);
            visibility *= horizontal;
            float vertical = fovFactor(static_cast<float>(static_cast<double>(angles.pitch) * 360.0 * (1.0 / 65536.0)), verticalFov, verticalBlur);
            if (distanceSquared < 22500.0 && vertical < 0.75f) {
                vertical = 0.75f;
            }
            visibility *= vertical;
        }
        if (checkVisibility && visibility > 0.0f) {
            visibility *= aiVisibility(other, true);
        }
        visibility -= pawn::VisibilityThreshold(self);
        if (visibility < 0.0f) {
            visibility = 0.0f;
        } else if (!(visibility < 1.0f)) {
            visibility = 1.0f;
        }
        if (checkLineOfSight && visibility > 0.0f) {
            Vector location = actor::Location(other);
            if (!checkCylinder) {
                if (!aiLineClear(self, other, location, eye)) {
                    visibility = 0.0f;
                }
            } else {
                bool seen = false;
                bool player = engineClasses().playerPawn && other->isA(engineClasses().playerPawn);
                if (player) {
                    seen = aiLineClear(self, other, location + Vector { 0, 0, pawn::BaseEyeHeight(other) }, eye);
                }
                if (!seen) {
                    seen = aiLineClear(self, other, location + Vector { 0, 0, actor::CollisionHeight(other) }, eye);
                }
                if (!seen) {
                    seen = aiLineClear(self, other, location - Vector { 0, 0, actor::CollisionHeight(other) }, eye);
                }
                if (!player) {
                    Vector side = Vector { dir.x, dir.y, 0 }.normal();
                    float radius = actor::CollisionRadius(other);
                    if (!seen) {
                        seen = aiLineClear(self, other, { location.x + side.y * radius, location.y - side.x * radius, location.z }, eye);
                    }
                    if (!seen) {
                        seen = aiLineClear(self, other, { location.x - side.y * radius, location.y + side.x * radius, location.z }, eye);
                    }
                }
                if (!seen) {
                    visibility = 0.0f;
                }
            }
        }
        return visibility;
    }

    float aiCanHear(UObject* self, UObject* other, float volume, float radius)
    {
        if (!other || !(volume > 0.0f) || !actor::bDetectable(other)) {
            return 0.0f;
        }
        if (!(radius > 0.0f)) {
            radius = 800.0f;
        }
        Vector delta = actor::Location(other) - actor::Location(self);
        double dz = static_cast<double>(delta.z) * 2.0;
        double distanceSquared = static_cast<double>(delta.x) * static_cast<double>(delta.x) + static_cast<double>(delta.y) * static_cast<double>(delta.y) + dz * dz;
        if (!(distanceSquared < static_cast<double>(radius) * static_cast<double>(radius))) {
            return 0.0f;
        }
        float heard = static_cast<float>((1.0 - std::sqrt(distanceSquared) / static_cast<double>(radius)) * static_cast<double>(volume)) - pawn::HearingThreshold(self);
        if (heard < 0.0f) {
            return 0.0f;
        }
        if (!(heard < 1.0f)) {
            return 1.0f;
        }
        return heard;
    }

    float aiCanSmell(UObject*, UObject*, float)
    {
        return 0.0f;
    }

    bool lineOfSightTo(UObject* self, UObject* other, bool showSelf, bool ignoreDistance)
    {
        if (!other || other->destroyed || !World::current) {
            return false;
        }
        UObject* enemy = pawn::Enemy(self);
        if (other == enemy) {
            showSelf = false;
        } else if (showSelf) {
            pawn::bLOSflag(self) = pawn::bLOSflag(self) ? 0 : 1;
        }
        bool otherPawn = isPawn(other);
        bool isPlayer = pawn::bIsPlayer(self) != 0;
        Vector location = actor::Location(self);
        Vector otherLocation = actor::Location(other);
        float distSq = (otherLocation - location).sizeSquared();
        float maxDistSq = 0;
        if (showSelf) {
            if (otherPawn) {
                maxDistSq = pawn::SightRadius(self) * std::min(1.0f, static_cast<float>(pawn::Visibility(other)) * 0.0078125f);
                maxDistSq = std::min(isPlayer ? 16000000.0f : 12000000.0f, maxDistSq * maxDistSq);
            } else {
                maxDistSq = pawn::SightRadius(self) * pawn::SightRadius(self);
            }
            if (distSq > maxDistSq) {
                return false;
            }
            Vector sightDir = (otherLocation - location).normal();
            float stimulus = sightDir.dot(rotationVector(actor::Rotation(self))) - pawn::PeripheralVision(self);
            if (stimulus > 0) {
                stimulus = static_cast<float>(static_cast<double>(stimulus) * 0.8 + 0.2);
            } else {
                stimulus = static_cast<float>(static_cast<double>(stimulus) * 0.17 + 0.2);
            }
            pawn::Stimulus(self) = stimulus;
            if (stimulus <= 0) {
                return false;
            }
            float heightMod = std::abs(otherLocation.z - location.z) / std::max(1.0f, pawn::Skill(self) + 1.0f);
            distSq = (distSq + heightMod * heightMod) / (stimulus * stimulus);
            if (distSq > maxDistSq && !ignoreDistance) {
                return false;
            }
            pawn::Stimulus(self) = 1.0f;
        } else {
            if (isPlayer) {
                if (otherPawn) {
                    maxDistSq = 5000.0f * std::min(1.0f, static_cast<float>(pawn::Visibility(other) + 16) * 0.015f);
                    maxDistSq = std::min(25000000.0f, maxDistSq * maxDistSq);
                } else {
                    maxDistSq = 16000000.0f;
                }
            } else if (otherPawn) {
                maxDistSq = 4000.0f * std::min(1.0f, static_cast<float>(pawn::Visibility(other) + 16) * 0.015f);
                maxDistSq = std::min(16000000.0f, maxDistSq * maxDistSq);
            } else {
                maxDistSq = 9000000.0f;
            }
            if (distSq > maxDistSq && !ignoreDistance) {
                return false;
            }
        }
        World& w = world();
        Vector viewPoint = location + Vector { 0, 0, pawn::BaseEyeHeight(self) };
        if (other == enemy) {
            if (w.fastLineCheck(otherLocation, viewPoint) || w.fastLineCheck(otherLocation, location)) {
                pawn::LastSeeingPos(self) = location;
                pawn::LastSeenPos(self) = actor::Location(enemy);
                return true;
            }
            if (distSq > 1000000.0f && !ignoreDistance) {
                return false;
            }
        } else if (distSq > 1000000.0f && !ignoreDistance) {
            if (otherPawn) {
                if (!pawn::bLOSflag(self) && distSq > 0.5f * maxDistSq) {
                    return false;
                }
                if (!isPlayer && randomFloat() < 0.5f) {
                    return false;
                }
            }
            return w.fastLineCheck(otherLocation, viewPoint);
        }
        if (!showSelf || !pawn::bLOSflag(self)) {
            Vector body = otherLocation;
            body.z = static_cast<float>(static_cast<double>(body.z) + static_cast<double>(actor::CollisionHeight(other)) * 0.8);
            if (w.fastLineCheck(body, viewPoint)) {
                return true;
            }
        }
        if ((distSq > 250000.0f && !ignoreDistance) || !otherPawn) {
            return false;
        }
        float r = actor::CollisionRadius(other);
        Vector points[4] = {
            { otherLocation.x - r, otherLocation.y + r, otherLocation.z },
            { otherLocation.x + r, otherLocation.y + r, otherLocation.z },
            { otherLocation.x - r, otherLocation.y - r, otherLocation.z },
            { otherLocation.x + r, otherLocation.y - r, otherLocation.z },
        };
        int imin = 0;
        int imax = 0;
        float currentMin = points[0].sizeSquared();
        float currentMax = currentMin;
        for (int i = 1; i < 4; i++) {
            float size = points[i].sizeSquared();
            if (size > currentMax) {
                currentMax = size;
                imax = i;
            } else if (size < currentMin) {
                currentMin = size;
                imin = i;
            }
        }
        bool skip = pawn::bLOSflag(self) != 0;
        for (int i = 0; i < 4; i++) {
            if (i == imin || i == imax) {
                continue;
            }
            if (skip && showSelf) {
                skip = false;
            } else {
                skip = true;
                if (w.fastLineCheck(points[i], viewPoint)) {
                    return true;
                }
            }
        }
        return false;
    }

    bool aiTimeBudget = true;

    uint32_t aiEventHash(Name eventName)
    {
        const uint32_t* table = crcTable();
        uint32_t crc = 0;
        for (const char* text = eventName.c_str(); *text; text++) {
            uint32_t c = static_cast<unsigned char>(*text);
            if (c >= 'a' && c <= 'z') {
                c -= 0x20;
            }
            crc = (crc >> 8) ^ table[(crc ^ (c & 0xff)) & 0xff];
            crc = (crc >> 8) ^ table[(crc ^ (c >> 8)) & 0xff];
        }
        return crc;
    }

    UEventManager* aiEventManager()
    {
        if (!World::current || !World::current->levelInfo) {
            return nullptr;
        }
        return cast<UEventManager>(levelinfo::EventManager(World::current->levelInfo));
    }

    void UAIEvent::serialize(Reader& reader, Linker& source)
    {
        type = cast<UAIEventType>(source.readObject(reader));
        actor = source.readObject(reader);
        deleted = reader.i32();
        next = cast<UAIEvent>(source.readObject(reader));
    }

    void UAISenderEvent::serialize(Reader& reader, Linker& source)
    {
        UAIEvent::serialize(reader, source);
        for (AIValues& slot : slots) {
            slot = readValues(reader);
        }
        persist = readValues(reader);
    }

    void UAIReceiverEvent::serialize(Reader& reader, Linker& source)
    {
        UAIEvent::serialize(reader, source);
        callback = source.readName(reader);
        scoreCallback = source.readName(reader);
        pending = reader.i32();
        state = reader.u8();
        checkVisibility = reader.i32();
        checkDirection = reader.i32();
        checkCylinder = reader.i32();
        checkLineOfSight = reader.i32();
        active = reader.i32();
        lastScore = reader.f32();
        lastBestActor = source.readObject(reader);
        lastSlot = reader.i32();
        procNext = cast<UAIReceiverEvent>(source.readObject(reader));
        procPrev = cast<UAIReceiverEvent>(source.readObject(reader));
    }

    void UAIEventType::serialize(Reader& reader, Linker& source)
    {
        eventName = source.readName(reader);
        hash = reader.u32();
        senders = cast<UAIEvent>(source.readObject(reader));
        receivers = cast<UAIEvent>(source.readObject(reader));
        nextType = cast<UAIEventType>(source.readObject(reader));
    }

    void UEventManager::serialize(Reader& reader, Linker& source)
    {
        level = cast<ULevel>(source.readObject(reader));
        refProcessing = reader.i32();
        deleteCount = reader.i32();
        curSlot = reader.i32();
        processHead = cast<UAIReceiverEvent>(source.readObject(reader));
        for (UAIEventType*& bucket : hash) {
            bucket = cast<UAIEventType>(source.readObject(reader));
        }
    }

    UAIEvent* UEventManager::findEvent(int32_t kind, Name eventName, UObject* a, UAIEventType** outType, UAIEvent*** outLink)
    {
        if (outLink) {
            *outLink = nullptr;
        }
        bool create = outLink || outType;
        uint32_t full = aiEventHash(eventName);
        UAIEventType* less = nullptr;
        UAIEventType* type = hash[full & 0xff];
        while (type) {
            int order = compareEventNames(type->eventName, eventName);
            if (order < 0) {
                less = type;
                type = type->nextType;
                continue;
            }
            if (order > 0) {
                type = nullptr;
            }
            break;
        }
        if (!type && create) {
            type = static_cast<UAIEventType*>(objects::newObject(eventClass("AIEventType"), this, Name(), 0));
            type->eventName = eventName;
            type->hash = full;
            UAIEventType** link = less ? &less->nextType : &hash[full & 0xff];
            type->nextType = *link;
            *link = type;
        }
        UAIEvent* found = nullptr;
        UAIEvent* previous = nullptr;
        if (type) {
            for (UAIEvent* event = kind == 0 ? type->senders : type->receivers; event; event = event->next) {
                if (!event->deleted && event->actor == a) {
                    found = event;
                    break;
                }
                previous = event;
            }
        }
        if (outType) {
            *outType = type;
        }
        if (outLink && type) {
            *outLink = previous ? &previous->next : (kind == 0 ? &type->senders : &type->receivers);
        }
        return found;
    }

    void UEventManager::setEventCallback(UObject* a, Name eventName, Name callback, Name scoreCallback, bool checkVisibility, bool checkDirection, bool checkCylinder, bool checkLineOfSight)
    {
        if (!a || eventName.none()) {
            return;
        }
        UAIEventType* type = nullptr;
        UAIEvent** link = nullptr;
        UAIReceiverEvent* receiver = static_cast<UAIReceiverEvent*>(findEvent(1, eventName, a, &type, &link));
        if (!receiver && type && link) {
            receiver = static_cast<UAIReceiverEvent*>(objects::newObject(eventClass("AIReceiverEvent"), this, Name(), 0));
            receiver->actor = a;
            receiver->type = type;
            receiver->lastSlot = curSlot;
            if (processHead) {
                receiver->procNext = processHead;
                receiver->procPrev = processHead->procPrev;
                processHead->procPrev->procNext = receiver;
                processHead->procPrev = receiver;
            } else {
                processHead = receiver;
                receiver->procNext = receiver;
                receiver->procPrev = receiver;
            }
            receiver->next = *link;
            *link = receiver;
        }
        if (receiver) {
            receiver->callback = callback;
            receiver->scoreCallback = scoreCallback;
            receiver->checkVisibility = checkVisibility ? 1 : 0;
            receiver->checkDirection = checkDirection ? 1 : 0;
            receiver->checkCylinder = checkCylinder ? 1 : 0;
            receiver->checkLineOfSight = checkLineOfSight ? 1 : 0;
        }
    }

    void UEventManager::clearEventCallback(UObject* a, Name eventName)
    {
        if (!a || eventName.none()) {
            return;
        }
        UAIEvent* receiver = findEvent(1, eventName, a, nullptr, nullptr);
        if (receiver) {
            safeDelete(receiver);
        }
    }

    void UEventManager::sendEvent(UObject* a, Name eventName, uint8_t eventType, float value, float radius, bool pulse)
    {
        if (!a || eventName.none()) {
            return;
        }
        if (actor::bDeleteMe(a)) {
            value = 0;
            radius = 0;
        }
        UAIEventType* type = nullptr;
        UAIEvent** link = nullptr;
        UAISenderEvent* sender = static_cast<UAISenderEvent*>(findEvent(0, eventName, a, &type, &link));
        if (!sender && type && link) {
            sender = static_cast<UAISenderEvent*>(objects::newObject(eventClass("AISenderEvent"), this, Name(), 0));
            sender->type = type;
            sender->actor = a;
            sender->next = *link;
            *link = sender;
        }
        if (sender) {
            writeSender(sender, curSlot, eventType, pulse, value, radius);
        }
    }

    void UEventManager::endEvent(UObject* a, Name eventName, uint8_t eventType)
    {
        if (!a || eventName.none()) {
            return;
        }
        UAISenderEvent* sender = static_cast<UAISenderEvent*>(findEvent(0, eventName, a, nullptr, nullptr));
        if (sender) {
            writeSender(sender, curSlot, eventType, false, 0, 0);
        }
    }

    void UEventManager::clearEvent(UObject* a, Name eventName)
    {
        if (!a || eventName.none()) {
            return;
        }
        UAISenderEvent* sender = static_cast<UAISenderEvent*>(findEvent(0, eventName, a, nullptr, nullptr));
        if (sender) {
            writeSender(sender, curSlot, 1, false, 0, 0);
            writeSender(sender, curSlot, 0, false, 0, 0);
            writeSender(sender, curSlot, 2, false, 0, 0);
        }
    }

    void UEventManager::safeDelete(UAIEvent* event)
    {
        if (!event->deleted) {
            deleteCount++;
        }
        event->deleted = 1;
    }

    UAIReceiverEvent* UEventManager::firstProcess() const
    {
        UAIReceiverEvent* receiver = processHead;
        while (receiver) {
            if (!receiver->deleted) {
                return receiver;
            }
            receiver = receiver->procNext;
            if (receiver == processHead) {
                return nullptr;
            }
        }
        return nullptr;
    }

    void UEventManager::process()
    {
        if (!level) {
            return;
        }
        World& w = world();
        int32_t processed = 0;
        refProcessing++;
        auto start = std::chrono::steady_clock::now();
        int32_t newSlot = (curSlot + 1) & 15;
        UAIReceiverEvent* receiver = processHead;
        while (receiver) {
            bool scored = false;
            if (!receiver->deleted) {
                if (deleting(receiver->actor)) {
                    safeDelete(receiver);
                }
            }
            if (!receiver->deleted) {
                UObject* self = receiver->actor;
                UAIEventType* type = receiver->type;
                receiver->pending = 0;
                float bestScore = 0;
                UObject* bestActor = nullptr;
                receiver->params = AIParams();
                bool anyRange = false;
                if (type && !w.inStasis(self)) {
                    anyRange = w.timeSeconds - actor::LastRenderTime(self) <= 5.0f || actor::DistanceFromPlayer(self) <= 1200.0f;
                }
                int32_t count = 0;
                float distSq = 0;
                for (UAIEvent* event = type ? type->senders : nullptr; event; event = event->next) {
                    UAISenderEvent* sender = static_cast<UAISenderEvent*>(event);
                    if (sender->deleted || !sender->actor || sender->actor == self) {
                        continue;
                    }
                    if (!anyRange) {
                        distSq = (actor::Location(self) - actor::Location(sender->actor)).sizeSquared();
                    }
                    if (distSq > 160000.0f) {
                        continue;
                    }
                    sender->score = computeScore(self, sender->actor, receiver->scoreCallback);
                    if (sender->score > 0) {
                        candidates[count++] = sender;
                    }
                    if (count >= MaxCandidates) {
                        std::fprintf(stderr, "*** WARNING: Greater than %d objects emitting event \"%s\"! (Some events will be dropped)\n", MaxCandidates - 1, type->name.c_str());
                        break;
                    }
                }
                UAISenderEvent* best = nullptr;
                if (count > 0) {
                    scored = true;
                    if (count > 1) {
                        std::sort(candidates, candidates + count, [](const UAISenderEvent* a, const UAISenderEvent* b) {
                            if (a->score != b->score) {
                                return a->score < b->score;
                            }
                            return std::less<const UAISenderEvent*>()(a, b);
                        });
                    }
                    for (int32_t i = 0; i < count; i++) {
                        float visibility = 0;
                        float volume = 0;
                        float smell = 0;
                        senseDetection(*this, receiver, candidates[i], visibility, volume, smell);
                        if (visibility > 0 || volume > 0 || smell > 0) {
                            best = candidates[i];
                            bestActor = best->actor;
                            bestScore = best->score;
                            receiver->params = { best->actor, best->score, visibility, volume, smell };
                            break;
                        }
                    }
                }
                bool now = best != nullptr;
                if (!receiver->active) {
                    if (now) {
                        bool persist = best->persist.volume > 0 || best->persist.visual > 0 || best->persist.smell > 0;
                        receiver->pending = 1;
                        receiver->state = persist ? StateBegin : StatePulse;
                    }
                } else if (now) {
                    if (receiver->lastBestActor != bestActor) {
                        receiver->pending = 1;
                        receiver->state = StateChangeBest;
                    }
                } else {
                    receiver->pending = 1;
                    receiver->state = StateEnd;
                }
                receiver->active = now ? 1 : 0;
                if (receiver->pending && receiver->state == StatePulse) {
                    receiver->active = 0;
                }
                receiver->lastScore = bestScore;
                receiver->lastBestActor = bestActor;
            }
            processed++;
            if (scored && aiTimeBudget && std::chrono::steady_clock::now() - start > std::chrono::milliseconds(2)) {
                break;
            }
            receiver = receiver->procNext;
            if (receiver == processHead) {
                receiver = nullptr;
            }
        }
        curSlot = newSlot;
        forEachType(*this, [&](UAIEventType* type) {
            for (UAIEvent* event = type->senders; event; event = event->next) {
                UAISenderEvent* sender = static_cast<UAISenderEvent*>(event);
                if (sender->deleted) {
                    continue;
                }
                if (deleting(sender->actor)) {
                    sender->persist = AIValues();
                }
                sender->slots[newSlot] = sender->persist;
                if (emptySlots(sender)) {
                    safeDelete(sender);
                }
            }
        });
        UAIReceiverEvent* first = firstProcess();
        receiver = first;
        int32_t dropped = 0;
        while (receiver) {
            if (!receiver->deleted && receiver->lastSlot == newSlot) {
                receiver->lastSlot = (newSlot + 1) & 15;
                dropped++;
            }
            receiver = receiver->procNext;
            if (receiver == first) {
                break;
            }
        }
        if (dropped > 0) {
            std::fprintf(stderr, "*** WARNING: Event manager not cycling quickly enough.  %d events have been dropped.\n", dropped);
            std::fprintf(stderr, "             There may be too many events occurring on this level.\n");
        }
        receiver = processHead;
        int32_t index = 0;
        while (receiver) {
            if (!receiver->deleted) {
                if (receiver->pending && receiver->type) {
                    deliver(receiver->actor, receiver->type->eventName, receiver->state, receiver->params, receiver->callback);
                    receiver->pending = 0;
                }
                receiver->lastSlot = newSlot;
            }
            receiver = receiver->procNext;
            if (++index >= processed) {
                break;
            }
            if (receiver == processHead) {
                break;
            }
        }
        processHead = receiver;
        refProcessing--;
    }

    void UEventManager::cleanupEvents()
    {
        if (refProcessing != 0 || deleteCount <= 0) {
            return;
        }
        processHead = firstProcess();
        if (processHead) {
            std::vector<UAIReceiverEvent*> ring;
            UAIReceiverEvent* receiver = processHead;
            do {
                if (!receiver->deleted) {
                    ring.push_back(receiver);
                }
                receiver = receiver->procNext;
            } while (receiver && receiver != processHead);
            for (size_t i = 0; i < ring.size(); i++) {
                ring[i]->procNext = ring[(i + 1) % ring.size()];
                ring[i]->procPrev = ring[(i + ring.size() - 1) % ring.size()];
            }
        }
        auto cleanupSlot = [&](UAIEvent*& head) {
            UAIEvent** link = &head;
            while (*link) {
                UAIEvent* event = *link;
                if (event->deleted) {
                    *link = event->next;
                    objects::destroyObject(event);
                    deleteCount--;
                } else {
                    link = &event->next;
                }
            }
        };
        forEachType(*this, [&](UAIEventType* type) {
            cleanupSlot(type->senders);
            cleanupSlot(type->receivers);
        });
    }

    void UEventManager::destroyActor(UObject* a)
    {
        forEachType(*this, [&](UAIEventType* type) {
            for (UAIEvent* event = type->senders; event; event = event->next) {
                if (!event->deleted && event->actor == a) {
                    safeDelete(event);
                }
            }
            for (UAIEvent* event = type->receivers; event; event = event->next) {
                if (event->deleted) {
                    continue;
                }
                UAIReceiverEvent* receiver = static_cast<UAIReceiverEvent*>(event);
                if (receiver->lastBestActor == a) {
                    receiver->lastBestActor = nullptr;
                }
                if (receiver->actor == a) {
                    safeDelete(receiver);
                }
            }
        });
    }

    void UEventManager::tick()
    {
        process();
        cleanupEvents();
    }

    void aiReferences(UObject* object, const std::function<void(UObject*)>& visit)
    {
        if (UEventManager* manager = cast<UEventManager>(object)) {
            manager->cleanupEvents();
            visit(manager->level);
            visit(manager->processHead);
            for (UAIEventType* bucket : manager->hash) {
                visit(bucket);
            }
            return;
        }
        if (UAIEventType* type = cast<UAIEventType>(object)) {
            visit(type->senders);
            visit(type->receivers);
            visit(type->nextType);
            return;
        }
        UAIEvent* event = cast<UAIEvent>(object);
        if (!event) {
            return;
        }
        visit(event->type);
        visit(event->actor);
        visit(event->next);
        if (UAIReceiverEvent* receiver = cast<UAIReceiverEvent>(object)) {
            visit(receiver->lastBestActor);
            visit(receiver->procNext);
            visit(receiver->procPrev);
        }
    }

    bool aiWriteNative(PackageWriter& out, UObject* object, Writer& data)
    {
        if (UEventManager* manager = cast<UEventManager>(object)) {
            manager->cleanupEvents();
            out.writeObject(data, manager->level);
            data.i32(manager->refProcessing);
            data.i32(manager->deleteCount);
            data.i32(manager->curSlot);
            out.writeObject(data, manager->processHead);
            for (UAIEventType* bucket : manager->hash) {
                out.writeObject(data, bucket);
            }
            return true;
        }
        if (UAIEventType* type = cast<UAIEventType>(object)) {
            out.writeName(data, type->eventName);
            data.u32(type->hash);
            out.writeObject(data, type->senders);
            out.writeObject(data, type->receivers);
            out.writeObject(data, type->nextType);
            return true;
        }
        if (UAISenderEvent* sender = cast<UAISenderEvent>(object)) {
            writeEventBase(out, sender, data);
            for (const AIValues& slot : sender->slots) {
                writeValues(slot, data);
            }
            writeValues(sender->persist, data);
            return true;
        }
        if (UAIReceiverEvent* receiver = cast<UAIReceiverEvent>(object)) {
            writeEventBase(out, receiver, data);
            out.writeName(data, receiver->callback);
            out.writeName(data, receiver->scoreCallback);
            data.i32(receiver->pending);
            data.u8(receiver->state);
            data.i32(receiver->checkVisibility);
            data.i32(receiver->checkDirection);
            data.i32(receiver->checkCylinder);
            data.i32(receiver->checkLineOfSight);
            data.i32(receiver->active);
            data.f32(receiver->lastScore);
            out.writeObject(data, receiver->lastBestActor);
            data.i32(receiver->lastSlot);
            out.writeObject(data, receiver->procNext);
            out.writeObject(data, receiver->procPrev);
            return true;
        }
        return false;
    }

    void World::aiEvents()
    {
        UEventManager* manager = levelInfo ? cast<UEventManager>(levelinfo::EventManager(levelInfo)) : nullptr;
        if (manager) {
            manager->tick();
        }
    }

    void World::tickPawnSenses(UObject* a, float deltaSeconds)
    {
        float& counter = pawn::SightCounter(a);
        if (counter < 0) {
            counter += 0.2f;
        }
        counter -= deltaSeconds;
        if (pawn::bIsPlayer(a) && !actor::bHidden(a) && levelInfo) {
            for (UObject* other = levelinfo::PawnList(levelInfo); other; other = pawn::nextPawn(other)) {
                if (other == a || other->destroyed || pawn::SightCounter(other) >= 0 || !vm::findEvent(other, Name("SeePlayer"))) {
                    continue;
                }
                if (lineOfSightTo(other, a, true, false)) {
                    vm::Call(other, Name("SeePlayer")).obj(a).run();
                }
            }
        }
        if (counter < 0 && vm::findEvent(a, Name("EnemyNotVisible"))) {
            UObject* enemy = pawn::Enemy(a);
            if (enemy) {
                if (lineOfSightTo(a, enemy, false, false)) {
                    pawn::LastSeenTime(a) = timeSeconds;
                } else {
                    vm::event(a, Name("EnemyNotVisible"));
                }
            }
            pawn::SightCounter(a) = 0.1f;
        }
    }

    void registerAINatives()
    {
        vm::registerNative(700, nativeGetLightLevel);
        vm::registerNative(701, nativeVisibility);
        vm::registerNative(710, nativeSetEventCallback);
        vm::registerNative(711, nativeClearEventCallback);
        vm::registerNative(713, nativeSendEvent);
        vm::registerNative(714, nativeStartEvent);
        vm::registerNative(715, nativeEndEvent);
        vm::registerNative(716, nativeClearEvent);
        vm::registerNative(705, nativeCanSee);
        vm::registerNative(706, nativeCanHear);
        vm::registerNative(707, nativeCanSmell);
        objects::registerFactory(Name("EventManager"), [] { return new UEventManager(); });
        objects::registerFactory(Name("AIEventType"), [] { return new UAIEventType(); });
        objects::registerFactory(Name("AIEvent"), [] { return new UAIEvent(); });
        objects::registerFactory(Name("AISenderEvent"), [] { return new UAISenderEvent(); });
        objects::registerFactory(Name("AIReceiverEvent"), [] { return new UAIReceiverEvent(); });
    }
}
