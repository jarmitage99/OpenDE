#include "audio/audio.h"
#include "core/objects.h"
#include "core/vm.h"
#include "engine/actor.h"
#include "engine/ai.h"
#include "engine/latent.h"
#include "engine/path_builder.h"
#include "engine/resources.h"
#include "engine/world.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <random>

namespace dx {
    int32_t clientHearSound(UObject* hearer, UObject* source, int32_t id, UObject* sound, const Vector& location, const Vector& parameters);

    namespace {
        World& world()
        {
            return *World::current;
        }

        bool isPawnClass(UObject* a)
        {
            return a && a->isA(engineClasses().pawn);
        }

        Vector safeNormal(const Vector& v)
        {
            float squared = v.sizeSquared();
            if (squared < 1e-8f) {
                return {};
            }
            return v * (1.0f / std::sqrt(squared));
        }

        bool glider(UObject* self)
        {
            uint8_t physics = actor::Physics(self);
            return pawn::bCanGlide(self) && !pawn::bCanStrafe(self) && (physics == PHYS_Flying || physics == PHYS_Swimming);
        }

        void glideAcceleration(UObject* self)
        {
            if (glider(self)) {
                actor::Acceleration(self) = rotationVector(actor::Rotation(self)) * pawn::AccelRate(self);
            }
        }

        bool rotateToward(UObject* self, const Vector& focus)
        {
            if (actor::Physics(self) == PHYS_Spider) {
                return true;
            }
            Rotator desired = vectorRotation(focus - actor::Location(self));
            desired.yaw &= 65535;
            if (actor::Physics(self) == PHYS_Walking && !isPawnClass(pawn::MoveTarget(self))) {
                desired.pitch = 0;
            }
            actor::DesiredRotation(self) = desired;
            int32_t difference = std::abs(desired.yaw - (actor::Rotation(self).yaw & 65535));
            return difference < 2000 || difference > 63535;
        }

        bool moveToward(UObject* self, const Vector& destination)
        {
            Vector direction = destination - actor::Location(self);
            uint8_t physics = actor::Physics(self);
            if (physics == PHYS_Walking) {
                direction.z = 0;
            } else if (physics == PHYS_Falling) {
                UObject* zone = zoneOf(self);
                if (!zone || !(static_cast<double>(zoneinfo::ZoneGravity(zone->cls->defaultObject).z) * 0.9 < static_cast<double>(zoneinfo::ZoneGravity(zone).z))) {
                    return false;
                }
                direction.z = 0;
                actor::Acceleration(self) = safeNormal(direction) * pawn::AccelRate(self);
                return actor::Velocity(self).z < 0 && actor::Location(self).z + 100.0f < destination.z;
            }
            UObject* moveTarget = pawn::MoveTarget(self);
            if (moveTarget && moveTarget->isA(engineClasses().inventory)) {
                Vector offset = actor::Location(self) - actor::Location(moveTarget);
                float radius = actor::CollisionRadius(self);
                if (std::abs(offset.z) < actor::CollisionHeight(self) && offset.x * offset.x + offset.y * offset.y < radius * radius) {
                    vm::Call(moveTarget, Name("Touch")).obj(self).run();
                }
            }
            float distance = static_cast<float>(std::sqrt(static_cast<double>(direction.sizeSquared())));
            bool gliding = glider(self);
            if (direction.x * direction.x + direction.y * direction.y < 256.0f && std::abs(direction.z) < std::max(48.0f, actor::CollisionHeight(self))) {
                if (!gliding) {
                    actor::Acceleration(self) = {};
                }
                return true;
            }
            if (gliding) {
                direction = rotationVector(actor::Rotation(self));
            } else if (distance > 0) {
                direction = direction * static_cast<float>(1.0 / static_cast<double>(distance));
            }
            actor::Acceleration(self) = direction * pawn::AccelRate(self);
            if (pawn::MoveTimer(self) < 0) {
                return true;
            }
            moveTarget = pawn::MoveTarget(self);
            if (isPawnClass(moveTarget)) {
                return static_cast<double>(distance) < static_cast<double>(actor::CollisionRadius(moveTarget) + actor::CollisionRadius(self)) + static_cast<double>(pawn::MeleeRange(self)) * 0.8;
            }
            Vector velocity = actor::Velocity(self);
            float speed = static_cast<float>(std::sqrt(static_cast<double>(velocity.sizeSquared())));
            if (!gliding && speed > 100.0f) {
                Vector velocityDirection = velocity * static_cast<float>(1.0 / static_cast<double>(speed));
                Vector difference = velocityDirection - direction;
                float scale = static_cast<float>(static_cast<double>((1.0f - direction.dot(velocityDirection)) * speed) * 0.2);
                actor::Acceleration(self) = actor::Acceleration(self) - difference * scale;
            }
            if (static_cast<double>(distance) < static_cast<double>(pawn::AvgPhysicsTime(self) * speed) * 1.4) {
                if (!pawn::bReducedSpeed(self)) {
                    pawn::DesiredSpeed(self) = static_cast<float>(static_cast<double>(pawn::DesiredSpeed(self)) * 0.5);
                    pawn::bReducedSpeed(self) = 1;
                }
                if (speed > 0) {
                    pawn::DesiredSpeed(self) = std::min(pawn::DesiredSpeed(self), 200.0f / speed);
                }
                return gliding;
            }
            return false;
        }

        void setMoveTimer(UObject* self, float moveSize)
        {
            float maxSpeed = 200.0f;
            uint8_t physics = actor::Physics(self);
            if (physics == PHYS_Walking || physics == PHYS_Falling || physics == PHYS_Spider) {
                maxSpeed = pawn::GroundSpeed(self);
            } else if (physics == PHYS_Flying) {
                maxSpeed = pawn::AirSpeed(self);
            } else if (physics == PHYS_Swimming) {
                maxSpeed = pawn::WaterSpeed(self);
            }
            float speed = maxSpeed * pawn::DesiredSpeed(self);
            if (speed == 0) {
                pawn::MoveTimer(self) = 0.5f;
                return;
            }
            pawn::MoveTimer(self) = static_cast<float>(static_cast<double>(moveSize) * 1.3 / static_cast<double>(speed) + 1.0);
        }

        float desiredSpeed(UObject* self, float speed)
        {
            return std::max(0.0f, std::min(pawn::MaxDesiredSpeed(self), speed));
        }

        void alterDestination(UObject* self)
        {
            if (pawn::bAdvancedTactics(self) && actor::Physics(self) == PHYS_Walking) {
                vm::event(self, Name("AlterDestination"));
            }
        }

        UObject* liveTarget(UObject* target)
        {
            return target && !target->destroyed ? target : nullptr;
        }

        void nativeMoveTo(UObject* self, Frame& stack, void*)
        {
            Vector destination = vm::get<Vector>(stack);
            float speed = vm::getOpt<float>(stack, 1.0f);
            vm::finish(stack);
            pawn::MoveTarget(self) = nullptr;
            pawn::bReducedSpeed(self) = 0;
            pawn::DesiredSpeed(self) = desiredSpeed(self, speed);
            setMoveTimer(self, (destination - actor::Location(self)).size());
            vm::stateFrame(self).latentAction = latent::MoveTo;
            pawn::Destination(self) = destination;
            pawn::Focus(self) = destination;
            rotateToward(self, pawn::Focus(self));
            moveToward(self, pawn::Destination(self));
        }

        void pollMoveTo(UObject* self, StateFrame& frame, float)
        {
            Vector saved = pawn::Destination(self);
            alterDestination(self);
            rotateToward(self, pawn::Destination(self));
            if (moveToward(self, pawn::Destination(self))) {
                frame.latentAction = 0;
            }
            pawn::Destination(self) = saved;
        }

        void nativeMoveToward(UObject* self, Frame& stack, void*)
        {
            UObject* target = vm::get<UObject*>(stack);
            float speed = vm::getOpt<float>(stack, 1.0f);
            vm::finish(stack);
            if (!target) {
                return;
            }
            pawn::bReducedSpeed(self) = 0;
            pawn::DesiredSpeed(self) = desiredSpeed(self, speed);
            if (isPawnClass(target)) {
                pawn::MoveTimer(self) = 1.2f;
            } else {
                setMoveTimer(self, (actor::Location(target) - actor::Location(self)).size());
            }
            pawn::MoveTarget(self) = target;
            pawn::Destination(self) = actor::Location(target);
            pawn::Focus(self) = actor::Location(target);
            vm::stateFrame(self).latentAction = latent::MoveToward;
            rotateToward(self, pawn::Focus(self));
            moveToward(self, pawn::Destination(self));
        }

        void pollMoveToward(UObject* self, StateFrame& frame, float)
        {
            UObject* target = liveTarget(pawn::MoveTarget(self));
            if (!target) {
                frame.latentAction = 0;
                return;
            }
            Vector destination = actor::Location(target);
            if (actor::Physics(self) == PHYS_Flying && isPawnClass(target)) {
                destination.z = static_cast<float>(static_cast<double>(actor::CollisionHeight(target)) * 0.7 + static_cast<double>(destination.z));
            } else if (actor::Physics(self) == PHYS_Spider) {
                destination = destination - pawn::Floor(self) * actor::CollisionRadius(target);
            }
            pawn::Destination(self) = destination;
            pawn::Focus(self) = destination;
            float oldDesiredSpeed = pawn::DesiredSpeed(self);
            alterDestination(self);
            rotateToward(self, pawn::Destination(self));
            if (moveToward(self, pawn::Destination(self))) {
                frame.latentAction = 0;
            }
            target = liveTarget(pawn::MoveTarget(self));
            if (!target) {
                return;
            }
            pawn::Destination(self) = actor::Location(target);
            if (isPawnClass(target)) {
                pawn::DesiredSpeed(self) = oldDesiredSpeed;
                UObject* zone = zoneOf(target);
                if (!pawn::bCanSwim(self) && zone && zoneinfo::bWaterZone(zone)) {
                    pawn::MoveTimer(self) = -1.0f;
                }
            }
        }

        void strafeSpeed(UObject* self, float speed)
        {
            pawn::bReducedSpeed(self) = 0;
            if (pawn::bIsPlayer(self)) {
                pawn::DesiredSpeed(self) = pawn::MaxDesiredSpeed(self);
            } else {
                pawn::DesiredSpeed(self) = desiredSpeed(self, speed);
            }
        }

        void nativeStrafeTo(UObject* self, Frame& stack, void*)
        {
            Vector destination = vm::get<Vector>(stack);
            Vector focus = vm::get<Vector>(stack);
            float speed = vm::getOpt<float>(stack, 1.0f);
            vm::finish(stack);
            pawn::MoveTarget(self) = nullptr;
            strafeSpeed(self, speed);
            setMoveTimer(self, (destination - actor::Location(self)).size());
            vm::stateFrame(self).latentAction = latent::StrafeTo;
            pawn::Destination(self) = destination;
            pawn::Focus(self) = focus;
            rotateToward(self, pawn::Focus(self));
            moveToward(self, pawn::Destination(self));
        }

        void pollStrafeTo(UObject* self, StateFrame& frame, float)
        {
            Vector saved = pawn::Destination(self);
            alterDestination(self);
            rotateToward(self, pawn::Focus(self));
            if (moveToward(self, pawn::Destination(self))) {
                frame.latentAction = 0;
            }
            pawn::Destination(self) = saved;
        }

        void nativeStrafeFacing(UObject* self, Frame& stack, void*)
        {
            Vector destination = vm::get<Vector>(stack);
            UObject* target = vm::get<UObject*>(stack);
            float speed = vm::getOpt<float>(stack, 1.0f);
            vm::finish(stack);
            if (!target) {
                return;
            }
            strafeSpeed(self, speed);
            setMoveTimer(self, (destination - actor::Location(self)).size());
            pawn::Destination(self) = destination;
            pawn::FaceTarget(self) = target;
            pawn::Focus(self) = actor::Location(target);
            vm::stateFrame(self).latentAction = latent::StrafeFacing;
            rotateToward(self, pawn::Focus(self));
            moveToward(self, pawn::Destination(self));
        }

        void pollStrafeFacing(UObject* self, StateFrame& frame, float)
        {
            UObject* target = liveTarget(pawn::FaceTarget(self));
            if (!target) {
                frame.latentAction = 0;
                return;
            }
            pawn::Focus(self) = actor::Location(target);
            Vector saved = pawn::Destination(self);
            rotateToward(self, pawn::Focus(self));
            alterDestination(self);
            if (moveToward(self, pawn::Destination(self))) {
                frame.latentAction = 0;
            }
            pawn::Destination(self) = saved;
        }

        void nativeTurnTo(UObject* self, Frame& stack, void*)
        {
            Vector focus = vm::get<Vector>(stack);
            vm::finish(stack);
            pawn::MoveTarget(self) = nullptr;
            vm::stateFrame(self).latentAction = latent::TurnTo;
            pawn::Focus(self) = focus;
            glideAcceleration(self);
            rotateToward(self, pawn::Focus(self));
        }

        void pollTurnTo(UObject* self, StateFrame& frame, float)
        {
            glideAcceleration(self);
            if (rotateToward(self, pawn::Focus(self))) {
                frame.latentAction = 0;
            }
        }

        void nativeTurnToward(UObject* self, Frame& stack, void*)
        {
            UObject* target = vm::get<UObject*>(stack);
            vm::finish(stack);
            if (!target) {
                return;
            }
            pawn::FaceTarget(self) = target;
            vm::stateFrame(self).latentAction = latent::TurnToward;
            glideAcceleration(self);
            pawn::Focus(self) = actor::Location(target);
            rotateToward(self, pawn::Focus(self));
        }

        void pollTurnToward(UObject* self, StateFrame& frame, float)
        {
            UObject* target = liveTarget(pawn::FaceTarget(self));
            if (!target) {
                frame.latentAction = 0;
                return;
            }
            glideAcceleration(self);
            pawn::Focus(self) = actor::Location(target);
            if (rotateToward(self, pawn::Focus(self))) {
                frame.latentAction = 0;
            }
        }

        void nativeWaitForLanding(UObject* self, Frame& stack, void*)
        {
            vm::finish(stack);
            actor::LatentFloat(self) = 2.5f;
            if (actor::Physics(self) == PHYS_Falling) {
                vm::stateFrame(self).latentAction = latent::WaitForLanding;
            }
        }

        void pollWaitForLanding(UObject* self, StateFrame& frame, float deltaSeconds)
        {
            if (actor::Physics(self) != PHYS_Falling) {
                frame.latentAction = 0;
                return;
            }
            actor::LatentFloat(self) -= deltaSeconds;
            if (actor::LatentFloat(self) < 0) {
                vm::event(self, Name("LongFall"));
            }
        }

        UClass* engineClass(const char* path)
        {
            UClass* cls = objects::findClass(path);
            objects::flushLoads();
            return cls;
        }

        int32_t truncate(double value)
        {
            return static_cast<int32_t>(value);
        }

        double squared(const Vector& a, const Vector& b)
        {
            double x = static_cast<double>(a.x) - static_cast<double>(b.x);
            double y = static_cast<double>(a.y) - static_cast<double>(b.y);
            double z = static_cast<double>(a.z) - static_cast<double>(b.z);
            return x * x + y * y + z * z;
        }

        PointRegion pointRegion(const Vector& location)
        {
            PointRegion result;
            result.zone = world().zoneAt(location, &result.iLeaf, &result.zoneNumber);
            return result;
        }

        UObject* currentZone(UObject* self)
        {
            return pointRegion(actor::Location(self)).zone;
        }

        UObject* footZone(UObject* self)
        {
            return pointRegion(actor::Location(self) - Vector { 0, 0, actor::CollisionHeight(self) }).zone;
        }

        bool waterZone(UObject* zone)
        {
            return zone && zoneinfo::bWaterZone(zone);
        }

        bool painfulZone(UObject* zone, UObject* self)
        {
            return zone && zoneinfo::bPainZone(zone) && zoneinfo::DamageType(zone) != pawn::ReducedDamageType(self);
        }

        Vector eyePoint(UObject* self)
        {
            Vector view = actor::Location(self);
            view.z += pawn::BaseEyeHeight(self);
            return view;
        }

        bool testMove(UObject* self, const Vector& delta, CheckResult& hit)
        {
            return world().moveActor(self, delta, actor::Rotation(self), hit, true, true, false, false);
        }

        void moveBack(UObject* self, const Vector& location)
        {
            world().farMoveActor(self, location, true, true);
        }

        void wallAdjust(const Vector& desired, Vector& delta, const Vector& hitNormal, const Vector& oldNormal, float hitTime)
        {
            if (oldNormal.dot(hitNormal) <= 0) {
                Vector direction = hitNormal.cross(oldNormal).normal();
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

        bool warpMarkerReached(UObject* self, UObject* goal)
        {
            UClass* marker = engineClass("Engine.WarpZoneMarker");
            if (!goal || !marker || !goal->isA(marker)) {
                return false;
            }
            uint8_t* zone = goal->propertyAddress(Name("markedWarpZone"));
            return zone && currentZone(self) == *reinterpret_cast<UObject**>(zone);
        }

        int walkMove(UObject* self, Vector delta, CheckResult& hit, UObject* goal, float threshold, bool adjust)
        {
            Vector start = actor::Location(self);
            delta.z = 0;
            Vector gravity { 0, 0, -1 };
            UObject* zone = currentZone(self);
            if (zone && zoneinfo::ZoneGravity(zone).z > 0) {
                gravity.z = 1;
            }
            Vector down = gravity * pawn::MaxStepHeight(self);
            Vector up = down * -1.0f;
            testMove(self, delta, hit);
            if (goal && hit.actor == goal) {
                return 5;
            }
            if (hit.time < 1.0f) {
                delta = delta * (1.0f - hit.time);
                testMove(self, up, hit);
                testMove(self, delta, hit);
                if (goal && hit.actor == goal) {
                    return 5;
                }
                testMove(self, down, hit);
                if (goal && hit.actor == goal) {
                    return 5;
                }
                if (hit.time < 1.0f && hit.normal.z < 0.7f) {
                    moveBack(self, start);
                    return 0;
                }
            }
            Vector location = actor::Location(self);
            down = gravity * (pawn::MaxStepHeight(self) + 2.0f);
            testMove(self, down, hit);
            if (hit.time == 1.0f) {
                moveBack(self, adjust ? start : location);
                return -1;
            }
            if (hit.normal.z < 0.7f) {
                moveBack(self, start);
                return -1;
            }
            if ((actor::Location(self) - start).sizeSquared() < threshold * threshold) {
                if (adjust) {
                    moveBack(self, start);
                }
                return 0;
            }
            return 1;
        }

        int flyMove(UObject* self, Vector delta, UObject* goal, float threshold, bool adjust)
        {
            Vector start = actor::Location(self);
            Vector up { 0, 0, pawn::MaxStepHeight(self) };
            CheckResult hit;
            testMove(self, delta, hit);
            if (goal && hit.actor == goal) {
                return 5;
            }
            if (hit.time < 1.0f) {
                delta = delta * (1.0f - hit.time);
                testMove(self, up, hit);
                testMove(self, delta, hit);
                if (goal && hit.actor == goal) {
                    return 5;
                }
            }
            if ((actor::Location(self) - start).sizeSquared() < threshold * threshold) {
                if (adjust) {
                    moveBack(self, start);
                }
                return 0;
            }
            return 1;
        }

        void findWaterLine(UObject* self, Vector start, Vector& end)
        {
            bool water = waterZone(currentZone(self));
            while ((end - start).sizeSquared() >= 1.0f) {
                Vector middle = (start + end) * 0.5f;
                if (waterZone(pointRegion(middle).zone) != water) {
                    start = middle;
                } else {
                    end = middle;
                }
            }
        }

        int swimMove(UObject* self, Vector delta, UObject* goal, float threshold, bool adjust)
        {
            Vector start = actor::Location(self);
            Vector up { 0, 0, pawn::MaxStepHeight(self) };
            CheckResult hit;
            testMove(self, delta, hit);
            if (goal && hit.actor == goal) {
                return 5;
            }
            if (!waterZone(currentZone(self))) {
                Vector end = actor::Location(self);
                findWaterLine(self, start, end);
                if (!(end == actor::Location(self))) {
                    testMove(self, end - actor::Location(self), hit);
                }
                return 0;
            }
            if (hit.time < 1.0f) {
                delta = delta * (1.0f - hit.time);
                testMove(self, up, hit);
                testMove(self, delta, hit);
                if (goal && hit.actor == goal) {
                    return 5;
                }
            }
            if ((actor::Location(self) - start).sizeSquared() < threshold * threshold) {
                if (adjust) {
                    moveBack(self, start);
                }
                return 0;
            }
            return 1;
        }

        void suggestJumpVelocity(UObject* self, const Vector& destination, Vector& velocity)
        {
            UObject* zone = zoneOf(self);
            float gravity = zone ? zoneinfo::ZoneGravity(zone).z : -950.0f;
            if (gravity >= 0) {
                gravity = -100.0f;
            }
            float startZ = velocity.z;
            float floor = destination.z - actor::Location(self).z;
            float currentZ = 0;
            float ticks = 0;
            while (currentZ > floor || velocity.z > 0) {
                velocity.z = velocity.z + gravity * 0.05f;
                ticks += 0.05f;
                currentZ = currentZ + velocity.z * 0.05f;
            }
            if (std::abs(velocity.z) > 1.0f) {
                ticks = ticks - (currentZ - floor) / velocity.z;
            }
            velocity = destination - actor::Location(self);
            velocity.z = 0;
            if (ticks > 0) {
                float size = velocity.size();
                if (size > 0) {
                    velocity = velocity / size;
                }
                size = std::min(pawn::GroundSpeed(self), size / ticks);
                velocity = velocity * size;
            } else {
                velocity = safeNormal(velocity) * pawn::GroundSpeed(self);
            }
            velocity.z = startZ;
        }

        void jumpLanding(UObject* self, Vector velocity, Vector& landing, bool movePawn)
        {
            Vector original = actor::Location(self);
            bool landed = false;
            int ticks = 0;
            const float tick = 0.1f;
            while (!landed) {
                UObject* zone = currentZone(self);
                Vector gravity = zone ? zoneinfo::ZoneGravity(zone) : Vector { 0, 0, -950 };
                float friction = zone ? zoneinfo::ZoneFluidFriction(zone) : 0.0f;
                Vector zoneVelocity = zone ? zoneinfo::ZoneVelocity(zone) : Vector {};
                velocity = velocity * (1.0f - friction * tick) + gravity * tick;
                Vector adjusted = (velocity + zoneVelocity) * tick;
                CheckResult hit;
                testMove(self, adjusted, hit);
                if (waterZone(currentZone(self))) {
                    landed = true;
                } else if (hit.time < 1.0f) {
                    if (hit.normal.z > 0.7f) {
                        landed = true;
                    } else {
                        Vector oldNormal = hit.normal;
                        Vector delta = (adjusted - hit.normal * adjusted.dot(hit.normal)) * (1.0f - hit.time);
                        if (delta.dot(adjusted) >= 0) {
                            testMove(self, delta, hit);
                            if (hit.time < 1.0f) {
                                if (hit.normal.z > 0.7f) {
                                    landed = true;
                                }
                                wallAdjust(safeNormal(adjusted), delta, hit.normal, oldNormal, hit.time);
                                testMove(self, delta, hit);
                                if (hit.normal.z > 0.7f) {
                                    landed = true;
                                }
                            }
                        }
                    }
                }
                ticks++;
                if (pointRegion(actor::Location(self)).zoneNumber == 0 || ticks > 35 || velocity.sizeSquared() > 2500000.0f) {
                    moveBack(self, original);
                    landed = true;
                }
            }
            landing = actor::Location(self);
            if (!movePawn) {
                moveBack(self, original);
            }
        }

        int findBestJump(UObject* self, const Vector& destination, Vector velocity, Vector& landing)
        {
            Vector real = actor::Location(self);
            velocity.z = pawn::JumpZ(self);
            suggestJumpVelocity(self, destination, velocity);
            jumpLanding(self, velocity, landing, true);
            Vector before = destination - real;
            Vector after = destination - actor::Location(self);
            if (painfulZone(footZone(self), self)) {
                return 0;
            }
            if (!pawn::bCanSwim(self) && waterZone(currentZone(self))) {
                return 0;
            }
            return before.size() - after.size() > 8.0f ? 1 : 0;
        }

        int swimReachable(UObject* self, const Vector& destination, float threshold, int reachFlags, UObject* goal);

        int flyReachable(UObject* self, const Vector& destination, float threshold, int reachFlags, UObject* goal)
        {
            reachFlags |= 2;
            int success = 0;
            Vector original = actor::Location(self);
            Vector velocity = actor::Velocity(self);
            int moving = 1;
            float close = threshold * threshold;
            float moveSize = std::max(200.0f, actor::CollisionRadius(self));
            float moveSquared = moveSize * moveSize;
            int ticks = 100;
            while (moving) {
                Vector direction = destination - actor::Location(self);
                float distance = direction.sizeSquared();
                if (distance > close || std::abs(direction.z) > actor::CollisionHeight(self)) {
                    if (distance < moveSquared) {
                        moving = flyMove(self, direction, goal, 8.0f, false);
                    } else {
                        moving = flyMove(self, safeNormal(direction) * moveSize, goal, 4.1f, false);
                    }
                    if (moving == 5) {
                        moving = 0;
                        success = 1;
                    }
                    UObject* zone = currentZone(self);
                    if (moving && waterZone(zone)) {
                        moving = 0;
                        if (pawn::bCanSwim(self) && !painfulZone(zone, self)) {
                            reachFlags = swimReachable(self, destination, threshold, reachFlags, goal);
                            success = reachFlags;
                        }
                    }
                } else {
                    moving = 0;
                    success = 1;
                }
                ticks--;
                if (ticks < 0) {
                    moving = 0;
                }
            }
            if (!success && warpMarkerReached(self, goal)) {
                success = 1;
            }
            moveBack(self, original);
            actor::Velocity(self) = velocity;
            return success ? reachFlags : 0;
        }

        int swimReachable(UObject* self, const Vector& destination, float threshold, int reachFlags, UObject* goal)
        {
            reachFlags |= 4;
            int success = 0;
            Vector original = actor::Location(self);
            Vector velocity = actor::Velocity(self);
            int moving = 1;
            float close = threshold * threshold;
            float moveSize = std::max(200.0f, actor::CollisionRadius(self));
            float moveSquared = moveSize * moveSize;
            int ticks = 100;
            while (moving) {
                Vector direction = destination - actor::Location(self);
                float distance = direction.sizeSquared();
                if (distance > close || std::abs(direction.z) > actor::CollisionHeight(self)) {
                    if (distance < moveSquared) {
                        moving = swimMove(self, direction, goal, 8.0f, false);
                    } else {
                        moving = swimMove(self, safeNormal(direction) * moveSize, goal, 4.1f, false);
                    }
                    if (moving == 5) {
                        moving = 0;
                        success = 1;
                    }
                    UObject* zone = currentZone(self);
                    if (!waterZone(zone)) {
                        moving = 0;
                        if (pawn::bCanFly(self)) {
                            reachFlags = flyReachable(self, destination, threshold, reachFlags, goal);
                            success = reachFlags;
                        } else if (pawn::bCanWalk(self) && destination.z < actor::Location(self).z + 50.0f + pawn::MaxStepHeight(self)) {
                            CheckResult hit;
                            testMove(self, { 0, 0, std::max(actor::CollisionHeight(self) + pawn::MaxStepHeight(self), destination.z - actor::Location(self).z) }, hit);
                            if (hit.time == 1.0f) {
                                success = flyReachable(self, destination, threshold, reachFlags, goal);
                                reachFlags = 1;
                            }
                        }
                    } else if (painfulZone(zone, self)) {
                        moving = 0;
                        success = 0;
                    }
                } else {
                    moving = 0;
                    success = 1;
                }
                ticks--;
                if (ticks < 0) {
                    moving = 0;
                }
            }
            if (!success && warpMarkerReached(self, goal)) {
                success = 1;
            }
            moveBack(self, original);
            actor::Velocity(self) = velocity;
            return success ? reachFlags : 0;
        }

        int walkReachable(UObject* self, const Vector& destination, float threshold, int reachFlags, UObject* goal)
        {
            reachFlags |= 1;
            int success = 0;
            Vector original = actor::Location(self);
            Vector velocity = actor::Velocity(self);
            int moving = 1;
            float close = threshold * threshold;
            float moveSize = 16.0f;
            if (!objects::editorMode()) {
                moveSize = pawn::bCanJump(self) ? std::max(128.0f, actor::CollisionRadius(self)) : actor::CollisionRadius(self);
            }
            int ticks = 100;
            float moveSquared = moveSize * moveSize;
            float maxZ = actor::CollisionHeight(self);
            if (goal) {
                maxZ = std::max(actor::CollisionHeight(self), actor::CollisionHeight(goal));
            }
            CheckResult hit;
            while (moving == 1) {
                Vector direction = destination - actor::Location(self);
                float zDifference = direction.z;
                direction.z = 0;
                float distance = direction.sizeSquared();
                if (zDifference > maxZ && static_cast<double>(distance) < 0.8 * static_cast<double>(zDifference - maxZ) * static_cast<double>(zDifference - maxZ)) {
                    moving = 0;
                    continue;
                }
                if (distance > close) {
                    if (distance < moveSquared) {
                        moving = walkMove(self, direction, hit, goal, 8.0f, false);
                    } else {
                        direction = safeNormal(direction);
                        moving = walkMove(self, direction * moveSize, hit, goal, 4.1f, false);
                    }
                    if (moving != 1) {
                        if (moving == 5) {
                            moving = 0;
                            success = 1;
                        } else if (pointRegion(actor::Location(self)).zoneNumber == 0) {
                            moving = 0;
                            success = 0;
                        } else if (pawn::bCanFly(self)) {
                            moving = 0;
                            reachFlags = flyReachable(self, destination, threshold, reachFlags, goal);
                            success = reachFlags;
                        } else if (pawn::bCanJump(self)) {
                            reachFlags |= 8;
                            if (moving == -1) {
                                Vector landing;
                                direction = safeNormal(direction);
                                moving = findBestJump(self, destination, direction * pawn::GroundSpeed(self), landing);
                            }
                        } else if (moving == -1 && moveSize > pawn::MaxStepHeight(self)) {
                            moving = 1;
                            moveSize = pawn::MaxStepHeight(self);
                        }
                    }
                    if (painfulZone(footZone(self), self)) {
                        moving = 0;
                        success = 0;
                    }
                    UObject* zone = currentZone(self);
                    if (waterZone(zone)) {
                        moving = 0;
                        if (pawn::bCanSwim(self) && !painfulZone(zone, self)) {
                            reachFlags = swimReachable(self, destination, threshold, reachFlags, goal);
                            success = reachFlags;
                        }
                    }
                } else {
                    moving = 0;
                    if (std::abs(zDifference) < maxZ) {
                        success = 1;
                    } else if (hit.normal.z < 0.95f && hit.normal.z > 0.7f) {
                        double slope = std::sqrt(1.0 / (static_cast<double>(hit.normal.z) * static_cast<double>(hit.normal.z)) - 1.0);
                        if (zDifference < 0 && static_cast<double>(-zDifference) < static_cast<double>(actor::CollisionHeight(self)) + static_cast<double>(actor::CollisionRadius(self)) * slope) {
                            success = 1;
                        } else {
                            float adjustRadius = goal ? actor::CollisionRadius(goal) : 46.0f;
                            if (actor::CollisionRadius(self) < adjustRadius && static_cast<double>(zDifference) < static_cast<double>(maxZ) + static_cast<double>(adjustRadius + 15.0f - actor::CollisionRadius(self)) * slope) {
                                success = 1;
                            }
                        }
                    }
                }
                ticks--;
                if (ticks < 0) {
                    moving = 0;
                }
            }
            if (!success && warpMarkerReached(self, goal)) {
                success = 1;
            }
            moveBack(self, original);
            actor::Velocity(self) = velocity;
            return success ? reachFlags : 0;
        }

        int reachable(UObject* self, const Vector& point, float threshold, UObject* goal)
        {
            uint8_t physics = actor::Physics(self);
            if (waterZone(currentZone(self))) {
                return swimReachable(self, point, threshold, 0, goal);
            }
            if (physics == PHYS_Walking || physics == PHYS_Swimming) {
                return walkReachable(self, point, threshold, 0, goal);
            }
            if (physics == PHYS_Flying) {
                return flyReachable(self, point, threshold, 0, goal);
            }
            return 0;
        }

        int actorReachable(UObject* self, UObject* other, bool knowVisible)
        {
            if (!other) {
                return 0;
            }
            Vector direction = actor::Location(other) - actor::Location(self);
            float distance = direction.sizeSquared();
            bool otherPawn = isPawnClass(other);
            if (!otherPawn) {
                if (!objects::editorMode() && distance > 640000.0f) {
                    return 0;
                }
                if (!objects::editorMode() && painfulZone(zoneOf(other), self)) {
                    return 0;
                }
            } else if (painfulZone(reinterpret_cast<PointRegion*>(pawn::FootRegion(other))->zone, self)) {
                return 0;
            }
            if (waterZone(zoneOf(other)) && !pawn::bCanSwim(self)) {
                return 0;
            }
            if (!knowVisible) {
                CheckResult hit;
                if (world().lineCheck(hit, eyePoint(self), actor::Location(other), {}, self, TRACE_Movers | TRACE_Level) && hit.actor != other) {
                    return 0;
                }
            }
            Vector real = actor::Location(self);
            Vector point = actor::Location(other);
            if (otherPawn) {
                float threshold = actor::CollisionRadius(self) + std::min(1.5f * actor::CollisionRadius(self), pawn::MeleeRange(self)) + actor::CollisionRadius(other);
                if (distance <= threshold * threshold) {
                    return 1;
                }
                if (distance > 640000.0f) {
                    threshold = std::max(direction.size() - 800.0f, threshold);
                }
                if (world().farMoveActor(self, actor::Location(other), true, false)) {
                    point = actor::Location(self);
                    moveBack(self, real);
                }
                return reachable(self, point, threshold, other);
            }
            float threshold = 15.0f;
            UClass* trigger = engineClass("Engine.Trigger");
            if (other->isA(engineClasses().inventory) || (trigger && other->isA(trigger))) {
                threshold = actor::CollisionRadius(self) + actor::CollisionRadius(other) - 2.0f;
            }
            if (world().farMoveActor(self, actor::Location(other), true, false)) {
                point = actor::Location(self);
                moveBack(self, real);
            }
            UClass* marker = engineClass("Engine.WarpZoneMarker");
            if (actor::bBlockActors(other) || (marker && other->isA(marker))) {
                return reachable(self, point, threshold, other);
            }
            return reachable(self, point, threshold, nullptr);
        }

        int pointReachable(UObject* self, Vector point, bool knowVisible)
        {
            Vector flat = point - actor::Location(self);
            if (!objects::editorMode() && static_cast<double>(flat.x) * static_cast<double>(flat.x) + static_cast<double>(flat.y) * static_cast<double>(flat.y) > 1000000.0) {
                return 0;
            }
            UObject* zone = pointRegion(point).zone;
            if (!waterZone(currentZone(self)) && !pawn::bCanSwim(self) && waterZone(zone)) {
                return 0;
            }
            UObject* foot = reinterpret_cast<PointRegion*>(pawn::FootRegion(self))->zone;
            if (!(foot && zoneinfo::bPainZone(foot)) && painfulZone(zone, self)) {
                return 0;
            }
            if (!knowVisible && !world().fastLineCheck(point, eyePoint(self))) {
                return 0;
            }
            Vector real = actor::Location(self);
            if (world().farMoveActor(self, point, true, false)) {
                point = actor::Location(self);
                moveBack(self, real);
            }
            return reachable(self, point, 15.0f, nullptr);
        }

        int calcMoveFlags(UObject* self)
        {
            return (pawn::bCanWalk(self) ? 1 : 0) + (pawn::bCanFly(self) ? 2 : 0) + (pawn::bCanSwim(self) ? 4 : 0) + (pawn::bCanJump(self) ? 8 : 0) + (pawn::bCanOpenDoors(self) ? 16 : 0) + (pawn::bCanDoSpecial(self) ? 32 : 0) + (pawn::bIsPlayer(self) ? 64 : 0);
        }

        const ReachSpec* reachSpec(int32_t index)
        {
            const std::vector<ReachSpec>& specs = world().level->reachSpecs;
            if (index < 0 || static_cast<size_t>(index) >= specs.size()) {
                return nullptr;
            }
            return &specs[static_cast<size_t>(index)];
        }

        bool supports(const ReachSpec& spec, int32_t radius, int32_t height, int32_t moveFlags)
        {
            return spec.collisionRadius >= radius && spec.collisionHeight >= height && (spec.reachFlags & moveFlags) == spec.reachFlags;
        }

        bool doorPasses(UObject* self, const ReachSpec& spec)
        {
            CheckResult hit;
            world().lineCheck(hit, actor::Location(spec.start), actor::Location(spec.end), {}, self, TRACE_Movers | TRACE_Level);
            return !hit.actor || !isMover(hit.actor) || (pawn::bCanOpenDoors(self) && (pawn::bIsPlayer(self) || !mover::bPlayerOnly(hit.actor)));
        }

        bool canMoveTo(UObject* self, UObject* anchor, UObject* destination)
        {
            int32_t radius = truncate(static_cast<double>(actor::CollisionRadius(self)));
            int32_t height = truncate(static_cast<double>(actor::CollisionHeight(self)));
            for (int pruned = 0; pruned < 2; pruned++) {
                for (int j = 0; j < 16; j++) {
                    int32_t index = pruned ? navigationpoint::PrunedPaths(anchor, j) : navigationpoint::Paths(anchor, j);
                    if (index == -1) {
                        break;
                    }
                    const ReachSpec* spec = reachSpec(index);
                    if (spec && spec->end == destination && supports(*spec, radius, height, calcMoveFlags(self)) && doorPasses(self, *spec)) {
                        return true;
                    }
                }
            }
            return false;
        }

        struct SortedPathList {
            std::array<UObject*, 32> path {};
            std::array<int32_t, 32> dist {};
            int32_t numPoints = 0;

            void addPath(UObject* node, int32_t distance)
            {
                int32_t n = 0;
                if (numPoints > 8) {
                    if (distance > dist[static_cast<size_t>(numPoints / 2)]) {
                        n = numPoints / 2;
                        if (numPoints > 16 && distance > dist[static_cast<size_t>(n + numPoints / 4)]) {
                            n += numPoints / 4;
                        }
                    } else if (numPoints > 16 && distance > dist[static_cast<size_t>(numPoints / 4)]) {
                        n = numPoints / 4;
                    }
                }
                while (n < numPoints && distance > dist[static_cast<size_t>(n)]) {
                    n++;
                }
                if (n >= 32) {
                    return;
                }
                UObject* nextPath = path[static_cast<size_t>(n)];
                int32_t nextDist = dist[static_cast<size_t>(n)];
                path[static_cast<size_t>(n)] = node;
                dist[static_cast<size_t>(n)] = distance;
                if (numPoints < 32) {
                    numPoints++;
                }
                n++;
                while (n < numPoints) {
                    UObject* afterPath = path[static_cast<size_t>(n)];
                    int32_t afterDist = dist[static_cast<size_t>(n)];
                    path[static_cast<size_t>(n)] = nextPath;
                    dist[static_cast<size_t>(n)] = nextDist;
                    nextPath = afterPath;
                    nextDist = afterDist;
                    n++;
                }
            }

            void removePath(int32_t p)
            {
                for (int32_t n = p; n < numPoints - 1; n++) {
                    path[static_cast<size_t>(n)] = path[static_cast<size_t>(n + 1)];
                    dist[static_cast<size_t>(n)] = dist[static_cast<size_t>(n + 1)];
                }
                numPoints--;
            }
        };

        UObject* navigationList()
        {
            return world().levelInfo ? levelinfo::NavigationPointList(world().levelInfo) : nullptr;
        }

        void clearPath(UObject* self, UObject* node)
        {
            navigationpoint::visitedWeight(node) = 10000000;
            navigationpoint::nextOrdered(node) = nullptr;
            navigationpoint::prevOrdered(node) = nullptr;
            navigationpoint::bEndPoint(node) = 0;
            if (navigationpoint::bSpecialCost(node)) {
                vm::Call call(node, Name("SpecialCost"));
                call.obj(self).run();
                navigationpoint::cost(node) = call.returnValue() ? *reinterpret_cast<int32_t*>(call.returnValue()) : 0;
            } else {
                navigationpoint::cost(node) = navigationpoint::ExtraCost(node);
            }
        }

        void clearPaths(UObject* self)
        {
            for (UObject* node = navigationList(); node; node = navigationpoint::nextNavigationPoint(node)) {
                clearPath(self, node);
            }
        }

        void findVisiblePaths(SortedPathList& endPoints, UObject* self, const Vector& destination, SortedPathList& destPoints, bool clear, int& startAnchor, int& endAnchor)
        {
            UObject* moveTarget = pawn::MoveTarget(self);
            if (moveTarget && moveTarget->isA(engineClasses().navigationPoint) && std::abs(actor::Location(moveTarget).z - actor::Location(self).z) < actor::CollisionHeight(moveTarget) + actor::CollisionHeight(self)) {
                Vector offset = actor::Location(moveTarget) - actor::Location(self);
                float maxDistance = actor::CollisionRadius(self) * actor::CollisionRadius(self);
                UClass* spot = engineClass("Engine.InventorySpot");
                if (pawn::bIsPlayer(self) && spot && moveTarget->isA(spot)) {
                    maxDistance += maxDistance;
                }
                if (offset.x * offset.x + offset.y * offset.y < maxDistance) {
                    startAnchor = 1;
                    endPoints.path[0] = moveTarget;
                    endPoints.dist[0] = 0;
                    endPoints.numPoints = 1;
                }
            }
            for (UObject* node = navigationList(); node; node = navigationpoint::nextNavigationPoint(node)) {
                if (clear) {
                    clearPath(self, node);
                }
                if (!startAnchor) {
                    int32_t distance = truncate(squared(actor::Location(self), actor::Location(node)));
                    if (distance < 640000) {
                        endPoints.addPath(node, distance);
                    }
                }
                if (!endAnchor) {
                    int32_t distance = truncate(squared(destination, actor::Location(node)));
                    if (distance < 640000) {
                        destPoints.addPath(node, distance);
                    }
                }
            }
        }

        bool findEndPoint(SortedPathList& list, UObject* self, int& startAnchor)
        {
            Vector view = eyePoint(self);
            while (list.numPoints > 0) {
                UObject* node = list.path[0];
                if (world().fastLineCheck(actor::Location(node), view) && pointReachable(self, actor::Location(node), true)) {
                    list.dist[0] = truncate(std::sqrt(static_cast<double>(list.dist[0])));
                    if (list.dist[0] < std::max(48, truncate(static_cast<double>(actor::CollisionRadius(self)))) && std::abs(actor::Location(node).z - actor::Location(self).z) < actor::CollisionHeight(self)) {
                        startAnchor = 1;
                    } else {
                        navigationpoint::bEndPoint(node) = 1;
                        navigationpoint::bestPathWeight(node) = list.dist[0];
                    }
                    return true;
                }
                list.removePath(0);
            }
            return false;
        }

        bool checkAnchorPath(SortedPathList& list, UObject* self, const Vector& destination)
        {
            Vector real = actor::Location(self);
            UObject* anchor = list.path[0];
            if (squared(destination, actor::Location(anchor)) < 640000.0 && world().fastLineCheck(actor::Location(anchor), destination) && world().farMoveActor(self, actor::Location(anchor), true, false)) {
                if (pointReachable(self, destination, false)) {
                    return true;
                }
                moveBack(self, real);
            }
            list.numPoints = 1;
            return false;
        }

        void expandAnchor(SortedPathList& list, UObject* self)
        {
            UObject* anchor = list.path[0];
            navigationpoint::cost(anchor) = 1000000;
            int32_t moveFlags = calcMoveFlags(self);
            int32_t radius = truncate(static_cast<double>(actor::CollisionRadius(self)));
            int32_t height = truncate(static_cast<double>(actor::CollisionHeight(self)));
            for (int pruned = 0; pruned < 2; pruned++) {
                for (int j = 0; j < 16; j++) {
                    int32_t index = pruned ? navigationpoint::PrunedPaths(anchor, j) : navigationpoint::Paths(anchor, j);
                    if (index == -1) {
                        break;
                    }
                    const ReachSpec* spec = reachSpec(index);
                    if (spec && supports(*spec, radius, height, moveFlags) && doorPasses(self, *spec)) {
                        navigationpoint::bEndPoint(spec->end) = 1;
                        navigationpoint::bestPathWeight(spec->end) = spec->distance;
                    }
                }
            }
        }

        void findAltEndPoint(SortedPathList& list, UObject* self, UObject*& bestPath)
        {
            int32_t bestDistance = navigationpoint::visitedWeight(list.path[0]) + list.dist[0];
            SortedPathList alternatives;
            for (int32_t j = 1; j < list.numPoints; j++) {
                UObject* node = list.path[static_cast<size_t>(j)];
                int32_t distance = truncate(std::sqrt(static_cast<double>(list.dist[static_cast<size_t>(j)]))) + navigationpoint::visitedWeight(node);
                if (distance >= bestDistance || !(std::abs(actor::Location(node).z - actor::Location(self).z) < 120.0f)) {
                    continue;
                }
                Vector toNode = actor::Location(node) - actor::Location(self);
                Vector toBest = actor::Location(bestPath) - actor::Location(self);
                if (toNode.dot(toBest) < 0 || distance < std::max(truncate(static_cast<double>(bestDistance) * 0.85), bestDistance - 150)) {
                    alternatives.addPath(node, distance);
                }
            }
            Vector view = eyePoint(self);
            for (int32_t j = 0; j < alternatives.numPoints; j++) {
                UObject* node = alternatives.path[static_cast<size_t>(j)];
                bool visible = world().fastLineCheck(actor::Location(node), view);
                if (pointReachable(self, actor::Location(node), true) && visible) {
                    bestPath = node;
                    return;
                }
            }
        }

        bool breadthPathFrom(UObject* self, UObject* start, UObject*& bestPath, bool singlePath, int32_t moveFlags)
        {
            UObject* current = start;
            UObject* binTree = current;
            int32_t radius = truncate(static_cast<double>(actor::CollisionRadius(self)));
            int32_t height = truncate(static_cast<double>(actor::CollisionHeight(self)));
            int32_t p = 0;
            int32_t n = 0;
            int32_t realSplit = 1;
            while (current) {
                if (navigationpoint::bEndPoint(current)) {
                    navigationpoint::previousPath(start) = nullptr;
                    bestPath = current;
                    return true;
                }
                if (!navigationpoint::bPlayerOnly(current) || pawn::bIsPlayer(self) || current == start) {
                    for (int i = 0; i < 16; i++) {
                        int32_t index = navigationpoint::upstreamPaths(current, i);
                        if (index == -1) {
                            break;
                        }
                        const ReachSpec* spec = reachSpec(index);
                        if (!spec || !supports(*spec, radius, height, moveFlags)) {
                            continue;
                        }
                        UObject* node = spec->start;
                        int32_t newVisit = spec->distance + navigationpoint::cost(node) + navigationpoint::visitedWeight(current) + (navigationpoint::bEndPoint(node) ? navigationpoint::bestPathWeight(node) : 0);
                        if (navigationpoint::visitedWeight(node) <= newVisit) {
                            continue;
                        }
                        if (navigationpoint::prevOrdered(node)) {
                            navigationpoint::nextOrdered(navigationpoint::prevOrdered(node)) = navigationpoint::nextOrdered(node);
                            if (navigationpoint::nextOrdered(node)) {
                                navigationpoint::prevOrdered(navigationpoint::nextOrdered(node)) = navigationpoint::prevOrdered(node);
                            }
                            if (binTree == node) {
                                if (navigationpoint::visitedWeight(navigationpoint::prevOrdered(node)) > newVisit) {
                                    binTree = navigationpoint::prevOrdered(node);
                                }
                            } else if (navigationpoint::visitedWeight(node) > navigationpoint::visitedWeight(binTree) && newVisit < navigationpoint::visitedWeight(binTree)) {
                                realSplit--;
                            }
                        } else if (newVisit > navigationpoint::visitedWeight(binTree)) {
                            realSplit++;
                        } else {
                            realSplit--;
                        }
                        navigationpoint::previousPath(node) = current;
                        navigationpoint::visitedWeight(node) = newVisit;
                        UObject* next = navigationpoint::visitedWeight(binTree) < newVisit ? binTree : current;
                        int32_t listed = 0;
                        while (navigationpoint::nextOrdered(next) && navigationpoint::visitedWeight(navigationpoint::nextOrdered(next)) < newVisit) {
                            listed++;
                            if (listed > 500) {
                                return false;
                            }
                            next = navigationpoint::nextOrdered(next);
                        }
                        if (navigationpoint::nextOrdered(next) != node) {
                            if (navigationpoint::nextOrdered(next)) {
                                navigationpoint::prevOrdered(navigationpoint::nextOrdered(next)) = node;
                            }
                            navigationpoint::nextOrdered(node) = navigationpoint::nextOrdered(next);
                            navigationpoint::nextOrdered(next) = node;
                            navigationpoint::prevOrdered(node) = next;
                        }
                    }
                    realSplit++;
                    int32_t move = truncate(static_cast<double>(realSplit) * 0.5);
                    while (p < move) {
                        p++;
                        if (navigationpoint::nextOrdered(binTree)) {
                            binTree = navigationpoint::nextOrdered(binTree);
                        }
                    }
                }
                n++;
                if (singlePath && n > 4) {
                    return false;
                }
                if (n > 1000) {
                    return false;
                }
                current = navigationpoint::nextOrdered(current);
            }
            return false;
        }

        void reverseRouteFor(UObject* destination)
        {
            if (!destination) {
                return;
            }
            UObject* route = destination;
            UObject* next = nullptr;
            while (navigationpoint::previousPath(route)) {
                UObject* last = navigationpoint::previousPath(route);
                navigationpoint::previousPath(route) = next;
                next = route;
                route = last;
            }
            navigationpoint::previousPath(route) = next;
        }

        bool hasNavigation()
        {
            return World::current && world().level && navigationList() && !world().level->reachSpecs.empty();
        }

        bool findPathTo(UObject* self, const Vector& destination, bool singlePath, UObject*& bestPath, bool clear)
        {
            bestPath = nullptr;
            if (!hasNavigation()) {
                return false;
            }
            Vector real = actor::Location(self);
            SortedPathList endPoints;
            SortedPathList destPoints;
            int startAnchor = 0;
            int endAnchor = 0;
            findVisiblePaths(endPoints, self, destination, destPoints, clear, startAnchor, endAnchor);
            if (endPoints.numPoints == 0 || destPoints.numPoints == 0) {
                return false;
            }
            if (!startAnchor && !findEndPoint(endPoints, self, startAnchor)) {
                moveBack(self, real);
                return false;
            }
            if (startAnchor) {
                if (checkAnchorPath(endPoints, self, destination)) {
                    bestPath = endPoints.path[0];
                    moveBack(self, real);
                    return true;
                }
                expandAnchor(endPoints, self);
            }
            if (!endAnchor) {
                for (int32_t j = 0; j < destPoints.numPoints; j++) {
                    UObject* node = destPoints.path[static_cast<size_t>(j)];
                    if (world().fastLineCheck(destination, actor::Location(node)) && world().farMoveActor(self, actor::Location(node), true, false) && pointReachable(self, destination, true)) {
                        endAnchor = 1;
                        destPoints.path[0] = node;
                        destPoints.dist[0] = truncate(std::sqrt(static_cast<double>(destPoints.dist[static_cast<size_t>(j)])));
                        break;
                    }
                }
            }
            if (endAnchor) {
                UObject* newPath = nullptr;
                int32_t moveFlags = calcMoveFlags(self);
                navigationpoint::visitedWeight(destPoints.path[0]) = destPoints.dist[0];
                if (breadthPathFrom(self, destPoints.path[0], newPath, singlePath, moveFlags)) {
                    bestPath = newPath;
                    moveBack(self, real);
                    if (!startAnchor && !singlePath) {
                        findAltEndPoint(endPoints, self, bestPath);
                    }
                    return true;
                }
            }
            moveBack(self, real);
            return false;
        }

        bool findPathToward(UObject* self, UObject* goal, bool singlePath, UObject*& bestPath, bool clear)
        {
            bestPath = nullptr;
            if (!goal || !hasNavigation()) {
                return false;
            }
            if (actor::Physics(self) != PHYS_Flying && isPawnClass(goal) && actor::Physics(goal) == PHYS_Falling) {
                Vector destination = actor::Location(goal);
                jumpLanding(goal, actor::Velocity(goal), destination, false);
                return findPathTo(self, destination, singlePath, bestPath, clear);
            }
            Vector real = actor::Location(self);
            SortedPathList endPoints;
            SortedPathList destPoints;
            int startAnchor = 0;
            int endAnchor = 0;
            bool navigationGoal = goal->isA(engineClasses().navigationPoint);
            if (navigationGoal) {
                endAnchor = 1;
                destPoints.path[0] = goal;
                destPoints.dist[0] = 0;
                destPoints.numPoints = 1;
            }
            findVisiblePaths(endPoints, self, actor::Location(goal), destPoints, clear, startAnchor, endAnchor);
            if (endPoints.numPoints == 0 || destPoints.numPoints == 0) {
                return false;
            }
            if (!startAnchor && !findEndPoint(endPoints, self, startAnchor)) {
                moveBack(self, real);
                return false;
            }
            if (startAnchor) {
                if ((navigationGoal && canMoveTo(self, endPoints.path[0], goal)) || checkAnchorPath(endPoints, self, actor::Location(goal))) {
                    bestPath = navigationGoal ? goal : endPoints.path[0];
                    moveBack(self, real);
                    return true;
                }
                expandAnchor(endPoints, self);
            }
            if (!endAnchor) {
                for (int32_t j = 0; j < destPoints.numPoints; j++) {
                    UObject* node = destPoints.path[static_cast<size_t>(j)];
                    if (world().fastLineCheck(actor::Location(goal), actor::Location(node)) && world().farMoveActor(self, actor::Location(node), true, false) && actorReachable(self, goal, true)) {
                        endAnchor = 1;
                        destPoints.path[0] = node;
                        destPoints.dist[0] = truncate(std::sqrt(static_cast<double>(destPoints.dist[static_cast<size_t>(j)])));
                        break;
                    }
                }
                if (!endAnchor && pawn::bHunting(self)) {
                    endAnchor = 1;
                }
            }
            if (endAnchor) {
                UObject* newPath = nullptr;
                navigationpoint::visitedWeight(destPoints.path[0]) = destPoints.dist[0];
                if (breadthPathFrom(self, destPoints.path[0], newPath, singlePath, calcMoveFlags(self))) {
                    bestPath = newPath;
                    moveBack(self, real);
                    if (!startAnchor && !singlePath) {
                        findAltEndPoint(endPoints, self, bestPath);
                    }
                    return true;
                }
                moveBack(self, real);
                return false;
            }
            findVisiblePaths(endPoints, self, actor::Location(goal), destPoints, clear, startAnchor, endAnchor);
            expandAnchor(destPoints, self);
            if (!endAnchor && !findEndPoint(destPoints, self, endAnchor)) {
                moveBack(self, real);
                return false;
            }
            UObject* result = nullptr;
            navigationpoint::visitedWeight(endPoints.path[0]) = endPoints.dist[0];
            if (!breadthPathFrom(self, endPoints.path[0], result, singlePath, calcMoveFlags(self))) {
                moveBack(self, real);
                return false;
            }
            moveBack(self, real);
            if (squared(actor::Location(goal), actor::Location(result)) > squared(actor::Location(goal), actor::Location(self))) {
                return false;
            }
            UObject* last = nullptr;
            for (UObject* node = result; node; node = navigationpoint::previousPath(node)) {
                last = node;
            }
            reverseRouteFor(result);
            bestPath = last;
            UObject* next = navigationpoint::previousPath(last);
            bool anchored = startAnchor && endPoints.path[0] == last;
            if (!next) {
                if (anchored) {
                    bestPath = nullptr;
                    return false;
                }
                return true;
            }
            if (!anchored && !(std::abs(actor::Location(next).z - actor::Location(self).z) < 120.0f)) {
                return true;
            }
            bool visible = world().fastLineCheck(actor::Location(next), eyePoint(self));
            if (pointReachable(self, actor::Location(next), true) && visible) {
                bestPath = next;
            }
            moveBack(self, real);
            return true;
        }

        UFunction* specialHandler(UObject* node)
        {
            UFunction* function = node ? vm::findEvent(node, Name("SpecialHandling")) : nullptr;
            return function && (function->functionFlags & FUNC_Defined) ? function : nullptr;
        }

        UObject* specialHandling(UObject* node, UFunction* function, UObject* self)
        {
            if (!function) {
                return nullptr;
            }
            vm::Call call(node, function);
            call.obj(self).run();
            return call.returnValue() ? *reinterpret_cast<UObject**>(call.returnValue()) : nullptr;
        }

        void handleSpecial(UObject* self, UObject*& bestPath)
        {
            UObject* realPath = bestPath;
            UObject* newPath = specialHandling(bestPath, specialHandler(bestPath), self);
            bestPath = newPath;
            if (!newPath || bestPath == realPath) {
                return;
            }
            if (!pawn::bCanDoSpecial(self)) {
                bestPath = nullptr;
                return;
            }
            pawn::SpecialGoal(self) = bestPath;
            if (actorReachable(self, bestPath, false)) {
                UFunction* handler = specialHandler(bestPath);
                if (handler) {
                    UObject* returned = specialHandling(bestPath, handler, self);
                    if (!returned) {
                        bestPath = nullptr;
                    } else if (bestPath != returned) {
                        bestPath = actorReachable(self, returned, false) ? returned : nullptr;
                    }
                }
                return;
            }
            if (!findPathToward(self, bestPath, false, newPath, true) || newPath == realPath) {
                bestPath = nullptr;
                return;
            }
            pawn::SpecialGoal(self) = bestPath;
            bestPath = newPath;
        }

        UObject* finishPath(UObject* self, UObject* bestPath)
        {
            pawn::SpecialPause(self) = 0;
            pawn::bShootSpecial(self) = 0;
            if (bestPath && specialHandler(bestPath)) {
                handleSpecial(self, bestPath);
            }
            if (bestPath == pawn::SpecialGoal(self)) {
                pawn::SpecialGoal(self) = nullptr;
            }
            return bestPath;
        }

        int onNetwork(UObject* self, UObject* target)
        {
            float maxDistance = std::max(48.0f, actor::CollisionRadius(self));
            maxDistance = maxDistance * maxDistance;
            bool onPath = false;
            auto near = [&](UObject* node) {
                if (!(std::abs(actor::Location(node).z - actor::Location(self).z) < actor::CollisionHeight(self))) {
                    return false;
                }
                Vector offset = actor::Location(node) - actor::Location(self);
                return offset.x * offset.x + offset.y * offset.y < maxDistance;
            };
            UObject* moveTarget = pawn::MoveTarget(self);
            if (moveTarget && moveTarget->isA(engineClasses().navigationPoint) && near(moveTarget)) {
                onPath = true;
                if (moveTarget == target || canMoveTo(self, moveTarget, target)) {
                    return 1;
                }
            }
            for (UObject* node = navigationList(); node; node = navigationpoint::nextNavigationPoint(node)) {
                if (near(node)) {
                    onPath = true;
                    if (node == target || canMoveTo(self, node, target)) {
                        return 1;
                    }
                }
            }
            if (onPath && actor::Physics(self) != PHYS_Flying) {
                return 0;
            }
            return -1;
        }

        bool execActorReachable(UObject* self, UObject* target)
        {
            if (!target) {
                return false;
            }
            UObject* realActor = nullptr;
            if (target->isA(engineClasses().inventory) && inventory::myMarker(target)) {
                realActor = target;
                target = inventory::myMarker(target);
            }
            if (target->isA(engineClasses().navigationPoint) && !world().level->reachSpecs.empty() && actor::CollisionRadius(self) <= 115.0f) {
                int network = onNetwork(self, target);
                if (network >= 0) {
                    return network == 1;
                }
            }
            if (realActor) {
                target = realActor;
            }
            return actorReachable(self, target, false) != 0;
        }

        void nativeActorReachable(UObject* self, Frame& stack, void* result)
        {
            UObject* target = vm::get<UObject*>(stack);
            vm::finish(stack);
            vm::retBool(result, World::current && world().level && execActorReachable(self, target));
        }

        void nativePointReachable(UObject* self, Frame& stack, void* result)
        {
            Vector point = vm::get<Vector>(stack);
            vm::finish(stack);
            vm::retBool(result, World::current && world().level && pointReachable(self, point, false) != 0);
        }

        void nativeFindPathToward(UObject* self, Frame& stack, void* result)
        {
            UObject* target = vm::get<UObject*>(stack);
            bool singlePath = vm::getBoolOpt(stack, false);
            bool clear = vm::getBoolOpt(stack, true);
            vm::finish(stack);
            if (!target || !World::current) {
                vm::ret<UObject*>(result, nullptr);
                return;
            }
            UObject* found = nullptr;
            UObject* next = findPathToward(self, target, singlePath, found, clear) ? found : nullptr;
            vm::ret<UObject*>(result, finishPath(self, next));
        }

        void nativeFindPathTo(UObject* self, Frame& stack, void* result)
        {
            Vector point = vm::get<Vector>(stack);
            bool singlePath = vm::getBoolOpt(stack, false);
            bool clear = vm::getBoolOpt(stack, true);
            vm::finish(stack);
            UObject* next = nullptr;
            if (World::current) {
                UObject* found = nullptr;
                next = finishPath(self, findPathTo(self, point, singlePath, found, clear) ? found : nullptr);
            }
            vm::ret<UObject*>(result, next);
        }

        void nativeFindRandomDest(UObject* self, Frame& stack, void* result)
        {
            bool clear = vm::getBoolOpt(stack, true);
            vm::finish(stack);
            if (clear && World::current) {
                clearPaths(self);
            }
            vm::ret<UObject*>(result, nullptr);
        }

        void nativeClearPaths(UObject* self, Frame& stack, void*)
        {
            vm::finish(stack);
            if (World::current) {
                clearPaths(self);
            }
        }

        int distanceBand(float minimumSquared, float maximumSquared, float distanceSquared)
        {
            if (distanceSquared < minimumSquared) {
                return -1;
            }
            if (distanceSquared > maximumSquared) {
                return 1;
            }
            return 0;
        }

        bool directionReachable(UObject* self, const Vector& focus, int32_t yaw, int32_t pitch, float minDist, float maxDist, Vector* bestDest)
        {
            float minimumSquared = minDist * minDist;
            float maximumSquared = maxDist * maxDist;
            Vector original = actor::Location(self);
            Vector velocity = actor::Velocity(self);
            float step = actor::CollisionRadius(self);
            if (step < 5.0f) {
                step = 5.0f;
            } else if (step > 25.0f) {
                step = 25.0f;
            }
            bool result = false;
            int moving = 1;
            int ticks = 100;
            Vector best = original;
            int previous = distanceBand(minimumSquared, maximumSquared, (original - focus).sizeSquared());
            int mode = 0;
            Rotator rotation { pitch, yaw, 0 };
            uint8_t physics = actor::Physics(self);
            if (waterZone(zoneOf(self))) {
                mode = 3;
            } else if (physics == PHYS_Walking || physics == PHYS_Swimming) {
                mode = 1;
                rotation.pitch = 0;
            } else if (physics == PHYS_Flying) {
                mode = 2;
            } else {
                return false;
            }
            Vector direction = rotationVector(rotation);
            CheckResult hit;
            while (moving == 1) {
                if (mode == 1) {
                    moving = walkMove(self, direction * step, hit, nullptr, 4.1f, false);
                } else if (mode == 2) {
                    moving = flyMove(self, direction * step, nullptr, 4.1f, false);
                } else {
                    moving = swimMove(self, direction * step, nullptr, 4.1f, false);
                }
                if (moving != 1) {
                    if (pointRegion(actor::Location(self)).zoneNumber == 0) {
                        moving = 0;
                    } else if (mode == 1 && moving == -1 && step > pawn::MaxStepHeight(self)) {
                        moving = 1;
                        step = pawn::MaxStepHeight(self);
                    }
                }
                if (painfulZone(footZone(self), self)) {
                    moving = 0;
                }
                bool water = waterZone(currentZone(self));
                if (mode == 3 ? !water : water) {
                    moving = 0;
                }
                ticks--;
                if (ticks < 0) {
                    moving = 0;
                    continue;
                }
                if (moving != 1) {
                    continue;
                }
                int band = distanceBand(minimumSquared, maximumSquared, (actor::Location(self) - focus).sizeSquared());
                if (band == 0) {
                    result = true;
                    best = actor::Location(self);
                    if (previous > 0) {
                        moving = 0;
                    }
                } else if (previous != band) {
                    if (previous != 0) {
                        result = true;
                        best = actor::Location(self);
                    }
                    moving = 0;
                }
                previous = band;
            }
            moveBack(self, original);
            actor::Velocity(self) = velocity;
            if (bestDest) {
                *bestDest = best;
            }
            return result;
        }

        void nativeDirectionReachable(UObject* self, Frame& stack, void* result)
        {
            Vector focus = vm::get<Vector>(stack);
            int32_t yaw = vm::get<int32_t>(stack);
            int32_t pitch = vm::get<int32_t>(stack);
            float minDist = vm::get<float>(stack);
            float maxDist = vm::get<float>(stack);
            Vector* bestDest = vm::getRef<Vector>(stack);
            vm::finish(stack);
            vm::retBool(result, World::current && world().level && directionReachable(self, focus, yaw, pitch, minDist, maxDist, bestDest));
        }

        void nativePickRandomDestination(UObject* self, Frame& stack, void* result)
        {
            float minDist = vm::get<float>(stack);
            float maxDist = vm::get<float>(stack);
            int32_t centralYaw = vm::get<int32_t>(stack);
            float yawDistribution = vm::get<float>(stack);
            int32_t centralPitch = vm::get<int32_t>(stack);
            float pitchDistribution = vm::get<float>(stack);
            int32_t tries = vm::get<int32_t>(stack);
            float multiplier = vm::get<float>(stack);
            Vector* destination = vm::getRef<Vector>(stack);
            vm::finish(stack);
            if (!World::current || !world().level) {
                vm::retBool(result, false);
                return;
            }
            tries = std::max(tries, 1);
            if (multiplier < 0.0001f) {
                multiplier = 0.0001f;
            } else if (!(multiplier < 1.0f)) {
                multiplier = 1.0f;
            }
            Vector best = actor::Location(self);
            uint8_t physics = actor::Physics(self);
            bool usePitch = !(physics == PHYS_Walking || (physics == PHYS_Swimming && !waterZone(zoneOf(self))));
            bool found = false;
            for (; tries > 0; tries--) {
                vm::Call call(self, Name("RandomBiasedRotation"));
                call.i(centralYaw).f(yawDistribution).i(centralPitch).f(pitchDistribution).run();
                Rotator rotation = call.returnValue() ? *reinterpret_cast<Rotator*>(call.returnValue()) : Rotator {};
                int32_t pitch = usePitch ? rotation.pitch : 0;
                found = directionReachable(self, actor::Location(self), rotation.yaw, pitch, minDist / multiplier, maxDist / multiplier, &best);
                if (found && static_cast<double>(multiplier) < 1.0) {
                    float distance = static_cast<float>(std::sqrt(squared(best, actor::Location(self))) * static_cast<double>(multiplier));
                    found = directionReachable(self, actor::Location(self), rotation.yaw, pitch, distance, distance, &best);
                }
                if (found) {
                    break;
                }
            }
            *destination = best;
            vm::retBool(result, found);
        }

        void nativeLineOfSightTo(UObject* self, Frame& stack, void* result)
        {
            UObject* other = vm::get<UObject*>(stack);
            bool ignoreDistance = vm::getBoolOpt(stack, false);
            vm::finish(stack);
            vm::retBool(result, World::current && lineOfSightTo(self, other, false, ignoreDistance));
        }

        void nativeCanSee(UObject* self, Frame& stack, void* result)
        {
            UObject* other = vm::get<UObject*>(stack);
            vm::finish(stack);
            vm::retBool(result, World::current && lineOfSightTo(self, other, true, false));
        }

        void nativeEAdjustJump(UObject* self, Frame& stack, void* result)
        {
            vm::finish(stack);
            Vector velocity = actor::Velocity(self);
            if (World::current) {
                suggestJumpVelocity(self, pawn::Destination(self), velocity);
            }
            vm::ret<Vector>(result, velocity);
        }

        float appFrand()
        {
            static std::mt19937 generator(31337);
            return std::uniform_real_distribution<float>(0.0f, 1.0f)(generator);
        }

        float wallTrace(UObject* self, const Vector& start, const Vector& end, bool cylinder, Vector* normal = nullptr)
        {
            CheckResult hit;
            world().lineCheck(hit, start, end, cylinder ? cylinderExtent(self) : Vector {}, self, TRACE_Movers | TRACE_Level);
            if (normal) {
                *normal = hit.normal;
            }
            return hit.time;
        }

        bool stepJump(UObject* self, Vector direction, const Vector& out, bool towardWall)
        {
            Vector location = actor::Location(self);
            if (towardWall) {
                Vector normal;
                if (wallTrace(self, location, location + out, true, &normal) < 1.0f) {
                    direction = normal * -1.0f;
                }
            }
            actor::Velocity(self) = direction * pawn::GroundSpeed(self);
            actor::Acceleration(self) = direction * pawn::AccelRate(self);
            actor::Velocity(self).z = pawn::JumpZ(self);
            pawn::bJumpOffPawn(self) = 1;
            setPhysics(self, PHYS_Falling, nullptr);
            return true;
        }

        bool sideClear(UObject* self, const Vector& offset, const Vector& out)
        {
            Vector location = actor::Location(self);
            if (wallTrace(self, location, location + offset, true) != 1.0f) {
                return false;
            }
            return wallTrace(self, location + offset, location + offset + out, true) == 1.0f;
        }

        bool pickWallAdjust(UObject* self)
        {
            uint8_t physics = actor::Physics(self);
            if (physics == PHYS_Falling) {
                return false;
            }
            Vector location = actor::Location(self);
            Vector view = eyePoint(self);
            Vector& destination = pawn::Destination(self);
            Vector direction = destination - location;
            float zDifference = direction.z;
            float radius = actor::CollisionRadius(self);
            float height = actor::CollisionHeight(self);
            float adjustDistance = radius + 16.0f;
            if (pawn::bIsPlayer(self)) {
                adjustDistance = static_cast<float>(static_cast<double>(radius) * 0.5 + static_cast<double>(adjustDistance));
            }
            direction.z = 0;
            int checkUp = 0;
            bool swimOrFly = physics == PHYS_Swimming || physics == PHYS_Flying;
            if (zDifference < height) {
                if (direction.dot(direction) - radius * radius < 0) {
                    return false;
                }
                if (swimOrFly && direction.sizeSquared() < 4.0f * height * height) {
                    Vector up { 0, 0, height };
                    checkUp = 1;
                    if (location.z < destination.z) {
                        checkUp = -1;
                        up = up * -1.0f;
                    }
                    if (wallTrace(self, location, location + up, true) == 1.0f) {
                        Vector shortDirection = safeNormal(direction) * radius;
                        if (wallTrace(self, location + up, location + up + shortDirection, true) == 1.0f) {
                            destination = location + up;
                            return true;
                        }
                    }
                }
            }
            float distance = direction.size();
            if (distance == 0) {
                return false;
            }
            direction = direction * static_cast<float>(1.0 / static_cast<double>(distance));
            if (!world().fastLineCheck(destination, view)) {
                if (!swimOrFly) {
                    adjustDistance += radius;
                    if (zDifference > 256.0f) {
                        return false;
                    }
                } else if (zDifference > 0) {
                    Vector up { 0, 0, height };
                    if (wallTrace(self, location, location + up * 2.0f, true) == 1.0f) {
                        destination = location + up;
                        return true;
                    }
                }
            }
            Vector left { direction.y, direction.x * -1.0f, 0 };
            bool checkedRight = false;
            Vector checkLeft = left * 1.42f * radius;
            float time = wallTrace(self, view + checkLeft, destination, false);
            if (time < 1.0f) {
                checkedRight = true;
                left = left * -1.0f;
                checkLeft = checkLeft * -1.0f;
                time = wallTrace(self, view + checkLeft, destination, false);
            }
            if (time < 1.0f) {
                return false;
            }
            Vector out = direction * 14.0f;
            bool checkedStepUp = false;
            if (pawn::bIsPlayer(self) && physics == PHYS_Walking && pawn::bCanJump(self)) {
                checkedStepUp = true;
                Vector up { 0, 0, 48 };
                float upTime = wallTrace(self, location, location + up, true);
                if (static_cast<double>(upTime) > 0.5 && wallTrace(self, location + up * upTime, location + up * upTime + out, true) == 1.0f) {
                    return stepJump(self, direction, out, pawn::bIsPlayer(self) && static_cast<double>(appFrand()) < 0.5);
                }
            }
            left = left * adjustDistance;
            if (sideClear(self, left, out)) {
                destination = location + left;
                return true;
            }
            if (!checkedRight) {
                checkLeft = checkLeft * -1.0f;
                if (wallTrace(self, view + checkLeft, destination, false) < 1.0f) {
                    return false;
                }
                left = left * -1.0f;
                if (sideClear(self, left, out)) {
                    destination = location + left;
                    return true;
                }
            }
            if (physics == PHYS_Walking) {
                if (checkedStepUp || !pawn::bCanJump(self)) {
                    return false;
                }
                Vector up { 0, 0, 48 };
                if (wallTrace(self, location, location + up, true) == 1.0f && wallTrace(self, location + up, location + up + out, true) == 1.0f) {
                    return stepJump(self, direction, out, pawn::bIsPlayer(self));
                }
                return false;
            }
            Vector up { 0, 0, height };
            if (checkUp != 1 && sideClear(self, up, out)) {
                destination = location + up;
                return true;
            }
            up = up * -1.0f;
            if (checkUp != -1 && sideClear(self, up, out)) {
                destination = location + up;
                return true;
            }
            return false;
        }

        void nativePickWallAdjust(UObject* self, Frame& stack, void* result)
        {
            vm::finish(stack);
            vm::retBool(result, World::current && world().level && pickWallAdjust(self));
        }

        float stairTrace(UObject* self, const Vector& start, const Vector& end, const Vector& extent)
        {
            CheckResult hit;
            world().lineCheck(hit, start, end, extent, self, TRACE_Movers | TRACE_Level);
            return hit.time;
        }

        void nativeFindStairRotation(UObject* self, Frame& stack, void* result)
        {
            float deltaTime = vm::get<float>(stack);
            vm::finish(stack);
            int32_t& pitch = pawn::ViewRotation(self).pitch;
            if (static_cast<double>(deltaTime) > 0.33 || !World::current) {
                vm::ret<int32_t>(result, pitch);
                return;
            }
            if (pitch > 32768) {
                pitch = (pitch & 65535) - 65536;
            }
            Vector direction = rotationVector({ 0, pawn::ViewRotation(self).yaw, pawn::ViewRotation(self).roll });
            Vector eye = actor::Location(self) + Vector { 0, 0, pawn::BaseEyeHeight(self) };
            float height = actor::CollisionHeight(self) + pawn::BaseEyeHeight(self);
            Vector slice { actor::CollisionRadius(self), actor::CollisionRadius(self), 1 };
            float distance = 2.0f * height * stairTrace(self, eye, eye + direction * (2.0f * height), slice);
            int32_t stairRotation = 0;
            if (static_cast<double>(distance) > static_cast<double>(height) * 0.8) {
                Vector spot = eye + direction * static_cast<float>(static_cast<double>(distance) * 0.5);
                float down = height * 3.0f;
                float time = stairTrace(self, spot, spot - Vector { 0, 0, down }, slice);
                if (time < 1.0f) {
                    float firstDown = down * time;
                    if (static_cast<double>(firstDown) < static_cast<double>(height) * 0.7 - 6.0) {
                        spot = eye + direction * distance;
                        time = stairTrace(self, spot, spot - Vector { 0, 0, down }, slice);
                        stairRotation = std::max(0, pitch);
                        if (down * time < firstDown - 10.0f) {
                            stairRotation = 5400;
                        }
                    } else if (static_cast<double>(firstDown) > static_cast<double>(height) * 0.7 + 6.0) {
                        Vector location = actor::Location(self);
                        if (stairTrace(self, location, location + direction * static_cast<float>(static_cast<double>(distance) * 0.9), {}) == 1.0f) {
                            spot = eye + direction * distance;
                            time = stairTrace(self, spot, spot - Vector { 0, 0, down }, slice);
                            stairRotation = std::min(0, pitch);
                            if (down * time > firstDown + 10.0f) {
                                stairRotation = -5000;
                            }
                        }
                    }
                }
            }
            int32_t difference = std::abs(pitch - stairRotation);
            if (difference > 0) {
                float rate = difference < 1000 ? static_cast<float>(8000 / difference) : 8.0f;
                rate = std::min(1.0f, rate * deltaTime);
                stairRotation = static_cast<int32_t>((1.0f - rate) * static_cast<float>(pitch) + static_cast<float>(stairRotation) * rate);
            }
            vm::ret<int32_t>(result, stairRotation);
        }

        void nativeAddPawn(UObject* self, Frame& stack, void*)
        {
            vm::finish(stack);
            if (!World::current || !world().levelInfo) {
                return;
            }
            UObject* level = world().levelInfo;
            pawn::nextPawn(self) = levelinfo::PawnList(level);
            levelinfo::PawnList(level) = self;
        }

        void nativeRemovePawn(UObject* self, Frame& stack, void*)
        {
            vm::finish(stack);
            if (!World::current || !world().levelInfo) {
                return;
            }
            UObject* level = world().levelInfo;
            if (levelinfo::PawnList(level) == self) {
                levelinfo::PawnList(level) = pawn::nextPawn(self);
                return;
            }
            for (UObject* p = levelinfo::PawnList(level); p; p = pawn::nextPawn(p)) {
                if (pawn::nextPawn(p) == self) {
                    pawn::nextPawn(p) = pawn::nextPawn(self);
                    return;
                }
            }
        }

        bool considerTarget(UObject* self, UObject* target, const Vector& fireDir, const Vector& projStart, float& bestAim, float& bestDist)
        {
            Vector delta = actor::Location(target) - projStart;
            if (!(delta.dot(fireDir) > 0)) {
                return false;
            }
            float distanceSquared = delta.sizeSquared();
            if (!(distanceSquared < 4000000.0f)) {
                return false;
            }
            float distance = static_cast<float>(std::sqrt(static_cast<double>(distanceSquared)));
            float aim = delta.dot(fireDir) / distance;
            if (!(aim > bestAim) || !lineOfSightTo(self, target, false, false)) {
                return false;
            }
            bestAim = aim;
            bestDist = distance;
            return true;
        }

        bool sameTeam(UObject* self, UObject* other)
        {
            UObject* game = levelinfo::Game(world().levelInfo);
            UObject* mine = pawn::PlayerReplicationInfo(self);
            UObject* theirs = pawn::PlayerReplicationInfo(other);
            return mine && theirs && gameinfo::bTeamGame(game) && playerreplicationinfo::Team(mine) == playerreplicationinfo::Team(theirs);
        }

        void nativePickTarget(UObject* self, Frame& stack, void* result)
        {
            float* bestAim = vm::getRef<float>(stack);
            float* bestDist = vm::getRef<float>(stack);
            Vector fireDir = vm::get<Vector>(stack);
            Vector projStart = vm::get<Vector>(stack);
            vm::finish(stack);
            UObject* best = nullptr;
            if (World::current && world().levelInfo) {
                for (UObject* p = levelinfo::PawnList(world().levelInfo); p; p = pawn::nextPawn(p)) {
                    if (p == self || pawn::Health(p) <= 0 || !actor::bProjTarget(p) || !levelinfo::Game(world().levelInfo) || sameTeam(self, p)) {
                        continue;
                    }
                    if (considerTarget(self, p, fireDir, projStart, *bestAim, *bestDist)) {
                        best = p;
                    }
                }
            }
            vm::ret<UObject*>(result, best);
        }

        void nativePickAnyTarget(UObject* self, Frame& stack, void* result)
        {
            float* bestAim = vm::getRef<float>(stack);
            float* bestDist = vm::getRef<float>(stack);
            Vector fireDir = vm::get<Vector>(stack);
            Vector projStart = vm::get<Vector>(stack);
            vm::finish(stack);
            UObject* best = nullptr;
            if (World::current) {
                std::vector<UObject*> actors = world().actors;
                for (UObject* a : actors) {
                    if (!a || a->destroyed || !actor::bProjTarget(a) || isPawnClass(a)) {
                        continue;
                    }
                    if (considerTarget(self, a, fireDir, projStart, *bestAim, *bestDist)) {
                        best = a;
                    }
                }
            }
            vm::ret<UObject*>(result, best);
        }

        float breadthPathToInventory(UObject* self, UObject* start, UObject*& bestPath, int32_t moveFlags, float bestWeight, bool predictRespawns)
        {
            UObject* current = start;
            UObject* binTree = current;
            UObject* bestDestination = nullptr;
            int32_t radius = truncate(static_cast<double>(actor::CollisionRadius(self)));
            int32_t height = truncate(static_cast<double>(actor::CollisionHeight(self)));
            int32_t p = 0;
            int32_t n = 0;
            int32_t realSplit = 1;
            UClass* spot = engineClass("Engine.InventorySpot");
            while (current) {
                if (navigationpoint::bEndPoint(current)) {
                    navigationpoint::startPath(current) = current;
                    navigationpoint::previousPath(current) = nullptr;
                }
                uint8_t* marked = spot && current->isA(spot) ? current->propertyAddress(Name("markedItem")) : nullptr;
                UObject* item = marked ? *reinterpret_cast<UObject**>(marked) : nullptr;
                if (item && (vm::isProbing(item, Name("Touch")) || (predictRespawns && static_cast<double>(actor::LatentFloat(item)) < 5.0)) && inventory::MaxDesireability(item) / static_cast<float>(navigationpoint::visitedWeight(current)) > bestWeight) {
                    vm::Call call(item, Name("BotDesireability"));
                    call.obj(self).run();
                    float desire = call.returnValue() ? *reinterpret_cast<float*>(call.returnValue()) : 0.0f;
                    float weight = desire / static_cast<float>(navigationpoint::visitedWeight(current));
                    if (weight > bestWeight) {
                        bestWeight = weight;
                        bestPath = navigationpoint::startPath(current);
                        bestDestination = current;
                    }
                }
                for (int i = 0; i < 16; i++) {
                    int32_t index = navigationpoint::Paths(current, i);
                    if (index == -1) {
                        break;
                    }
                    const ReachSpec* spec = reachSpec(index);
                    if (!spec || !supports(*spec, radius, height, moveFlags)) {
                        continue;
                    }
                    UObject* node = spec->end;
                    int32_t newVisit = spec->distance + navigationpoint::cost(node) + navigationpoint::visitedWeight(current);
                    if (navigationpoint::visitedWeight(node) <= newVisit) {
                        continue;
                    }
                    navigationpoint::startPath(node) = navigationpoint::startPath(current);
                    navigationpoint::previousPath(node) = current;
                    if (navigationpoint::prevOrdered(node)) {
                        navigationpoint::nextOrdered(navigationpoint::prevOrdered(node)) = navigationpoint::nextOrdered(node);
                        if (navigationpoint::nextOrdered(node)) {
                            navigationpoint::prevOrdered(navigationpoint::nextOrdered(node)) = navigationpoint::prevOrdered(node);
                        }
                        if (binTree == node) {
                            if (navigationpoint::visitedWeight(navigationpoint::prevOrdered(node)) > newVisit) {
                                binTree = navigationpoint::prevOrdered(node);
                            }
                        } else if (navigationpoint::visitedWeight(node) > navigationpoint::visitedWeight(binTree) && newVisit < navigationpoint::visitedWeight(binTree)) {
                            realSplit--;
                        }
                    } else if (newVisit > navigationpoint::visitedWeight(binTree)) {
                        realSplit++;
                    } else {
                        realSplit--;
                    }
                    navigationpoint::visitedWeight(node) = newVisit;
                    UObject* next = navigationpoint::visitedWeight(binTree) < newVisit ? binTree : current;
                    int32_t listed = 0;
                    while (navigationpoint::nextOrdered(next) && navigationpoint::visitedWeight(navigationpoint::nextOrdered(next)) < newVisit) {
                        listed++;
                        if (listed > 500) {
                            return 0;
                        }
                        next = navigationpoint::nextOrdered(next);
                    }
                    if (navigationpoint::nextOrdered(next) != node) {
                        if (navigationpoint::nextOrdered(next)) {
                            navigationpoint::prevOrdered(navigationpoint::nextOrdered(next)) = node;
                        }
                        navigationpoint::nextOrdered(node) = navigationpoint::nextOrdered(next);
                        navigationpoint::nextOrdered(next) = node;
                        navigationpoint::prevOrdered(node) = next;
                    }
                }
                realSplit++;
                int32_t move = truncate(static_cast<double>(realSplit) * 0.5);
                while (p < move) {
                    p++;
                    if (navigationpoint::nextOrdered(binTree)) {
                        binTree = navigationpoint::nextOrdered(binTree);
                    }
                }
                n++;
                if (n > 250) {
                    if (bestWeight > 0) {
                        reverseRouteFor(bestDestination);
                        return bestWeight;
                    }
                    n = 200;
                }
                current = navigationpoint::nextOrdered(current);
            }
            reverseRouteFor(bestDestination);
            return bestWeight;
        }

        float findPathTowardBestInventory(UObject* self, UObject*& bestPath, bool clear, float minWeight, bool predictRespawns)
        {
            bestPath = nullptr;
            if (!hasNavigation()) {
                return 0;
            }
            Vector real = actor::Location(self);
            SortedPathList endPoints;
            SortedPathList destPoints;
            int startAnchor = 0;
            int endAnchor = 1;
            findVisiblePaths(endPoints, self, {}, destPoints, clear, startAnchor, endAnchor);
            if (endPoints.numPoints == 0) {
                return 0;
            }
            if (!startAnchor && !findEndPoint(endPoints, self, startAnchor)) {
                moveBack(self, real);
                return 0;
            }
            if (!startAnchor) {
                bestPath = endPoints.path[0];
                moveBack(self, real);
                return 0.00005f;
            }
            expandAnchor(endPoints, self);
            UObject* newPath = nullptr;
            int32_t moveFlags = calcMoveFlags(self);
            navigationpoint::visitedWeight(endPoints.path[0]) = std::max(10, endPoints.dist[0]);
            float weight = breadthPathToInventory(self, endPoints.path[0], newPath, moveFlags, minWeight, predictRespawns);
            moveBack(self, real);
            if (weight > minWeight) {
                bestPath = newPath;
                return weight;
            }
            return 0;
        }

        void nativeFindBestInventoryPath(UObject* self, Frame& stack, void* result)
        {
            float* weight = vm::getRef<float>(stack);
            bool predictRespawns = vm::getBool(stack);
            vm::finish(stack);
            UObject* bestPath = nullptr;
            if (World::current && world().level) {
                UObject* newPath = nullptr;
                float bestWeight = findPathTowardBestInventory(self, newPath, true, *weight, predictRespawns);
                if (bestWeight > *weight) {
                    bestPath = newPath;
                    *weight = bestWeight;
                    bestPath = finishPath(self, bestPath);
                }
            }
            vm::ret<UObject*>(result, bestPath);
        }

        std::vector<std::pair<UObject*, float>> pathnodeList(UObject* self, int32_t maximum, UObject* start, bool usePruned, bool sort)
        {
            std::vector<std::pair<UObject*, float>> list;
            int32_t moveFlags = calcMoveFlags(self);
            int32_t radius = truncate(static_cast<double>(actor::CollisionRadius(self)));
            int32_t height = truncate(static_cast<double>(actor::CollisionHeight(self)));
            Vector origin = actor::Location(self);
            UObject* anchor = nullptr;
            UClass* navigation = engineClasses().navigationPoint;
            if (start) {
                if (start->isA(navigation)) {
                    anchor = start;
                } else {
                    origin = actor::Location(start);
                }
            }
            UObject* moveTarget = pawn::MoveTarget(self);
            if (!anchor && moveTarget && moveTarget->isA(navigation) && isOverlapping(self, moveTarget)) {
                anchor = moveTarget;
            }
            if (!anchor) {
                for (UObject* node = navigationList(); node; node = navigationpoint::nextNavigationPoint(node)) {
                    if (isOverlapping(self, node)) {
                        anchor = node;
                        break;
                    }
                }
            }
            if (anchor) {
                for (int pruned = 0; pruned < (usePruned ? 2 : 1); pruned++) {
                    for (int i = 0; i < 16; i++) {
                        int32_t index = pruned ? navigationpoint::PrunedPaths(anchor, i) : navigationpoint::Paths(anchor, i);
                        if (index == -1 || static_cast<int32_t>(list.size()) >= maximum) {
                            break;
                        }
                        const ReachSpec* spec = reachSpec(index);
                        if (spec && spec->end && spec->end->isA(navigation) && supports(*spec, radius, height, moveFlags)) {
                            list.emplace_back(spec->end, static_cast<float>(spec->distance));
                        }
                    }
                }
            } else {
                for (UObject* node = navigationList(); node; node = navigationpoint::nextNavigationPoint(node)) {
                    double distanceSquared = squared(actor::Location(node), origin);
                    if (!(distanceSquared < 1000000.0)) {
                        continue;
                    }
                    float distance = static_cast<float>(std::sqrt(distanceSquared));
                    if (static_cast<int32_t>(list.size()) < maximum) {
                        list.emplace_back(node, distance);
                        continue;
                    }
                    int32_t slot = -1;
                    for (int32_t i = 0; i < static_cast<int32_t>(list.size()); i++) {
                        if (!(distance < list[static_cast<size_t>(i)].second)) {
                            continue;
                        }
                        if (slot < 0 || list[static_cast<size_t>(slot)].second > list[static_cast<size_t>(i)].second) {
                            slot = i;
                        }
                    }
                    if (slot >= 0) {
                        list[static_cast<size_t>(slot)] = { node, distance };
                    }
                }
                std::vector<std::pair<UObject*, float>> reached;
                for (const auto& entry : list) {
                    if (actorReachable(self, entry.first, false)) {
                        reached.push_back(entry);
                    }
                }
                list = std::move(reached);
            }
            if (sort && list.size() > 1) {
                std::sort(list.begin(), list.end(), [](const std::pair<UObject*, float>& a, const std::pair<UObject*, float>& b) {
                    return a.second < b.second;
                });
            }
            return list;
        }

        void nativeReachablePathnodes(UObject* self, Frame& stack, void*)
        {
            vm::get<UObject*>(stack);
            UObject** out = vm::getRef<UObject*>(stack);
            UObject* from = vm::get<UObject*>(stack);
            float* distanceOut = vm::getRef<float>(stack);
            bool usePruned = vm::getBoolOpt(stack, false);
            vm::finish(stack);
            std::vector<std::pair<UObject*, float>> list;
            if (World::current && world().level) {
                list = pathnodeList(self, 32, from, usePruned, true);
            }
            size_t index = 0;
            vm::iterate(stack, [&]() {
                *out = nullptr;
                if (index < list.size()) {
                    *out = list[index].first;
                    *distanceOut = list[index].second;
                    index++;
                    return true;
                }
                return false;
            });
        }

        void relaxPathnodes(UObject* self, UObject* node, int32_t radius, int32_t height, int32_t moveFlags)
        {
            for (int i = 0; i < 16; i++) {
                int32_t index = navigationpoint::Paths(node, i);
                if (index == -1) {
                    break;
                }
                const ReachSpec* spec = reachSpec(index);
                if (!spec || !spec->end || !spec->end->isA(engineClasses().navigationPoint) || !supports(*spec, radius, height, moveFlags)) {
                    continue;
                }
                float weight = static_cast<float>(navigationpoint::visitedWeight(node) + spec->distance);
                if (static_cast<float>(navigationpoint::visitedWeight(spec->end)) > weight) {
                    navigationpoint::visitedWeight(spec->end) = static_cast<int32_t>(weight);
                    relaxPathnodes(self, spec->end, radius, height, moveFlags);
                }
            }
        }

        void nativeComputePathnodeDistances(UObject* self, Frame& stack, void*)
        {
            UObject* start = vm::getOpt<UObject*>(stack, nullptr);
            vm::finish(stack);
            if (!World::current || !world().level) {
                return;
            }
            int32_t moveFlags = calcMoveFlags(self);
            int32_t radius = truncate(static_cast<double>(actor::CollisionRadius(self)));
            int32_t height = truncate(static_cast<double>(actor::CollisionHeight(self)));
            clearPaths(self);
            for (const auto& [node, distance] : pathnodeList(self, 32, start, false, false)) {
                if (static_cast<float>(navigationpoint::visitedWeight(node)) > distance) {
                    navigationpoint::visitedWeight(node) = static_cast<int32_t>(distance);
                    relaxPathnodes(self, node, radius, height, moveFlags);
                }
            }
        }

        void nativeStopWaiting(UObject* self, Frame& stack, void*)
        {
            vm::finish(stack);
            if (self->stateFrame && self->stateFrame->latentAction == latent::Sleep) {
                actor::LatentFloat(self) = -1.0f;
            }
        }

        void nativeClientHearSound(UObject* self, Frame& stack, void*)
        {
            UObject* source = vm::get<UObject*>(stack);
            int32_t id = vm::get<int32_t>(stack);
            UObject* sound = vm::get<UObject*>(stack);
            Vector location = vm::get<Vector>(stack);
            Vector parameters = vm::get<Vector>(stack);
            vm::finish(stack);
            clientHearSound(self, source, id, sound, location, parameters);
        }
    }

    int pawnPointReachable(UObject* pawn, Vector point, bool knowVisible)
    {
        return pointReachable(pawn, point, knowVisible);
    }

    bool pawnFindPathToward(UObject* pawn, UObject* goal, bool singlePath, UObject*& bestPath, bool clear)
    {
        return findPathToward(pawn, goal, singlePath, bestPath, clear);
    }

    int pawnWalkMove(UObject* pawn, Vector delta, CheckResult& hit, UObject* goal, float threshold, bool adjust)
    {
        return walkMove(pawn, delta, hit, goal, threshold, adjust);
    }

    int pawnWalkReachable(UObject* pawn, const Vector& destination, float threshold, int reachFlags, UObject* goal)
    {
        return walkReachable(pawn, destination, threshold, reachFlags, goal);
    }

    int32_t clientHearSound(UObject* hearer, UObject* source, int32_t id, UObject* sound, const Vector& location, const Vector& parameters)
    {
        USound* data = cast<USound>(sound);
        if (!data || !isPlayerPawn(hearer) || !playerpawn::Player(hearer) || !playerpawn::Player(hearer)->isA(engineClasses().viewport)) {
            return 0;
        }
        if (source && source->destroyed) {
            source = nullptr;
        }
        float volume = static_cast<float>(static_cast<double>(parameters.x) * 0.01);
        float radius = parameters.y != 0 ? parameters.y : 1600.0f;
        float pitch = static_cast<float>(static_cast<double>(parameters.z) * 0.01);
        objects::ensureLoaded(data);
        return audio::playSound(data, source, (id >> 1) & 7, location, volume, radius, pitch, true, (id & 1) != 0);
    }

    void registerPawnNatives()
    {
        vm::registerNative(500, nativeMoveTo);
        vm::registerNative(502, nativeMoveToward);
        vm::registerNative(504, nativeStrafeTo);
        vm::registerNative(506, nativeStrafeFacing);
        vm::registerNative(508, nativeTurnTo);
        vm::registerNative(510, nativeTurnToward);
        vm::registerNative(527, nativeWaitForLanding);
        vm::registerNative(520, nativeActorReachable);
        vm::registerNative(521, nativePointReachable);
        vm::registerNative(517, nativeFindPathToward);
        vm::registerNative(518, nativeFindPathTo);
        vm::registerNative(525, nativeFindRandomDest);
        vm::registerNative(522, nativeClearPaths);
        vm::registerNative(514, nativeLineOfSightTo);
        vm::registerNative(533, nativeCanSee);
        vm::registerNative(523, nativeEAdjustJump);
        vm::registerNative(526, nativePickWallAdjust);
        vm::registerNative(524, nativeFindStairRotation);
        vm::registerNative(529, nativeAddPawn);
        vm::registerNative(530, nativeRemovePawn);
        vm::registerNative(531, nativePickTarget);
        vm::registerNative(534, nativePickAnyTarget);
        vm::registerNative(540, nativeFindBestInventoryPath);
        vm::registerNative(708, nativeDirectionReachable);
        vm::registerNative(709, nativePickRandomDestination);
        vm::registerNative(1004, nativeReachablePathnodes);
        vm::registerNative(1020, nativeComputePathnodeDistances);
        vm::registerNative("Pawn", "StopWaiting", nativeStopWaiting);
        vm::registerNative("Pawn", "ClientHearSound", nativeClientHearSound);
        latent::registerPoll(latent::MoveTo, pollMoveTo);
        latent::registerPoll(latent::MoveToward, pollMoveToward);
        latent::registerPoll(latent::StrafeTo, pollStrafeTo);
        latent::registerPoll(latent::StrafeFacing, pollStrafeFacing);
        latent::registerPoll(latent::TurnTo, pollTurnTo);
        latent::registerPoll(latent::TurnToward, pollTurnToward);
        latent::registerPoll(latent::WaitForLanding, pollWaitForLanding);
    }
}
