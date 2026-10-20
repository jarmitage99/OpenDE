#include "audio/audio.h"
#include "core/config.h"
#include "core/objects.h"
#include "core/vm.h"
#include "engine/actor.h"
#include "engine/ai.h"
#include "engine/console.h"
#include "engine/mesh.h"
#include "engine/resources.h"
#include "engine/world.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>

namespace dx {
    void registerAnimNatives();
    void registerPawnNatives();
    void registerMiscNatives();
    int32_t clientHearSound(UObject* hearer, UObject* source, int32_t id, UObject* sound, const Vector& location, const Vector& parameters);
    Vector transformPointBy(const Vector& point, const Coords& coords);
    Coords coordsTranspose(const Coords& coords);
    Coords coordsTransform(const Coords& coords, const Coords& by);
    Coords coordsRotate(Coords coords, const Rotator& rotation);
    Coords coordsUnrotate(Coords coords, const Rotator& rotation);

    namespace {
        World& world()
        {
            return *World::current;
        }

        float randomFloat()
        {
            static std::mt19937 generator(4242);
            return std::uniform_real_distribution<float>(0.0f, 1.0f)(generator);
        }

        UClass* classParam(Frame& stack)
        {
            return cast<UClass>(vm::get<UObject*>(stack));
        }

        bool matches(UObject* a, UClass* cls)
        {
            return a && !a->destroyed && (!cls || a->isA(cls));
        }

        Color mulColor(const Color& c, float f)
        {
            auto scale = [f](uint8_t v) {
                return static_cast<uint8_t>(static_cast<int64_t>(static_cast<double>(v) * static_cast<double>(f)));
            };
            return { scale(c.r), scale(c.g), scale(c.b), c.a };
        }

        void nativeMultiplyColorFloat(UObject*, Frame& stack, void* result)
        {
            Color a = vm::get<Color>(stack);
            float b = vm::get<float>(stack);
            vm::finish(stack);
            vm::ret<Color>(result, mulColor(a, b));
        }

        void nativeMultiplyFloatColor(UObject*, Frame& stack, void* result)
        {
            float a = vm::get<float>(stack);
            Color b = vm::get<Color>(stack);
            vm::finish(stack);
            vm::ret<Color>(result, mulColor(b, a));
        }

        void nativeAddColorColor(UObject*, Frame& stack, void* result)
        {
            Color a = vm::get<Color>(stack);
            Color b = vm::get<Color>(stack);
            vm::finish(stack);
            auto add = [](uint8_t x, uint8_t y) {
                return static_cast<uint8_t>(x + y);
            };
            vm::ret<Color>(result, { add(a.r, b.r), add(a.g, b.g), add(a.b, b.b), a.a });
        }

        void nativeSubtractColorColor(UObject*, Frame& stack, void* result)
        {
            Color a = vm::get<Color>(stack);
            Color b = vm::get<Color>(stack);
            vm::finish(stack);
            auto sub = [](uint8_t x, uint8_t y) {
                return static_cast<uint8_t>(x - y);
            };
            vm::ret<Color>(result, { sub(a.r, b.r), sub(a.g, b.g), sub(a.b, b.b), a.a });
        }

        void nativeSpawn(UObject* self, Frame& stack, void* result)
        {
            UClass* cls = classParam(stack);
            UObject* owner = vm::getOpt<UObject*>(stack, nullptr);
            Name tag = vm::getOpt<Name>(stack, Name());
            Vector location = vm::getOpt<Vector>(stack, actor::Location(self));
            Rotator rotation = vm::getOpt<Rotator>(stack, actor::Rotation(self));
            vm::finish(stack);
            UObject* spawned = nullptr;
            if (cls && World::current) {
                spawned = world().spawn(cls, owner, Name(), location, rotation, nullptr, false, actor::Instigator(self));
            }
            if (spawned) {
                actor::Tag(spawned) = tag;
            }
            vm::ret<UObject*>(result, spawned);
        }

        void nativeDestroy(UObject* self, Frame& stack, void* result)
        {
            vm::finish(stack);
            vm::retBool(result, World::current && world().destroy(self));
        }

        void nativeSetTimer(UObject* self, Frame& stack, void*)
        {
            float rate = vm::get<float>(stack);
            bool loop = vm::getBool(stack);
            vm::finish(stack);
            actor::TimerCounter(self) = 0;
            actor::TimerRate(self) = rate;
            actor::bTimerLoop(self) = loop ? 1 : 0;
        }

        void nativeTrace(UObject* self, Frame& stack, void* result)
        {
            Vector* hitLocation = vm::getRef<Vector>(stack);
            Vector* hitNormal = vm::getRef<Vector>(stack);
            Vector end = vm::get<Vector>(stack);
            Vector start = vm::getOpt<Vector>(stack, actor::Location(self));
            bool traceActors = vm::getBoolOpt(stack, actor::bCollideActors(self) != 0);
            Vector extent = vm::getOpt<Vector>(stack, Vector {});
            vm::finish(stack);
            uint32_t flags = traceActors ? (TRACE_AllColliding | TRACE_OnlyProjActor) : TRACE_World;
            CheckResult hit;
            UObject* hitActor = nullptr;
            if (World::current && world().lineCheck(hit, start, end, extent, self, flags)) {
                *hitLocation = hit.location;
                *hitNormal = hit.normal;
                hitActor = hit.actor;
            } else {
                *hitLocation = {};
                *hitNormal = {};
            }
            vm::ret<UObject*>(result, hitActor);
        }

        void nativeFastTrace(UObject* self, Frame& stack, void* result)
        {
            Vector end = vm::get<Vector>(stack);
            Vector start = vm::getOpt<Vector>(stack, actor::Location(self));
            vm::finish(stack);
            vm::retBool(result, !World::current || world().fastLineCheck(end, start));
        }

        void nativeSetLocation(UObject* self, Frame& stack, void* result)
        {
            Vector location = vm::get<Vector>(stack);
            vm::finish(stack);
            vm::retBool(result, World::current && world().farMoveActor(self, location, false, false));
        }

        void nativeSetRotation(UObject* self, Frame& stack, void* result)
        {
            Rotator rotation = vm::get<Rotator>(stack);
            vm::finish(stack);
            CheckResult hit;
            vm::retBool(result, World::current && world().moveActor(self, {}, rotation, hit, false, false, false, false));
        }

        void nativeMove(UObject* self, Frame& stack, void* result)
        {
            Vector delta = vm::get<Vector>(stack);
            vm::finish(stack);
            CheckResult hit;
            vm::retBool(result, World::current && world().moveActor(self, delta, actor::Rotation(self), hit, false, false, false, false));
        }

        void nativeMoveSmooth(UObject* self, Frame& stack, void* result)
        {
            Vector delta = vm::get<Vector>(stack);
            vm::finish(stack);
            actor::bJustTeleported(self) = 0;
            vm::retBool(result, World::current && moveSmooth(self, delta));
        }

        void nativeSetCollision(UObject* self, Frame& stack, void*)
        {
            bool collide = vm::getBoolOpt(stack, actor::bCollideActors(self) != 0);
            bool blockActors = vm::getBoolOpt(stack, actor::bBlockActors(self) != 0);
            bool blockPlayers = vm::getBoolOpt(stack, actor::bBlockPlayers(self) != 0);
            vm::finish(stack);
            setCollision(self, collide, blockActors, blockPlayers);
        }

        void nativeSetCollisionSize(UObject* self, Frame& stack, void* result)
        {
            float radius = vm::get<float>(stack);
            float height = vm::get<float>(stack);
            vm::finish(stack);
            actor::CollisionRadius(self) = radius;
            actor::CollisionHeight(self) = height;
            if (World::current) {
                World::current->collisionHash.update(self);
            }
            vm::retBool(result, true);
        }

        void nativeSetBase(UObject* self, Frame& stack, void*)
        {
            UObject* base = vm::get<UObject*>(stack);
            vm::finish(stack);
            setBase(self, base, true);
        }

        void nativeSetOwner(UObject* self, Frame& stack, void*)
        {
            UObject* owner = vm::get<UObject*>(stack);
            vm::finish(stack);
            setOwner(self, owner);
        }

        void nativeSetPhysics(UObject* self, Frame& stack, void*)
        {
            uint8_t physics = vm::get<uint8_t>(stack);
            UObject* floor = vm::getOpt<UObject*>(stack, nullptr);
            vm::finish(stack);
            setPhysics(self, physics, floor);
        }

        void nativeAutonomousPhysics(UObject* self, Frame& stack, void*)
        {
            float delta = vm::get<float>(stack);
            vm::finish(stack);
            performPhysics(self, delta);
        }

        void nativeError(UObject* self, Frame& stack, void*)
        {
            std::string message = vm::get<std::string>(stack);
            vm::finish(stack);
            std::fprintf(stderr, "script error from %s: %s\n", self->name.c_str(), message.c_str());
        }

        template <typename Filter>
        void liveIterator(Frame& stack, UObject** out, Filter filter)
        {
            size_t index = 0;
            vm::iterate(stack, [&]() {
                while (World::current && index < world().actors.size()) {
                    UObject* a = world().actors[index++];
                    if (a && !a->destroyed && filter(a)) {
                        *out = a;
                        return true;
                    }
                }
                *out = nullptr;
                return false;
            });
        }

        void nativeAllActors(UObject*, Frame& stack, void*)
        {
            UClass* cls = classParam(stack);
            UObject** out = vm::getRef<UObject*>(stack);
            Name tag = vm::getOpt<Name>(stack, Name());
            vm::finish(stack);
            liveIterator(stack, out, [&](UObject* a) {
                return matches(a, cls) && (tag.none() || actor::Tag(a) == tag);
            });
        }

        bool isOwnedBy(UObject* a, UObject* owner)
        {
            for (UObject* current = a; current; current = actor::Owner(current)) {
                if (current == owner) {
                    return true;
                }
            }
            return false;
        }

        void nativeChildActors(UObject* self, Frame& stack, void*)
        {
            UClass* cls = classParam(stack);
            UObject** out = vm::getRef<UObject*>(stack);
            vm::finish(stack);
            liveIterator(stack, out, [&](UObject* a) {
                return matches(a, cls) && isOwnedBy(a, self);
            });
        }

        void nativeBasedActors(UObject* self, Frame& stack, void*)
        {
            UClass* cls = classParam(stack);
            UObject** out = vm::getRef<UObject*>(stack);
            vm::finish(stack);
            liveIterator(stack, out, [&](UObject* a) {
                return matches(a, cls) && actor::Base(a) == self;
            });
        }

        void nativeTouchingActors(UObject* self, Frame& stack, void*)
        {
            UClass* cls = classParam(stack);
            UObject** out = vm::getRef<UObject*>(stack);
            vm::finish(stack);
            int index = 0;
            vm::iterate(stack, [&]() {
                while (index < 4) {
                    UObject* a = actor::Touching(self, index++);
                    if (matches(a, cls)) {
                        *out = a;
                        return true;
                    }
                }
                *out = nullptr;
                return false;
            });
        }

        void nativeRadiusActors(UObject* self, Frame& stack, void*)
        {
            UClass* cls = classParam(stack);
            UObject** out = vm::getRef<UObject*>(stack);
            float radius = vm::get<float>(stack);
            Vector location = vm::getOpt<Vector>(stack, actor::Location(self));
            vm::finish(stack);
            liveIterator(stack, out, [&](UObject* a) {
                float reach = radius + actor::CollisionRadius(a);
                return matches(a, cls) && (actor::Location(a) - location).sizeSquared() < reach * reach;
            });
        }

        void nativeVisibleActors(UObject* self, Frame& stack, void*)
        {
            UClass* cls = classParam(stack);
            UObject** out = vm::getRef<UObject*>(stack);
            float radius = vm::getOpt<float>(stack, 0.0f);
            Vector location = vm::getOpt<Vector>(stack, actor::Location(self));
            vm::finish(stack);
            liveIterator(stack, out, [&](UObject* a) {
                if (actor::bHidden(a) || !matches(a, cls)) {
                    return false;
                }
                if (radius != 0 && !((actor::Location(a) - location).sizeSquared() < radius * radius)) {
                    return false;
                }
                return world().fastLineCheck(actor::Location(a), location);
            });
        }

        void nativeVisibleCollidingActors(UObject* self, Frame& stack, void*)
        {
            UClass* cls = classParam(stack);
            UObject** out = vm::getRef<UObject*>(stack);
            float radius = vm::getOpt<float>(stack, 0.0f);
            Vector location = vm::getOpt<Vector>(stack, actor::Location(self));
            bool ignoreHidden = vm::getBoolOpt(stack, false);
            vm::finish(stack);
            if (radius == 0) {
                radius = 1000.0f;
            }
            std::vector<UObject*> list;
            if (World::current) {
                Vector reach { radius, radius, radius };
                for (UObject* a : world().collisionHash.query(location - reach, location + reach)) {
                    if (a && !a->destroyed && actor::bCollideActors(a) && (actor::Location(a) - location).sizeSquared() < radius * radius) {
                        list.push_back(a);
                    }
                }
            }
            std::reverse(list.begin(), list.end());
            size_t index = 0;
            vm::iterate(stack, [&]() {
                while (index < list.size()) {
                    UObject* a = list[index++];
                    if (a->destroyed || !matches(a, cls) || (ignoreHidden && actor::bHidden(a))) {
                        continue;
                    }
                    if (world().fastLineCheck(actor::Location(a), location)) {
                        *out = a;
                        return true;
                    }
                }
                *out = nullptr;
                return false;
            });
        }

        void nativeTraceActors(UObject* self, Frame& stack, void*)
        {
            classParam(stack);
            UObject** out = vm::getRef<UObject*>(stack);
            Vector* hitLocation = vm::getRef<Vector>(stack);
            Vector* hitNormal = vm::getRef<Vector>(stack);
            Vector end = vm::get<Vector>(stack);
            Vector start = vm::getOpt<Vector>(stack, actor::Location(self));
            Vector extent = vm::getOpt<Vector>(stack, Vector {});
            vm::finish(stack);
            std::vector<CheckResult> hits;
            if (World::current) {
                hits = world().multiLineCheck(start, end, extent, nullptr, TRACE_AllColliding);
            }
            size_t index = 0;
            vm::iterate(stack, [&]() {
                while (index < hits.size()) {
                    const CheckResult& hit = hits[index++];
                    if (hit.actor) {
                        *out = hit.actor;
                        *hitLocation = hit.location;
                        *hitNormal = hit.normal;
                        return true;
                    }
                }
                *out = nullptr;
                return false;
            });
        }

        void nativeTraceVisibleActors(UObject* self, Frame& stack, void*)
        {
            classParam(stack);
            UObject** out = vm::getRef<UObject*>(stack);
            Vector* hitLocation = vm::getRef<Vector>(stack);
            Vector* hitNormal = vm::getRef<Vector>(stack);
            Vector end = vm::get<Vector>(stack);
            Vector start = vm::getOpt<Vector>(stack, actor::Location(self));
            Vector extent = vm::getOpt<Vector>(stack, Vector {});
            vm::finish(stack);
            std::vector<CheckResult> hits;
            if (World::current) {
                hits = world().multiLineCheck(start, end, extent, nullptr, TRACE_AllColliding, 4);
            }
            size_t index = 0;
            vm::iterate(stack, [&]() {
                while (index < hits.size()) {
                    const CheckResult& hit = hits[index++];
                    if (hit.actor) {
                        *out = hit.actor;
                        *hitLocation = hit.location;
                        *hitNormal = hit.normal;
                        return true;
                    }
                }
                *out = nullptr;
                return false;
            });
        }

        void nativeCycleActors(UObject*, Frame& stack, void*)
        {
            UClass* cls = classParam(stack);
            UObject** out = vm::getRef<UObject*>(stack);
            int32_t* index = vm::getRef<int32_t>(stack);
            vm::finish(stack);
            auto count = []() {
                return World::current ? world().actors.size() : size_t { 0 };
            };
            size_t slot = *index >= 0 && static_cast<size_t>(*index) < count() ? static_cast<size_t>(*index) : 0;
            size_t limit = count();
            size_t scanned = 0;
            vm::iterate(stack, [&]() {
                *out = nullptr;
                limit = std::min(limit, count());
                while (scanned < limit) {
                    if (slot >= world().actors.size()) {
                        slot = 0;
                    }
                    UObject* a = world().actors[slot++];
                    scanned++;
                    if (matches(a, cls)) {
                        *out = a;
                        break;
                    }
                }
                *index = static_cast<int32_t>(slot);
                return *out != nullptr;
            });
        }

        void nativeTraceTexture(UObject* self, Frame& stack, void*)
        {
            classParam(stack);
            UObject** out = vm::getRef<UObject*>(stack);
            Name* texName = vm::getRef<Name>(stack);
            Name* texGroup = vm::getRef<Name>(stack);
            int32_t* flags = vm::getRef<int32_t>(stack);
            Vector* hitLocation = vm::getRef<Vector>(stack);
            Vector* hitNormal = vm::getRef<Vector>(stack);
            Vector end = vm::get<Vector>(stack);
            Vector start = vm::getOpt<Vector>(stack, actor::Location(self));
            Vector extent = vm::getOpt<Vector>(stack, Vector {});
            vm::finish(stack);
            std::vector<CheckResult> hits;
            if (World::current) {
                hits = world().multiLineCheck(start, end, extent, nullptr, TRACE_AllColliding);
            }
            size_t index = 0;
            vm::iterate(stack, [&]() {
                while (index < hits.size()) {
                    const CheckResult& hit = hits[index++];
                    if (!hit.actor) {
                        continue;
                    }
                    *out = hit.actor;
                    *hitLocation = hit.location;
                    *hitNormal = hit.normal;
                    *texName = Name();
                    *texGroup = Name();
                    *flags = 0;
                    UModel* model = world().level ? world().level->model : nullptr;
                    if (hit.item >= 0 && model && hit.actor->isA(engineClasses().levelInfo) && static_cast<size_t>(hit.item) < model->nodes.size()) {
                        int32_t surf = model->nodes[static_cast<size_t>(hit.item)].surf;
                        if (surf >= 0 && static_cast<size_t>(surf) < model->surfs.size()) {
                            const BspSurf& face = model->surfs[static_cast<size_t>(surf)];
                            if (face.texture) {
                                *texName = face.texture->name;
                                if (face.texture->outer) {
                                    *texGroup = face.texture->outer->name;
                                }
                            }
                            *flags = static_cast<int32_t>(face.polyFlags);
                        }
                    }
                    return true;
                }
                *out = nullptr;
                return false;
            });
        }

        struct SoundRequest {
            UObject* sound = nullptr;
            uint8_t slot = 1;
            float volume = 0;
            bool noOverride = false;
            float radius = 0;
            float pitch = 1;
        };

        SoundRequest soundRequest(UObject* self, Frame& stack)
        {
            SoundRequest request;
            request.sound = vm::get<UObject*>(stack);
            request.slot = vm::getOpt<uint8_t>(stack, 1);
            request.volume = vm::getOpt<float>(stack, actor::TransientSoundVolume(self));
            request.noOverride = vm::getBoolOpt(stack, false);
            request.radius = vm::getOpt<float>(stack, actor::TransientSoundRadius(self));
            request.pitch = vm::getOpt<float>(stack, 1.0f);
            vm::finish(stack);
            return request;
        }

        int32_t checkHearSound(UObject* self, const SoundRequest& request, float radiusSquared)
        {
            UObject* hearer = World::current ? world().localPlayer : nullptr;
            if (!hearer || hearer->destroyed) {
                return 0;
            }
            int32_t id = static_cast<int32_t>(self->handle * 16u + request.slot * 2u + (request.noOverride ? 1u : 0u));
            Vector parameters { 100.0f * request.volume, request.radius, 100.0f * request.pitch };
            UObject* viewTarget = isPlayerPawn(hearer) ? playerpawn::ViewTarget(hearer) : nullptr;
            Vector hearLocation = viewTarget ? actor::Location(viewTarget) : actor::Location(hearer);
            float threshold = radiusSquared * 0.7692308f;
            float distanceSquared = (hearLocation - actor::Location(self)).sizeSquared();
            if (!(distanceSquared < threshold)) {
                return 0;
            }
            if (!world().fastLineCheck(hearLocation, actor::Location(self))) {
                if (actor::Instigator(self) != hearer) {
                    threshold *= 0.6f;
                }
                parameters.x *= 0.35f;
                if (distanceSquared > threshold) {
                    return 0;
                }
            }
            return clientHearSound(hearer, self, id, request.sound, actor::Location(self), parameters);
        }

        void nativePlaySound(UObject* self, Frame& stack, void* result)
        {
            SoundRequest request = soundRequest(self, stack);
            if (!request.sound) {
                return;
            }
            if (request.radius <= 0) {
                request.radius = 800.0f;
            }
            vm::ret<int32_t>(result, checkHearSound(self, request, request.radius * request.radius));
        }

        void nativePlayOwnedSound(UObject* self, Frame& stack, void*)
        {
            SoundRequest request = soundRequest(self, stack);
            if (!request.sound) {
                return;
            }
            float radius = request.radius != 0 ? request.radius : 1600.0f;
            checkHearSound(self, request, radius * radius);
        }

        void nativeStopSound(UObject*, Frame& stack, void*)
        {
            int32_t id = vm::get<int32_t>(stack);
            vm::finish(stack);
            audio::stopSound(id);
        }

        void nativeGetSoundDuration(UObject*, Frame& stack, void* result)
        {
            UObject* sound = vm::get<UObject*>(stack);
            vm::finish(stack);
            USound* data = cast<USound>(sound);
            float length = 0;
            if (data) {
                objects::ensureLoaded(data);
                length = audio::duration(data);
            }
            vm::ret<float>(result, length);
        }

        void nativeSetInstantSoundVolume(UObject*, Frame& stack, void*)
        {
            float value = static_cast<float>(vm::get<uint8_t>(stack)) / 255.0f;
            vm::finish(stack);
            audio::Volumes current = audio::volumes();
            audio::setVolumes(value, current.music, current.speech);
        }

        void nativeSetInstantSpeechVolume(UObject*, Frame& stack, void*)
        {
            float value = static_cast<float>(vm::get<uint8_t>(stack)) / 255.0f;
            vm::finish(stack);
            audio::Volumes current = audio::volumes();
            audio::setVolumes(current.sound, current.music, value);
        }

        void nativeSetInstantMusicVolume(UObject*, Frame& stack, void*)
        {
            float value = static_cast<float>(vm::get<uint8_t>(stack)) / 255.0f;
            vm::finish(stack);
            audio::Volumes current = audio::volumes();
            audio::setVolumes(current.sound, value, current.speech);
        }

        bool canHearNoise(UObject* listener, const Vector& noiseLocation, float loudness)
        {
            float distanceSquared = (actor::Location(listener) - noiseLocation).sizeSquared();
            if (loudness * loudness * 4000000.0f < distanceSquared) {
                return false;
            }
            if (distanceSquared == 0) {
                return true;
            }
            float perceived = 1200000.0f / distanceSquared;
            float alerted = perceived;
            if (perceived > 2.0f) {
                perceived = 2.0f;
                alerted = 0.5f;
            } else if (!(0.5f > perceived)) {
                alerted = 0.5f;
            }
            pawn::Stimulus(listener) = loudness * perceived + alerted * pawn::Alertness(listener);
            if (pawn::Stimulus(listener) < pawn::HearingThreshold(listener)) {
                return false;
            }
            return world().fastLineCheck(noiseLocation, actor::Location(listener));
        }

        bool rememberNoise(UObject* self, UObject* instigator, float loudness)
        {
            float now = world().timeSeconds;
            Vector location = actor::Location(self);
            auto near = [&](const Vector& spot) {
                return (spot - location).sizeSquared() < 2500.0f;
            };
            double recent = static_cast<double>(now) - 0.2;
            if (static_cast<double>(pawn::noise1time(instigator)) > recent && near(pawn::noise1spot(instigator)) && !(static_cast<double>(loudness) * 0.9 > static_cast<double>(pawn::noise1loudness(instigator)))) {
                return false;
            }
            if (static_cast<double>(pawn::noise2time(instigator)) > recent && near(pawn::noise2spot(instigator)) && !(static_cast<double>(loudness) * 0.9 > static_cast<double>(pawn::noise2loudness(instigator)))) {
                return false;
            }
            double stale = static_cast<double>(now) - 0.18;
            bool first = static_cast<double>(pawn::noise1time(instigator)) < stale;
            bool second = !first && static_cast<double>(pawn::noise2time(instigator)) < stale;
            if (!first && !second) {
                first = (near(pawn::noise1spot(instigator)) && !(pawn::noise1loudness(instigator) > loudness)) || !(pawn::noise2loudness(instigator) > loudness);
            }
            if (first) {
                pawn::noise1time(instigator) = now;
                pawn::noise1spot(instigator) = location;
                pawn::noise1loudness(instigator) = loudness;
            } else if (second) {
                pawn::noise2time(instigator) = now;
                pawn::noise2spot(instigator) = location;
                pawn::noise2loudness(instigator) = loudness;
            }
            return true;
        }

        void nativeMakeNoise(UObject* self, Frame& stack, void*)
        {
            float loudness = vm::get<float>(stack);
            vm::finish(stack);
            UObject* instigator = actor::Instigator(self);
            if (!World::current || !instigator || !rememberNoise(self, instigator, loudness)) {
                return;
            }
            UObject* enemy = pawn::Enemy(instigator);
            bool fromPlayer = pawn::bIsPlayer(instigator) || (enemy && pawn::bIsPlayer(enemy));
            static const Name hearNoise("HearNoise");
            for (UObject* listener = levelinfo::PawnList(world().levelInfo); listener; listener = pawn::nextPawn(listener)) {
                if (listener == instigator || !vm::isProbing(listener, hearNoise)) {
                    continue;
                }
                if (fromPlayer && isPlayerPawn(listener)) {
                    continue;
                }
                if (!fromPlayer && !listener->isA(self->cls) && !self->isA(listener->cls)) {
                    continue;
                }
                if (canHearNoise(listener, actor::Location(self), loudness)) {
                    vm::Call(listener, hearNoise).f(loudness).obj(self).run();
                }
            }
        }

        void nativePlayerCanSeeMe(UObject* self, Frame& stack, void* result)
        {
            vm::finish(stack);
            bool visible = false;
            UObject* viewer = World::current ? world().localPlayer : nullptr;
            if (viewer && playerpawn::ViewTarget(viewer) == self) {
                visible = true;
            } else if (viewer) {
                Vector direction = actor::Location(self) - actor::Location(viewer);
                float distanceSquared = direction.sizeSquared();
                bool inRange = (actor::CollisionRadius(self) + 3.6f) * 100000.0f > distanceSquared;
                float facing = rotationVector(pawn::ViewRotation(viewer)).dot(direction);
                bool inView = pawn::bBehindView(viewer) || !(0.25f * distanceSquared > facing * facing);
                visible = inRange && inView && lineOfSightTo(viewer, self, false, false);
            }
            vm::retBool(result, visible);
        }

        void nativeGetURLMap(UObject*, Frame& stack, void* result)
        {
            vm::finish(stack);
            std::string map;
            if (World::current) {
                map = world().url;
                size_t cut = map.find_first_of("?#");
                if (cut != std::string::npos) {
                    map = map.substr(0, cut);
                }
                size_t dot = map.rfind('.');
                if (dot != std::string::npos) {
                    map = map.substr(0, dot);
                }
            }
            vm::ret<std::string>(result, map);
        }

        void nativeGetMapName(UObject*, Frame& stack, void* result)
        {
            std::string prefix = vm::get<std::string>(stack);
            std::string current = vm::get<std::string>(stack);
            int32_t direction = vm::get<int32_t>(stack);
            vm::finish(stack);
            std::vector<std::string> maps;
            std::filesystem::path directory = objects::gameRoot() / "Maps";
            if (std::filesystem::is_directory(directory)) {
                for (const auto& item : std::filesystem::directory_iterator(directory)) {
                    std::string file = item.path().filename().string();
                    if (prefix.empty() || iequals(file.substr(0, prefix.size()), prefix)) {
                        maps.push_back(file);
                    }
                }
            }
            std::sort(maps.begin(), maps.end());
            std::string chosen;
            if (!maps.empty()) {
                auto it = std::find(maps.begin(), maps.end(), current);
                if (it == maps.end()) {
                    chosen = maps.front();
                } else {
                    ptrdiff_t index = (it - maps.begin()) + direction;
                    ptrdiff_t count = static_cast<ptrdiff_t>(maps.size());
                    chosen = maps[static_cast<size_t>(((index % count) + count) % count)];
                }
            }
            vm::ret<std::string>(result, chosen);
        }

        void nativeGetNextSkin(UObject*, Frame& stack, void*)
        {
            vm::get<std::string>(stack);
            vm::get<std::string>(stack);
            vm::get<int32_t>(stack);
            std::string* skin = vm::getRef<std::string>(stack);
            std::string* description = vm::getRef<std::string>(stack);
            vm::finish(stack);
            skin->clear();
            description->clear();
        }

        void nativeGetNextInt(UObject*, Frame& stack, void* result)
        {
            std::string className = vm::get<std::string>(stack);
            int32_t number = vm::get<int32_t>(stack);
            vm::finish(stack);
            std::string entry;
            int32_t found = 0;
            const char* packages[] = { "Engine", "DeusEx", "Extension", "ConSys" };
            for (const char* package : packages) {
                Ini* ini = config::localization(package);
                if (!ini) {
                    continue;
                }
                for (const auto& [key, value] : ini->section("Public")) {
                    if (!iequals(key, "Object")) {
                        continue;
                    }
                    if (value.find(className) == std::string::npos) {
                        continue;
                    }
                    if (found++ == number) {
                        size_t start = value.find("Name=");
                        if (start != std::string::npos) {
                            size_t end = value.find_first_of(",)", start);
                            entry = value.substr(start + 5, end - start - 5);
                        }
                    }
                }
            }
            vm::ret<std::string>(result, entry);
        }

        void nativeGetNextIntDesc(UObject*, Frame& stack, void*)
        {
            vm::get<std::string>(stack);
            vm::get<int32_t>(stack);
            std::string* entry = vm::getRef<std::string>(stack);
            std::string* description = vm::getRef<std::string>(stack);
            vm::finish(stack);
            entry->clear();
            description->clear();
        }

        float sheerSnap(float sheer)
        {
            double value = static_cast<double>(sheer);
            if (value < -0.65) {
                return static_cast<float>(value + 0.15);
            }
            if (value > 0.65) {
                return static_cast<float>(value - 0.15);
            }
            if (value < -0.55) {
                return -0.5f;
            }
            if (value > 0.55) {
                return 0.5f;
            }
            if (value < -0.05) {
                return static_cast<float>(value + 0.05);
            }
            if (value > 0.05) {
                return static_cast<float>(value - 0.05);
            }
            return 0.0f;
        }

        Coords coordsScale(Coords coords, UObject* self, const char* property)
        {
            UProperty* scaleProperty = self->cls->findProperty(Name(property));
            uint8_t* data = self->data + scaleProperty->offset;
            UStruct* type = scaleProperty->structType;
            Vector scale = *reinterpret_cast<Vector*>(data + type->findProperty(Name("Scale"))->offset);
            float sheer = sheerSnap(*reinterpret_cast<float*>(data + type->findProperty(Name("SheerRate"))->offset));
            uint8_t axis = *(data + type->findProperty(Name("SheerAxis"))->offset);
            Coords sheering { {}, { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
            if (axis == 1) {
                sheering.xAxis.y = sheer;
            } else if (axis == 2) {
                sheering.xAxis.z = sheer;
            } else if (axis == 3) {
                sheering.yAxis.x = sheer;
            } else if (axis == 4) {
                sheering.yAxis.z = sheer;
            } else if (axis == 5) {
                sheering.zAxis.x = sheer;
            } else if (axis == 6) {
                sheering.zAxis.y = sheer;
            }
            coords = coordsTransform(coords, sheering);
            coords.xAxis = coords.xAxis * scale;
            coords.yAxis = coords.yAxis * scale;
            coords.zAxis = coords.zAxis * scale;
            coords.origin = { coords.origin.x / scale.x, coords.origin.y / scale.y, coords.origin.z / scale.z };
            return coords;
        }

        void includePoint(Vector& minimum, Vector& maximum, bool& valid, const Vector& point)
        {
            if (!valid) {
                minimum = point;
                maximum = point;
                valid = true;
                return;
            }
            minimum = { std::min(minimum.x, point.x), std::min(minimum.y, point.y), std::min(minimum.z, point.z) };
            maximum = { std::max(maximum.x, point.x), std::max(maximum.y, point.y), std::max(maximum.z, point.z) };
        }

        void includeBox(Vector& minimum, Vector& maximum, bool& valid, const Vector& boxMin, const Vector& boxMax, const Coords& coords)
        {
            for (int i = 0; i < 8; i++) {
                Vector corner { (i & 4) ? boxMax.x : boxMin.x, (i & 2) ? boxMax.y : boxMin.y, (i & 1) ? boxMax.z : boxMin.z };
                includePoint(minimum, maximum, valid, transformPointBy(corner, coords));
            }
        }

        void nativeGetBoundingBox(UObject* self, Frame& stack, void* result)
        {
            Vector* minimum = vm::getRef<Vector>(stack);
            Vector* maximum = vm::getRef<Vector>(stack);
            bool exact = vm::getBoolOpt(stack, false);
            Vector location = vm::getOpt<Vector>(stack, actor::Location(self));
            Rotator rotation = vm::getOpt<Rotator>(stack, actor::Rotation(self));
            vm::finish(stack);
            Vector boxMin;
            Vector boxMax;
            bool valid = false;
            UModel* model = cast<UModel>(actor::Brush(self));
            UMesh* mesh = cast<UMesh>(actor::Mesh(self));
            const Coords unit { {}, { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
            if (model) {
                objects::ensureLoaded(model);
                Coords coords = unit;
                coords.origin = coords.origin - location;
                if (isBrush(self)) {
                    coords = coordsScale(coords, self, "PostScale");
                    coords = coordsRotate(coords, rotation);
                    coords = coordsScale(coords, self, "MainScale");
                    coords.origin = coords.origin + actor::PrePivot(self);
                } else {
                    coords = coordsRotate(coords, rotation);
                }
                if (!exact || !model->polys) {
                    includeBox(boxMin, boxMax, valid, model->bounds.min, model->bounds.max, coords);
                } else {
                    for (const Poly& poly : model->polys->polys) {
                        for (const Vector& vertex : poly.vertices) {
                            includePoint(boxMin, boxMax, valid, transformPointBy(vertex, coords));
                        }
                    }
                }
            } else if (mesh) {
                objects::ensureLoaded(mesh);
                Vector frameMin = mesh->meshBounds.min;
                Vector frameMax = mesh->meshBounds.max;
                const AnimSequence* sequence = mesh->findSequence(actor::AnimSequence(self));
                if (sequence && sequence->numFrames > 0 && !(actor::AnimFrame(self) < 0)) {
                    float scaledFrame = static_cast<float>((static_cast<double>(actor::AnimFrame(self)) + 1.0) * sequence->numFrames);
                    int32_t frame = static_cast<int32_t>(std::nearbyint(static_cast<double>(scaledFrame) - 0.5));
                    size_t first = static_cast<size_t>(sequence->startFrame + frame % sequence->numFrames);
                    size_t second = static_cast<size_t>(sequence->startFrame + (frame + 1) % sequence->numFrames);
                    if (first < mesh->frameBounds.size() && second < mesh->frameBounds.size()) {
                        const BoundingBox& a = mesh->frameBounds[first];
                        const BoundingBox& b = mesh->frameBounds[second];
                        frameMin = { std::min(a.min.x, b.min.x), std::min(a.min.y, b.min.y), std::min(a.min.z, b.min.z) };
                        frameMax = { std::max(a.max.x, b.max.x), std::max(a.max.y, b.max.y), std::max(a.max.z, b.max.z) };
                    }
                }
                float drawScale = actor::bParticles(self) ? 1.5f : actor::DrawScale(self);
                Vector scale = mesh->scale * drawScale;
                Vector one { 1, 1, 1 };
                Vector scaledMin = scale * (frameMin - mesh->origin) - one;
                Vector scaledMax = scale * (frameMax - mesh->origin) + one;
                Coords coords = coordsUnrotate(coordsUnrotate(unit, mesh->rotOrigin), rotation);
                coords.origin = location + actor::PrePivot(self);
                includeBox(boxMin, boxMax, valid, scaledMin, scaledMax, coordsTranspose(coords));
            } else {
                Vector extent = cylinderExtent(self) + Vector { 1, 1, 1 };
                boxMin = location - extent;
                boxMax = location + extent;
                valid = true;
            }
            *minimum = boxMin;
            *maximum = boxMax;
            vm::retBool(result, valid);
        }

        void nativeLastRendered(UObject* self, Frame& stack, void* result)
        {
            vm::finish(stack);
            float last = 0;
            uint8_t* address = self->propertyAddress(Name("LastRenderTime"));
            if (address && World::current) {
                last = std::max(world().timeSeconds - *reinterpret_cast<float*>(address), 0.0f);
            }
            vm::ret<float>(result, last);
        }

        Vector safeNormal(const Vector& v)
        {
            float squared = v.sizeSquared();
            if (squared < 1e-8f) {
                return {};
            }
            return v * static_cast<float>(1.0 / std::sqrt(static_cast<double>(squared)));
        }

        PointRegion pointRegionAt(const Vector& location)
        {
            PointRegion region;
            region.zone = world().zoneAt(location, &region.iLeaf, &region.zoneNumber);
            return region;
        }

        void parabolicMove(UObject* self, Vector& location, Vector delta, const Vector& extent, bool checkActors, PointRegion& region, CheckResult& hit)
        {
            if (std::abs(delta.x) < 0.0001f && std::abs(delta.y) < 0.0001f && std::abs(delta.z) < 0.0001f) {
                return;
            }
            hit = CheckResult {};
            hit.time = 1;
            float deltaSize = static_cast<float>(std::sqrt(static_cast<double>(delta.sizeSquared())));
            Vector direction = delta * static_cast<float>(1.0 / static_cast<double>(deltaSize));
            Vector testDelta = delta + direction * 2.0f;
            uint32_t flags = checkActors ? TRACE_AllColliding : (TRACE_Level | TRACE_LevelGeometry);
            for (const CheckResult& check : world().multiLineCheck(location, location + testDelta, extent, nullptr, flags)) {
                if (!check.actor || isOwnedBy(self, check.actor)) {
                    continue;
                }
                if (check.actor == world().levelInfo || actor::bBlockActors(check.actor)) {
                    hit = check;
                    break;
                }
            }
            if (hit.time < 1.0f) {
                float scaled = (2.0f + deltaSize) * hit.time;
                if (!(scaled > 2.0f)) {
                    delta = {};
                    hit.time = 0;
                } else {
                    delta = testDelta * hit.time - direction * 2.0f;
                    hit.time = (scaled - 2.0f) / deltaSize;
                }
            }
            location = location + delta;
            region = pointRegionAt(location);
        }

        void twoWallAdjust(const Vector& desired, Vector& delta, const Vector& hitNormal, const Vector& oldNormal, float hitTime)
        {
            if (oldNormal.dot(hitNormal) <= 0) {
                Vector direction = safeNormal(hitNormal.cross(oldNormal));
                delta = direction * (delta.dot(direction) * (1.0f - hitTime));
                if (desired.dot(delta) < 0) {
                    delta = delta * -1.0f;
                }
            } else {
                delta = (delta - hitNormal * delta.dot(hitNormal)) * (1.0f - hitTime);
                if (delta.dot(desired) <= 0) {
                    delta = {};
                }
            }
        }

        Vector bounceVelocity(const Vector& velocity, const Vector& normal, float elasticity)
        {
            return (velocity + normal * (velocity.dot(normal) * -2.0f)) * elasticity;
        }

        void nativeParabolicTrace(UObject* self, Frame& stack, void* result)
        {
            Vector* finalLocation = vm::getRef<Vector>(stack);
            Vector startVelocity = vm::getOpt<Vector>(stack, actor::Velocity(self));
            Vector startLocation = vm::getOpt<Vector>(stack, actor::Location(self));
            bool checkActors = vm::getBoolOpt(stack, actor::bCollideActors(self) != 0);
            Vector cylinder = vm::getOpt<Vector>(stack, cylinderExtent(self));
            float maxTime = vm::getOpt<float>(stack, 5.0f);
            float elasticity = vm::getOpt<float>(stack, 0.9f);
            bool bounce = vm::getBoolOpt(stack, actor::bBounce(self) != 0);
            float landingSpeed = vm::getOpt<float>(stack, 60.0f);
            float granularity = vm::getOpt<float>(stack, 0.05f);
            vm::finish(stack);
            if (!World::current) {
                *finalLocation = startLocation;
                vm::ret<float>(result, 0.0f);
                return;
            }
            if (granularity < 0.005f) {
                granularity = 0.005f;
            } else if (!(granularity < 1.0f)) {
                granularity = 1.0f;
            }
            float landingSquared = landingSpeed * landingSpeed;
            Vector position = startLocation;
            Vector velocity = startVelocity;
            CheckResult hit;
            hit.time = 1;
            int32_t steps = 0;
            bool done = false;
            PointRegion region = pointRegionAt(position);
            while (!done) {
                UObject* zone = region.zone;
                velocity = velocity + zoneinfo::ZoneGravity(zone) * granularity;
                Vector move = (velocity + zoneinfo::ZoneVelocity(zone)) * granularity;
                parabolicMove(self, position, move, cylinder, checkActors, region, hit);
                bool struck = false;
                if (region.zone && zoneinfo::bWaterZone(region.zone)) {
                    steps = 0;
                    position = startLocation;
                    done = true;
                } else if (hit.time < 1.0f) {
                    struck = true;
                    bool reflect = !(hit.normal.z > 0.7f);
                    if (!reflect) {
                        reflect = bounce && !(landingSquared > velocity.sizeSquared());
                        done = !reflect;
                    }
                    if (reflect) {
                        Vector oldNormal = hit.normal;
                        velocity = bounceVelocity(velocity, hit.normal, elasticity);
                        Vector adjusted = (move - hit.normal * move.dot(hit.normal)) * static_cast<float>(1.0 - static_cast<double>(hit.time));
                        if (!(adjusted.dot(move) < 0)) {
                            parabolicMove(self, position, adjusted, cylinder, checkActors, region, hit);
                            if (hit.time < 1.0f) {
                                velocity = bounceVelocity(velocity, hit.normal, elasticity);
                                if (hit.normal.z > 0.7f && (!bounce || landingSquared > velocity.sizeSquared())) {
                                    done = true;
                                }
                                twoWallAdjust(safeNormal(move), adjusted, hit.normal, oldNormal, hit.time);
                                parabolicMove(self, position, adjusted, cylinder, checkActors, region, hit);
                                if (!done && hit.time < 1.0f && hit.normal.z > 0.7f && (!bounce || landingSquared > adjusted.sizeSquared())) {
                                    done = true;
                                }
                            }
                        }
                    }
                }
                if (!bounce || !struck) {
                    float terminal = zoneinfo::ZoneTerminalVelocity(region.zone);
                    if (velocity.sizeSquared() > terminal * terminal) {
                        velocity = safeNormal(velocity) * terminal;
                    }
                }
                steps++;
                if (region.zoneNumber == 0 || static_cast<float>(steps) * granularity > maxTime || !(move.sizeSquared() <= 2500000.0f)) {
                    steps = 0;
                    done = true;
                    position = startLocation;
                }
            }
            *finalLocation = position;
            vm::ret<float>(result, static_cast<float>(steps) * granularity);
        }

        void nativeInStasis(UObject* self, Frame& stack, void* result)
        {
            vm::finish(stack);
            vm::retBool(result, World::current && World::current->inStasis(self));
        }

        void nativeGetPlayerPawn(UObject*, Frame& stack, void* result)
        {
            vm::finish(stack);
            vm::ret<UObject*>(result, World::current ? world().localPlayer : nullptr);
        }

        void nativeIsOverlapping(UObject* self, Frame& stack, void* result)
        {
            UObject* other = vm::get<UObject*>(stack);
            vm::finish(stack);
            vm::retBool(result, other && isOverlapping(self, other));
        }

        void nativeRandomBiasedRotation(UObject*, Frame& stack, void* result)
        {
            int32_t centralYaw = vm::get<int32_t>(stack);
            float yawDistribution = vm::get<float>(stack);
            int32_t centralPitch = vm::get<int32_t>(stack);
            float pitchDistribution = vm::get<float>(stack);
            vm::finish(stack);
            auto biased = [](int32_t center, float distribution, double negativeScale, double positiveScale) {
                float bias = static_cast<float>((static_cast<double>(std::clamp(distribution, 0.0f, 1.0f)) + 1.0) * 0.5);
                double b = static_cast<double>(bias);
                float r = randomFloat() * 2.0f - 1.0f;
                double a = std::abs(static_cast<double>(r));
                double value = a <= b ? a * (1.0 - b) / b : (a - b) * b / (1.0 - b) + (1.0 - b);
                value *= r < 0 ? negativeScale : positiveScale;
                return center + static_cast<int32_t>(value);
            };
            Rotator rotation;
            rotation.yaw = biased(centralYaw, yawDistribution, -32768.0, 32767.0);
            rotation.pitch = biased(centralPitch, pitchDistribution, -16384.0, 16383.0);
            rotation.roll = 0;
            vm::ret<Rotator>(result, rotation);
        }

        void nativeGetMeshTexture(UObject* self, Frame& stack, void* result)
        {
            int32_t index = vm::getOpt<int32_t>(stack, 0);
            vm::finish(stack);
            UObject* texture = nullptr;
            UMesh* mesh = cast<UMesh>(actor::Mesh(self));
            if (mesh) {
                const std::vector<UObject*>& textures = mesh->textures;
                bool inRange = index >= 0 && static_cast<size_t>(index) < textures.size();
                if (index >= 0 && index < 8 && actor::MultiSkins(self, index)) {
                    texture = actor::MultiSkins(self, index);
                } else if (index > 0 && inRange && textures[static_cast<size_t>(index)]) {
                    texture = textures[static_cast<size_t>(index)];
                } else if (actor::Skin(self)) {
                    texture = actor::Skin(self);
                } else if (inRange) {
                    texture = textures[static_cast<size_t>(index)];
                }
            }
            vm::ret<UObject*>(result, texture);
        }

        void nativeConsoleCommand(UObject* self, Frame& stack, void* result)
        {
            std::string command = vm::get<std::string>(stack);
            vm::finish(stack);
            vm::ret<std::string>(result, console::execute(self, command));
        }
    }

    namespace engine {
        void registerEngineNatives()
        {
            vm::registerNative(552, nativeMultiplyColorFloat);
            vm::registerNative(550, nativeMultiplyFloatColor);
            vm::registerNative(551, nativeAddColorColor);
            vm::registerNative(549, nativeSubtractColorColor);
            vm::registerNative(278, nativeSpawn);
            vm::registerNative(279, nativeDestroy);
            vm::registerNative(280, nativeSetTimer);
            vm::registerNative(277, nativeTrace);
            vm::registerNative(548, nativeFastTrace);
            vm::registerNative(267, nativeSetLocation);
            vm::registerNative(299, nativeSetRotation);
            vm::registerNative(266, nativeMove);
            vm::registerNative(3969, nativeMoveSmooth);
            vm::registerNative(262, nativeSetCollision);
            vm::registerNative(283, nativeSetCollisionSize);
            vm::registerNative(298, nativeSetBase);
            vm::registerNative(272, nativeSetOwner);
            vm::registerNative(3970, nativeSetPhysics);
            vm::registerNative(3971, nativeAutonomousPhysics);
            vm::registerNative(233, nativeError);
            vm::registerNative(304, nativeAllActors);
            vm::registerNative(305, nativeChildActors);
            vm::registerNative(306, nativeBasedActors);
            vm::registerNative(307, nativeTouchingActors);
            vm::registerNative(309, nativeTraceActors);
            vm::registerNative(310, nativeRadiusActors);
            vm::registerNative(311, nativeVisibleActors);
            vm::registerNative(312, nativeVisibleCollidingActors);
            vm::registerNative(1000, nativeTraceTexture);
            vm::registerNative(1002, nativeCycleActors);
            vm::registerNative(1003, nativeTraceVisibleActors);
            vm::registerNative(264, nativePlaySound);
            vm::registerNative("Actor", "PlayOwnedSound", nativePlayOwnedSound);
            vm::registerNative("Actor", "DemoPlaySound", nativePlayOwnedSound);
            vm::registerNative(265, nativeStopSound);
            vm::registerNative("Actor", "GetSoundDuration", nativeGetSoundDuration);
            vm::registerNative(268, nativeSetInstantSoundVolume);
            vm::registerNative(269, nativeSetInstantSpeechVolume);
            vm::registerNative(270, nativeSetInstantMusicVolume);
            vm::registerNative(512, nativeMakeNoise);
            vm::registerNative(532, nativePlayerCanSeeMe);
            vm::registerNative(547, nativeGetURLMap);
            vm::registerNative(539, nativeGetMapName);
            vm::registerNative(545, nativeGetNextSkin);
            vm::registerNative("Actor", "GetNextInt", nativeGetNextInt);
            vm::registerNative("Actor", "GetNextIntDesc", nativeGetNextIntDesc);
            vm::registerNative(724, nativeGetBoundingBox);
            vm::registerNative(723, nativeLastRendered);
            vm::registerNative(722, nativeParabolicTrace);
            vm::registerNative(721, nativeInStasis);
            vm::registerNative(720, nativeGetPlayerPawn);
            vm::registerNative(718, nativeIsOverlapping);
            vm::registerNative(717, nativeRandomBiasedRotation);
            vm::registerNative(1013, nativeGetMeshTexture);
            vm::registerNative("Actor", "ConsoleCommand", nativeConsoleCommand);
            registerAnimNatives();
            registerAINatives();
            registerPawnNatives();
            registerMiscNatives();
        }
    }
}
