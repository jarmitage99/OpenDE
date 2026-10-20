#include "core/objects.h"
#include "core/vm.h"
#include "engine/actor.h"
#include "engine/world.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace dx {
    namespace {
        float frand()
        {
            static std::mt19937 generator(12345);
            return std::uniform_real_distribution<float>(0.0f, 1.0f)(generator);
        }

        World& world()
        {
            return *World::current;
        }

        bool move(UObject* a, const Vector& delta, CheckResult& hit)
        {
            return world().moveActor(a, delta, actor::Rotation(a), hit, false, false, false, false);
        }

        bool ladderAhead(UObject* a, const Vector& accelDir)
        {
            if (!isPlayerPawn(a) || accelDir.zero()) {
                return false;
            }
            Vector flat { accelDir.x, accelDir.y, 0 };
            if (flat.sizeSquared() < 0.01f) {
                return false;
            }
            flat = flat.normal();
            float reach = actor::CollisionRadius(a) + 16.0f;
            for (float offset : { 0.0f, 4.0f - actor::CollisionHeight(a) }) {
                Vector location = actor::Location(a) + Vector { 0, 0, offset };
                CheckResult hit;
                world().lineCheck(hit, location, location + flat * reach, {}, a, TRACE_World);
                if (hit.time >= 1.0f || !hit.material || std::abs(hit.normal.z) > 0.3f || flat.dot(hit.normal) > -0.3f) {
                    continue;
                }
                UObject* group = hit.material->outer;
                if (group && group->outer && group->name == Name("Ladder")) {
                    return true;
                }
            }
            return false;
        }

        void climbLadder(UObject* a, float tick, float speed)
        {
            int32_t pitch = pawn::ViewRotation(a).pitch & 65535;
            float direction = (pitch > 32768 && pitch < 61000) ? -1.0f : 1.0f;
            CheckResult hit;
            move(a, { 0, 0, direction * speed * tick }, hit);
        }

        Vector zoneGravity(UObject* a)
        {
            UObject* zone = zoneOf(a);
            return zone ? zoneinfo::ZoneGravity(zone) : Vector { 0, 0, -950 };
        }

        Vector zoneVelocity(UObject* a)
        {
            UObject* zone = zoneOf(a);
            return zone ? zoneinfo::ZoneVelocity(zone) : Vector {};
        }

        float zoneGroundFriction(UObject* a)
        {
            UObject* zone = zoneOf(a);
            return zone ? zoneinfo::ZoneGroundFriction(zone) : 8.0f;
        }

        float zoneFluidFriction(UObject* a)
        {
            UObject* zone = zoneOf(a);
            return zone ? zoneinfo::ZoneFluidFriction(zone) : 1.2f;
        }

        float zoneTerminalVelocity(UObject* a)
        {
            UObject* zone = zoneOf(a);
            return zone ? zoneinfo::ZoneTerminalVelocity(zone) : 2500.0f;
        }

        bool waterZone(UObject* zone)
        {
            return zone && zoneinfo::bWaterZone(zone);
        }

        bool probing(UObject* a, const char* eventName)
        {
            return vm::findEvent(a, Name(eventName)) != nullptr;
        }

        void twoWallAdjust(const Vector& desired, Vector& delta, const Vector& hitNormal, const Vector& oldNormal, float hitTime)
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

        int32_t fixedTurn(UObject* a, int32_t current, int32_t desired, int32_t deltaRate)
        {
            if (deltaRate == 0) {
                return current & 65535;
            }
            int32_t result = current & 65535;
            current = result;
            desired = desired & 65535;
            if (actor::bFixedRotationDir(a)) {
                if (actor::bRotateToDesired(a)) {
                    if (deltaRate > 0) {
                        if (current > desired) {
                            desired += 65536;
                        }
                        result += std::min(deltaRate, desired - current);
                    } else {
                        if (current < desired) {
                            current += 65536;
                        }
                        result += std::max(deltaRate, desired - current);
                    }
                } else {
                    result += deltaRate;
                }
            } else if (actor::bRotateToDesired(a)) {
                int32_t rate = std::abs(deltaRate);
                if (current > desired) {
                    if (current - desired < 32768) {
                        result -= std::min(current - desired, rate);
                    } else {
                        result += std::min(desired + 65536 - current, rate);
                    }
                } else {
                    if (desired - current < 32768) {
                        result += std::min(desired - current, rate);
                    } else {
                        result -= std::min(current + 65536 - desired, rate);
                    }
                }
            }
            return result & 65535;
        }

        void actorRotation(UObject* a, float deltaTime)
        {
            bool toDesired = actor::bRotateToDesired(a) != 0;
            if ((!toDesired && !actor::bFixedRotationDir(a)) || (toDesired && actor::Rotation(a) == actor::DesiredRotation(a))) {
                return;
            }
            Rotator rotation = actor::Rotation(a);
            Rotator rate = actor::RotationRate(a);
            Rotator desired = actor::DesiredRotation(a);
            int32_t yaw = static_cast<int32_t>(static_cast<float>(rate.yaw) * deltaTime);
            int32_t pitch = static_cast<int32_t>(static_cast<float>(rate.pitch) * deltaTime);
            int32_t roll = static_cast<int32_t>(static_cast<float>(rate.roll) * deltaTime);
            if (yaw != 0 && (!toDesired || desired.yaw != rotation.yaw)) {
                rotation.yaw = fixedTurn(a, rotation.yaw, desired.yaw, yaw);
            }
            if (pitch != 0 && (!toDesired || desired.pitch != rotation.pitch)) {
                rotation.pitch = fixedTurn(a, rotation.pitch, desired.pitch, pitch);
            }
            if (roll != 0 && (!toDesired || desired.roll != rotation.roll)) {
                rotation.roll = fixedTurn(a, rotation.roll, desired.roll, roll);
            }
            if (!(rotation == actor::Rotation(a))) {
                CheckResult hit;
                world().moveActor(a, {}, rotation, hit, false, false, false, false);
            }
            if (toDesired && actor::Rotation(a) == actor::DesiredRotation(a)) {
                vm::event(a, Name("EndedRotation"));
            }
        }

        void pawnRotation(UObject* a, float deltaTime)
        {
            Rotator rotation = actor::Rotation(a);
            Rotator rate = actor::RotationRate(a);
            if (!isPlayerPawn(a)) {
                int32_t deltaYaw = static_cast<int32_t>(static_cast<float>(rate.yaw) * deltaTime);
                actor::bRotateToDesired(a) = 1;
                actor::bFixedRotationDir(a) = 0;
                Rotator desired = actor::DesiredRotation(a);
                if (desired.yaw != rotation.yaw) {
                    rotation.yaw = fixedTurn(a, rotation.yaw, desired.yaw, deltaYaw);
                }
                if (desired.pitch != rotation.pitch) {
                    int32_t deltaPitch = static_cast<int32_t>(deltaTime * 32768.0f);
                    uint8_t physics = actor::Physics(a);
                    int32_t target = (physics == PHYS_Walking || physics == PHYS_Falling) ? 0 : desired.pitch;
                    rotation.pitch = fixedTurn(a, rotation.pitch, target, deltaPitch);
                    if (rotation.pitch < 32768) {
                        if (rotation.pitch > rate.pitch) {
                            rotation.pitch = rate.pitch;
                        }
                    } else if (rotation.pitch < 65536 - rate.pitch) {
                        rotation.pitch = 65536 - rate.pitch;
                    }
                }
            }
            rotation.roll = 0;
            if (!(rotation == actor::Rotation(a))) {
                CheckResult hit;
                world().moveActor(a, {}, rotation, hit, false, false, false, false);
            }
        }

        void calcVelocity(UObject* a, const Vector& accelDir, float deltaTime, float maxSpeed, float friction, bool fluid, bool brake, bool buoyant)
        {
            float effectiveFriction = std::max(fluid ? 1.0f : 0.0f, friction);
            bool walkingPlayer = isPlayerPawn(a) && pawn::bIsWalking(a);
            Vector& velocity = actor::Velocity(a);
            Vector& acceleration = actor::Acceleration(a);
            float accelRate = pawn::AccelRate(a);
            if (brake && acceleration.zero()) {
                Vector old = velocity;
                velocity = velocity - velocity * (2 * deltaTime * effectiveFriction);
                if (old.dot(velocity) < 0) {
                    velocity = {};
                }
            } else {
                float speed = velocity.size();
                if (walkingPlayer) {
                    if (acceleration.sizeSquared() > 0.09f * accelRate * accelRate) {
                        acceleration = accelDir * (accelRate * 0.3f);
                    }
                } else if (acceleration.sizeSquared() > accelRate * accelRate) {
                    acceleration = accelDir * accelRate;
                }
                velocity = velocity - (velocity - accelDir * speed) * (deltaTime * effectiveFriction);
            }
            velocity = velocity * (1 - (fluid ? friction * deltaTime : 0.0f)) + acceleration * deltaTime;
            if (!isPlayerPawn(a)) {
                maxSpeed *= pawn::DesiredSpeed(a);
            }
            if (buoyant) {
                float mass = std::max(actor::Mass(a), 1.0f);
                velocity = velocity + zoneGravity(a) * (deltaTime * (1.0f - actor::Buoyancy(a) / mass));
            }
            if (walkingPlayer && velocity.sizeSquared() > 0.09f * maxSpeed * maxSpeed) {
                float speed = velocity.size();
                velocity = velocity / speed;
                velocity = velocity * std::max(0.3f * maxSpeed, speed * (1 - deltaTime * 2 * effectiveFriction));
            } else if (velocity.sizeSquared() > maxSpeed * maxSpeed) {
                velocity = velocity.normal() * maxSpeed;
            }
        }

        void physFalling(UObject* a, float deltaTime, int iterations);
        void physFlying(UObject* a, float deltaTime, int iterations);
        void physSwimming(UObject* a, float deltaTime, int iterations);

        void stepUp(UObject* a, const Vector& gravDir, const Vector& desiredDir, Vector delta, CheckResult& hit)
        {
            Vector down = gravDir * pawn::MaxStepHeight(a);
            Vector up = down * -1.0f;
            move(a, up, hit);
            move(a, delta, hit);
            if (hit.time < 1.0f) {
                UObject* hitActor = hit.actor;
                bool pushable = hitActor && isPlayerPawn(a) && hitActor->isA(engineClasses().decoration) && decoration::bPushable(hitActor) && hit.normal.dot(desiredDir) < -0.9f;
                if (pushable) {
                    actor::bJustTeleported(a) = 1;
                    actor::Velocity(a) = actor::Velocity(a) * (actor::Mass(a) / (actor::Mass(a) + actor::Mass(hitActor)));
                    processHitWall(a, hit.normal, hitActor);
                    if (actor::Physics(a) == PHYS_Falling) {
                        return;
                    }
                } else if (std::abs(hit.normal.z) < 0.2f && hit.time * delta.sizeSquared() > 144.0f) {
                    stepUp(a, gravDir, desiredDir, delta * (1 - hit.time), hit);
                    if (actor::Physics(a) == PHYS_Falling) {
                        return;
                    }
                } else {
                    processHitWall(a, hit.normal, hitActor);
                    Vector original = delta;
                    Vector oldNormal = hit.normal;
                    delta = (delta - hit.normal * delta.dot(hit.normal)) * (1.0f - hit.time);
                    if (delta.dot(original) >= 0) {
                        move(a, delta, hit);
                        if (hit.time < 1.0f) {
                            processHitWall(a, hit.normal, hit.actor);
                            if (actor::Physics(a) == PHYS_Falling) {
                                return;
                            }
                            twoWallAdjust(desiredDir, delta, hit.normal, oldNormal, hit.time);
                            move(a, delta, hit);
                        }
                    }
                }
            }
            move(a, down, hit);
            if (hit.time < 1.0f && hit.normal.z < 0.5f) {
                Vector slide = (down - hit.normal * down.dot(hit.normal)) * (1.0f - hit.time);
                if (slide.dot(down) >= 0) {
                    move(a, slide, hit);
                }
            }
        }

        void physWalking(UObject* a, float deltaTime, int iterations)
        {
            Vector& velocity = actor::Velocity(a);
            Vector& acceleration = actor::Acceleration(a);
            velocity.z = 0;
            acceleration.z = 0;
            Vector accelDir = acceleration.zero() ? Vector {} : acceleration.normal();
            calcVelocity(a, accelDir, deltaTime, pawn::GroundSpeed(a), zoneGroundFriction(a), false, true, false);
            Vector desiredMove = actor::Velocity(a);
            Vector zoneVel = zoneVelocity(a);
            if (isPlayerPawn(a) || zoneVel.sizeSquared() > 90000.0f) {
                desiredMove = desiredMove + zoneVel;
            }
            desiredMove.z = 0;
            Vector gravDir { 0, 0, zoneGravity(a).z > 0 ? 1.0f : -1.0f };
            float maxStep = pawn::MaxStepHeight(a);
            Vector down = gravDir * (maxStep + 2.0f);
            CheckResult hit;
            actor::OldLocation(a) = actor::Location(a);
            actor::bJustTeleported(a) = 0;
            bool checkedFall = false;
            bool mustJump = false;
            float remaining = deltaTime;
            float radius = actor::CollisionRadius(a);
            float height = actor::CollisionHeight(a);
            Vector extent = cylinderExtent(a);
            while (remaining > 0 && iterations < 8) {
                iterations++;
                float tick;
                if (remaining > 0.05f && (isPlayerPawn(a) || desiredMove.sizeSquared() * remaining * remaining > 400.0f)) {
                    tick = std::min(0.05f, remaining * 0.5f);
                } else {
                    tick = remaining;
                }
                remaining -= tick;
                Vector delta = desiredMove * tick;
                Vector subLocation = actor::Location(a);
                Vector subMove = delta;
                bool zeroMove = delta.sizeSquared() < 1e-6f;
                Vector ladderDir = accelDir.zero() ? rotationAxes(actor::Rotation(a)).xAxis : accelDir;
                if (ladderAhead(a, ladderDir)) {
                    if (zeroMove || accelDir.zero()) {
                        remaining = 0;
                        continue;
                    }
                    climbLadder(a, tick, std::max(desiredMove.size(), pawn::GroundSpeed(a) * 0.5f) * 0.5f);
                    move(a, delta, hit);
                    if (a->destroyed) {
                        return;
                    }
                    continue;
                }
                if (zeroMove) {
                    remaining = 0;
                    pawn::bHitSlopedWall(a) = 0;
                } else {
                    Vector forwardCheck = accelDir * radius;
                    if (!pawn::bAvoidLedges(a)) {
                        forwardCheck = forwardCheck * 0.5f;
                    }
                    bool cautious = (!isPlayerPawn(a) || pawn::bIsWalking(a)) && !pawn::bCanFly(a);
                    if (cautious) {
                        Vector location = actor::Location(a);
                        Vector destination = location + delta + forwardCheck;
                        world().lineCheck(hit, location, destination, {}, a, TRACE_World | TRACE_Blocking | TRACE_Pawns);
                        if (hit.time >= 1.0f) {
                            float desiredDist = delta.size();
                            float testDown = std::max(4.0f + maxStep + height, 4.0f + height + radius + desiredDist);
                            world().lineCheck(hit, destination, destination + gravDir * testDown, {}, a, TRACE_World | TRACE_Blocking);
                            float maxRadius = std::min(14.0f, 0.5f * radius);
                            bool deep = hit.time >= 1.0f || (hit.normal.z > 0.7f && hit.time * testDown > height + maxStep + 4.0f && hit.time * testDown > height + 4.0f + std::sqrt(std::max(0.0f, 1 - hit.normal.z * hit.normal.z)) * (radius + desiredDist) / hit.normal.z);
                            if (deep) {
                                world().lineCheck(hit, destination, destination + gravDir * (maxStep + 4.0f), { maxRadius, maxRadius, height }, a, TRACE_World | TRACE_Blocking);
                            }
                            if (hit.time >= 1.0f) {
                                Vector desiredDir = delta / desiredDist;
                                Vector ledgeDown = gravDir * (height + 6.0f);
                                destination = location + accelDir * desiredDist + forwardCheck;
                                world().lineCheck(hit, destination + ledgeDown, location + ledgeDown - accelDir * (2 * radius), {}, a, TRACE_World | TRACE_Blocking);
                                ledgeDown = gravDir * (maxStep + 6.0f);
                                Vector ledgeDir;
                                bool moveForward = false;
                                bool goodMove = false;
                                float desiredSpeed = pawn::DesiredSpeed(a);
                                float groundSpeed = pawn::GroundSpeed(a);
                                if (hit.time < 1.0f) {
                                    if (pawn::bAvoidLedges(a)) {
                                        ledgeDir = hit.normal;
                                        ledgeDir.z = 0;
                                        delta = ledgeDir * (-desiredSpeed * groundSpeed * tick);
                                        pawn::MoveTimer(a) -= 0.25f;
                                    } else {
                                        ledgeDir = Vector { hit.normal.y, -hit.normal.x, 0 }.normal();
                                        if (ledgeDir.dot(accelDir) < 0) {
                                            ledgeDir = ledgeDir * -1.0f;
                                        }
                                        float dp = ledgeDir.dot(accelDir);
                                        moveForward = dp < 0.5f || (pawn::bCanJump(a) && dp < 0.7f);
                                        if (dp < 0.7f) {
                                            delta = ledgeDir * (std::min(0.8f, desiredSpeed) * groundSpeed * tick);
                                        } else {
                                            delta = ledgeDir * (desiredSpeed * groundSpeed * tick);
                                        }
                                    }
                                } else {
                                    ledgeDir = { desiredDir.y, -desiredDir.x, 0 };
                                    moveForward = true;
                                    delta = ledgeDir * (std::min(0.8f, desiredSpeed) * groundSpeed * tick);
                                    destination = location + delta;
                                    world().lineCheck(hit, location, destination, extent, a, TRACE_World | TRACE_Blocking);
                                    if (hit.time >= 1.0f) {
                                        world().lineCheck(hit, destination, destination + ledgeDown, extent, a, TRACE_World | TRACE_Blocking);
                                        if (hit.time >= 1.0f) {
                                            delta = delta * -1.0f;
                                        } else {
                                            goodMove = true;
                                        }
                                    } else {
                                        goodMove = true;
                                    }
                                }
                                if (isPlayerPawn(a)) {
                                    moveForward = false;
                                    if (!goodMove) {
                                        destination = location + delta + forwardCheck;
                                        world().lineCheck(hit, location, destination, extent, a, TRACE_World | TRACE_Blocking);
                                        if (hit.time >= 1.0f) {
                                            world().lineCheck(hit, destination, destination + ledgeDown, { maxRadius, maxRadius, height }, a, TRACE_World | TRACE_Blocking);
                                        }
                                        if (hit.time >= 1.0f) {
                                            actor::Acceleration(a) = {};
                                            delta = {};
                                        }
                                    }
                                }
                                if (pawn::bCanJump(a) && moveForward) {
                                    if (!probing(a, "MayFall")) {
                                        delta = accelDir * desiredDist;
                                    } else if (!checkedFall) {
                                        checkedFall = true;
                                        moveForward = false;
                                        vm::event(a, Name("MayFall"));
                                        if (pawn::bCanJump(a)) {
                                            mustJump = true;
                                            delta = accelDir * desiredDist;
                                        }
                                    }
                                }
                                if (!pawn::bCanJump(a)) {
                                    if (moveForward) {
                                        destination = location + desiredDir * (desiredDist + radius);
                                        world().lineCheck(hit, location, destination, extent, a, TRACE_World | TRACE_Blocking);
                                        if (hit.time >= 1.0f) {
                                            world().lineCheck(hit, destination, destination + ledgeDown, extent, a, TRACE_World | TRACE_Blocking);
                                            if (hit.time < 1.0f) {
                                                destination = location + desiredDir * desiredDist;
                                                world().lineCheck(hit, destination, destination + ledgeDown, extent, a, TRACE_World | TRACE_Blocking);
                                            }
                                        }
                                        if (hit.time < 1.0f) {
                                            delta = desiredDir * desiredDist;
                                        } else {
                                            moveForward = false;
                                            if (frand() < 2 * tick) {
                                                pawn::MoveTimer(a) = -1.0f;
                                            } else {
                                                pawn::MoveTimer(a) -= 0.1f;
                                            }
                                        }
                                    }
                                    if (!moveForward && !goodMove) {
                                        destination = location + delta + forwardCheck;
                                        world().lineCheck(hit, location, destination, extent, a, TRACE_World | TRACE_Blocking);
                                        if (hit.time >= 1.0f) {
                                            world().lineCheck(hit, destination, destination + ledgeDown, extent, a, TRACE_World | TRACE_Blocking);
                                        } else if (hit.normal.dot(desiredDir) < pawn::MinHitWall(a)) {
                                            pawn::MoveTimer(a) = -1.0f;
                                        }
                                        if (hit.time >= 1.0f) {
                                            world().lineCheck(hit, location, location + ledgeDown, { maxRadius, maxRadius, height }, a, TRACE_World | TRACE_Blocking);
                                            remaining = 0;
                                            pawn::MoveTimer(a) = -1.0f;
                                            actor::Acceleration(a) = {};
                                            if (hit.time >= 1.0f) {
                                                delta = desiredDir * (-groundSpeed * tick);
                                            } else {
                                                delta = {};
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        subMove = delta;
                    }
                    if (pawn::bHitSlopedWall(a)) {
                        float desiredDist = delta.size();
                        if (desiredDist > 0) {
                            Vector desiredDir = delta / desiredDist;
                            Vector location = actor::Location(a);
                            world().lineCheck(hit, location, location + desiredDir * std::max(30.0f, desiredDist + 4), extent, a, TRACE_World | TRACE_Blocking);
                            bool sloped = hit.time < 1.0f && hit.normal.z > 0.01f && hit.normal.z < 0.7f;
                            pawn::bHitSlopedWall(a) = sloped ? 1 : 0;
                            if (sloped) {
                                Vector normal { hit.normal.x, hit.normal.y, 0 };
                                normal = normal.normal();
                                delta = delta - normal * delta.dot(normal);
                            }
                        }
                        move(a, delta, hit);
                    } else {
                        move(a, delta, hit);
                        pawn::bHitSlopedWall(a) = (hit.time < 1.0f && hit.normal.z > 0.01f && hit.normal.z < 0.7f) ? 1 : 0;
                    }
                    if (a->destroyed) {
                        return;
                    }
                    if (hit.time < 1.0f) {
                        Vector desiredDir = delta.normal();
                        stepUp(a, gravDir, desiredDir, delta * (1.0f - hit.time), hit);
                        if (actor::Physics(a) == PHYS_Falling) {
                            float desiredDist = subMove.size();
                            float actualDist = (actor::Location(a) - subLocation).size2D();
                            if (desiredDist > 0) {
                                remaining += tick * (1 - std::min(1.0f, actualDist / desiredDist));
                            }
                            vm::event(a, Name("Falling"));
                            if (actor::Physics(a) == PHYS_Falling) {
                                if (remaining > 0.01f) {
                                    physFalling(a, remaining, iterations);
                                }
                            } else if (actor::Physics(a) == PHYS_Flying) {
                                actor::Velocity(a) = { 0, 0, pawn::AirSpeed(a) };
                                actor::Acceleration(a) = { 0, 0, pawn::AccelRate(a) };
                                if (remaining > 0.01f) {
                                    physFlying(a, remaining, iterations);
                                }
                            }
                            return;
                        }
                    }
                    if (actor::Physics(a) == PHYS_Swimming) {
                        physSwimming(a, remaining, iterations);
                        return;
                    }
                }
                Vector location = actor::Location(a);
                if (zeroMove) {
                    Vector foot = location - Vector { 0, 0, height };
                    world().lineCheck(hit, foot, foot - Vector { 0, 0, 20 }, {}, a, TRACE_World | TRACE_Blocking);
                    float floorDist = hit.time * 20;
                    zeroMove = actor::Base(a) == hit.actor && floorDist <= 4.6f && floorDist >= 4.1f;
                }
                if (!zeroMove) {
                    world().lineCheck(hit, location, location + down, extent, a, TRACE_AllColliding | TRACE_Blocking);
                    float floorDist = hit.time * (maxStep + 2.0f);
                    if (hit.time < 1.0f && (hit.actor != actor::Base(a) || floorDist > 2.4f)) {
                        move(a, down, hit);
                        if (hit.actor != actor::Base(a)) {
                            setSupportBase(a, hit.actor);
                        }
                        if (actor::Physics(a) == PHYS_Swimming) {
                            physSwimming(a, remaining, iterations);
                            return;
                        }
                    } else if (floorDist < 1.9f) {
                        Vector realNormal = hit.normal;
                        move(a, { 0, 0, 2.1f - floorDist }, hit);
                        hit.time = 0;
                        hit.normal = realNormal;
                    }
                    if (!mustJump && hit.time < 1.0f && hit.normal.z >= 0.7f) {
                        float friction = zoneGroundFriction(a);
                        if (hit.normal.z < 1.0f && hit.normal.z * friction < 3.3f) {
                            Vector slide = zoneGravity(a) * (deltaTime * deltaTime / (2 * std::max(0.5f, friction)));
                            Vector slideDelta = slide - hit.normal * slide.dot(hit.normal);
                            if (slideDelta.dot(slide) >= 0) {
                                move(a, slideDelta, hit);
                            }
                            if (actor::Physics(a) == PHYS_Swimming) {
                                physSwimming(a, remaining, iterations);
                                return;
                            }
                        }
                    } else {
                        if (!mustJump && pawn::bCanJump(a) && !checkedFall && probing(a, "MayFall")) {
                            checkedFall = true;
                            vm::event(a, Name("MayFall"));
                        }
                        if (!mustJump && (!pawn::bCanJump(a) || pawn::bIsWalking(a))) {
                            actor::Velocity(a) = {};
                            actor::Acceleration(a) = {};
                            world().farMoveActor(a, actor::OldLocation(a), false, false);
                            pawn::MoveTimer(a) = -1.0f;
                            return;
                        }
                        if (hit.time < 1.0f) {
                            pawn::bHitSlopedWall(a) = 1;
                        }
                        float desiredDist = subMove.size();
                        float actualDist = (actor::Location(a) - subLocation).size2D();
                        if (desiredDist > 0) {
                            remaining += tick * (1 - std::min(1.0f, actualDist / desiredDist));
                        }
                        actor::Velocity(a).z = 0;
                        vm::event(a, Name("Falling"));
                        if (actor::Physics(a) == PHYS_Walking) {
                            setPhysics(a, PHYS_Falling, nullptr);
                        }
                        if (!mustJump && actor::Physics(a) == PHYS_Falling) {
                            float velocityZ = actor::Velocity(a).z;
                            if (!actor::bJustTeleported(a) && deltaTime > remaining) {
                                actor::Velocity(a) = (actor::Location(a) - actor::OldLocation(a)) / (deltaTime - remaining);
                            }
                            actor::Velocity(a).z = velocityZ;
                            if (remaining > 0.01f) {
                                physFalling(a, remaining, iterations);
                            }
                            return;
                        }
                        move(a, desiredMove * remaining, hit);
                        remaining = 0;
                    }
                }
            }
            if (!actor::bJustTeleported(a)) {
                actor::Velocity(a) = (actor::Location(a) - actor::OldLocation(a)) / deltaTime;
            }
            actor::Velocity(a).z = 0;
        }

        Vector fallingVelocity(UObject* a, const Vector& oldVelocity, float tick)
        {
            UObject* zone = zoneOf(a);
            Vector acceleration = actor::Acceleration(a);
            Vector gravity = zoneGravity(a);
            if (!waterZone(zone)) {
                if (a->isA(engineClasses().decoration) && decoration::bBobbing(a)) {
                    return oldVelocity + (acceleration + gravity * 0.5f) * (0.5f * tick);
                }
                if (isPlayerPawn(a)) {
                    UObject* foot = reinterpret_cast<PointRegion*>(pawn::FootRegion(a))->zone;
                    if (waterZone(foot) && oldVelocity.z < 0) {
                        return oldVelocity * (1 - zoneinfo::ZoneFluidFriction(foot) * tick) + (acceleration + gravity) * (0.5f * tick);
                    }
                }
                return oldVelocity + (acceleration + gravity) * (0.5f * tick);
            }
            float mass = std::max(1.0f, actor::Mass(a));
            return oldVelocity * (1 - 2 * zoneFluidFriction(a) * tick) + (acceleration + gravity * (1.0f - actor::Buoyancy(a) / mass)) * (0.5f * tick);
        }

        void physFalling(UObject* a, float deltaTime, int iterations)
        {
            if (region(a).zoneNumber == 0) {
                if (!isPawn(a) || !pawn::bIsPlayer(a)) {
                    setPhysics(a, PHYS_None, nullptr);
                    world().destroy(a);
                }
                return;
            }
            if (isPawn(a)) {
                float maxAccel = pawn::AccelRate(a) * 0.05f;
                float speed2d = actor::Velocity(a).size2D();
                if (speed2d < 10.0f) {
                    maxAccel = maxAccel + (10 - speed2d) / deltaTime;
                } else if (speed2d > pawn::GroundSpeed(a)) {
                    maxAccel = 1.0f;
                }
                Vector& acceleration = actor::Acceleration(a);
                acceleration.z = 0;
                if (acceleration.sizeSquared() > maxAccel * maxAccel) {
                    acceleration = acceleration.normal() * maxAccel;
                }
            }
            if (isPawn(a) && ladderAhead(a, actor::Acceleration(a).zero() ? Vector {} : actor::Acceleration(a).normal())) {
                actor::Velocity(a) = {};
                setPhysics(a, PHYS_Walking, nullptr);
                return;
            }
            float remaining = deltaTime;
            int bounces = 0;
            CheckResult hit;
            bool adjustApex = false;
            while (remaining > 0 && iterations < 8) {
                iterations++;
                float tick = remaining > 0.1f ? std::min(0.1f, remaining * 0.5f) : remaining;
                remaining -= tick;
                actor::OldLocation(a) = actor::Location(a);
                actor::bJustTeleported(a) = 0;
                Vector oldVelocity = actor::Velocity(a);
                actor::Velocity(a) = fallingVelocity(a, oldVelocity, tick);
                Vector velocity = actor::Velocity(a);
                if (!adjustApex && ((oldVelocity.z > 0) != (velocity.z > 0)) && std::abs(oldVelocity.z) > 5.0f && std::abs(velocity.z) > 5.0f) {
                    adjustApex = true;
                    float part = std::abs(oldVelocity.z) / (std::abs(oldVelocity.z) + std::abs(velocity.z));
                    if (part * tick > 0.015f && (1 - part) * tick > 0.015f) {
                        remaining = remaining + tick * (1 - part);
                        tick = tick * part;
                        actor::Velocity(a) = fallingVelocity(a, oldVelocity, tick);
                    }
                } else {
                    adjustApex = false;
                }
                Vector zoneVel = zoneVelocity(a);
                Vector adjusted;
                if (!isPawn(a) || isPlayerPawn(a) || zoneVel.sizeSquared() > 40000.0f) {
                    adjusted = (actor::Velocity(a) + zoneVel) * tick;
                } else {
                    adjusted = actor::Velocity(a) * tick;
                }
                move(a, adjusted, hit);
                if (a->destroyed) {
                    return;
                }
                if (isPawn(a) && actor::Physics(a) == PHYS_Swimming) {
                    remaining = remaining + tick * (1.0f - hit.time);
                    physSwimming(a, remaining, iterations);
                    return;
                }
                if (hit.time < 1.0f) {
                    if (actor::bBounce(a)) {
                        vm::Call(a, Name("HitWall")).v(hit.normal).obj(hit.actor).run();
                        if (actor::Physics(a) == PHYS_None || a->destroyed) {
                            return;
                        }
                        if (bounces < 2) {
                            remaining += tick * (1.0f - hit.time);
                        }
                        bounces++;
                    } else if (hit.normal.z > 0.7f) {
                        remaining += tick * (1.0f - hit.time);
                        if (!actor::bJustTeleported(a) && hit.time > 0.1f && hit.time * tick > 0.003f) {
                            actor::Velocity(a) = (actor::Location(a) - actor::OldLocation(a)) / (tick * hit.time);
                        }
                        processLanded(a, hit.normal, hit.actor, remaining, iterations);
                        return;
                    } else {
                        processHitWall(a, hit.normal, hit.actor);
                        if (a->destroyed || actor::Physics(a) != PHYS_Falling) {
                            return;
                        }
                        Vector oldNormal = hit.normal;
                        Vector delta = (adjusted - hit.normal * adjusted.dot(hit.normal)) * (1.0f - hit.time);
                        if (delta.dot(adjusted) >= 0) {
                            move(a, delta, hit);
                            if (hit.time < 1.0f) {
                                if (hit.normal.z > 0.7f) {
                                    processLanded(a, hit.normal, hit.actor, 0, iterations);
                                    return;
                                }
                                processHitWall(a, hit.normal, hit.actor);
                                Vector desiredDir = adjusted.normal();
                                twoWallAdjust(desiredDir, delta, hit.normal, oldNormal, hit.time);
                                bool ditch = oldNormal.z > 0 && hit.normal.z > 0 && delta.z == 0 && hit.normal.dot(oldNormal) < 0;
                                move(a, delta, hit);
                                if (ditch || hit.normal.z > 0.7f) {
                                    processLanded(a, hit.normal, hit.actor, 0, iterations);
                                    return;
                                }
                            }
                        }
                        float oldZ = oldVelocity.z;
                        oldVelocity = (actor::Location(a) - actor::OldLocation(a)) / tick;
                        oldVelocity.z = oldZ;
                    }
                }
                if (!actor::bBounce(a) && !actor::bJustTeleported(a)) {
                    Vector newVelocity = (actor::Location(a) - actor::OldLocation(a)) / tick;
                    if (newVelocity.z < oldVelocity.z || oldVelocity.z >= 0) {
                        newVelocity = newVelocity * 2.0f - oldVelocity;
                    }
                    float terminal = zoneTerminalVelocity(a);
                    if (newVelocity.sizeSquared() > terminal * terminal) {
                        newVelocity = newVelocity.normal() * terminal;
                    }
                    actor::Velocity(a) = newVelocity;
                }
            }
        }

        void physFlying(UObject* a, float deltaTime, int iterations)
        {
            if (region(a).zoneNumber == 0) {
                if (!pawn::bIsPlayer(a)) {
                    world().destroy(a);
                }
                return;
            }
            Vector acceleration = actor::Acceleration(a);
            Vector accelDir = acceleration.zero() ? Vector {} : acceleration.normal();
            calcVelocity(a, accelDir, deltaTime, pawn::AirSpeed(a), zoneFluidFriction(a), true, false, false);
            actor::OldLocation(a) = actor::Location(a);
            actor::bJustTeleported(a) = 0;
            Vector zoneVel = (isPlayerPawn(a) || zoneVelocity(a).sizeSquared() > 90000.0f) ? zoneVelocity(a) : Vector {};
            Vector adjusted = (actor::Velocity(a) + zoneVel) * deltaTime;
            CheckResult hit;
            move(a, adjusted, hit);
            if (hit.time < 1.0f && !a->destroyed) {
                Vector gravDir { 0, 0, zoneGravity(a).z > 0 ? 1.0f : -1.0f };
                Vector desiredDir = adjusted.normal();
                float upDown = gravDir.dot(actor::Velocity(a).normal());
                if (std::abs(hit.normal.z) < 0.2f && upDown < 0.5f && upDown > -0.2f) {
                    float stepZ = actor::Location(a).z;
                    stepUp(a, gravDir, desiredDir, adjusted * (1.0f - hit.time), hit);
                    actor::OldLocation(a).z = actor::Location(a).z + (actor::OldLocation(a).z - stepZ);
                } else {
                    processHitWall(a, hit.normal, hit.actor);
                    Vector oldNormal = hit.normal;
                    Vector delta = (adjusted - hit.normal * adjusted.dot(hit.normal)) * (1.0f - hit.time);
                    if (delta.dot(adjusted) >= 0) {
                        move(a, delta, hit);
                        if (hit.time < 1.0f) {
                            processHitWall(a, hit.normal, hit.actor);
                            twoWallAdjust(desiredDir, delta, hit.normal, oldNormal, hit.time);
                            move(a, delta, hit);
                        }
                    }
                }
            }
            if (!actor::bJustTeleported(a)) {
                actor::Velocity(a) = (actor::Location(a) - actor::OldLocation(a)) / deltaTime;
            }
            (void)iterations;
        }

        void physSwimming(UObject* a, float deltaTime, int iterations)
        {
            if (deltaTime <= 0) {
                return;
            }
            UObject* head = reinterpret_cast<PointRegion*>(pawn::HeadRegion(a))->zone;
            if (!waterZone(head) && actor::Velocity(a).z > 100.0f) {
                actor::Velocity(a).z = actor::Velocity(a).z * (1 - deltaTime);
            }
            actor::OldLocation(a) = actor::Location(a);
            actor::bJustTeleported(a) = 0;
            Vector acceleration = actor::Acceleration(a);
            Vector accelDir = acceleration.zero() ? Vector {} : acceleration.normal();
            calcVelocity(a, accelDir, deltaTime, pawn::WaterSpeed(a), zoneFluidFriction(a), true, false, true);
            Vector zoneVel = (isPlayerPawn(a) || zoneVelocity(a).sizeSquared() > 90000.0f) ? zoneVelocity(a) : Vector {};
            Vector adjusted = (actor::Velocity(a) + zoneVel) * deltaTime;
            CheckResult hit;
            move(a, adjusted, hit);
            if (hit.time < 1.0f && !a->destroyed) {
                Vector gravDir { 0, 0, zoneGravity(a).z > 0 ? 1.0f : -1.0f };
                Vector desiredDir = adjusted.normal();
                float upDown = gravDir.dot(actor::Velocity(a).normal());
                if (std::abs(hit.normal.z) < 0.2f && upDown < 0.5f && upDown > -0.2f) {
                    float stepZ = actor::Location(a).z;
                    stepUp(a, gravDir, desiredDir, adjusted * (1.0f - hit.time), hit);
                    actor::OldLocation(a).z = actor::Location(a).z + (actor::OldLocation(a).z - stepZ);
                } else {
                    processHitWall(a, hit.normal, hit.actor);
                    Vector delta = (adjusted - hit.normal * adjusted.dot(hit.normal)) * (1.0f - hit.time);
                    if (delta.dot(adjusted) >= 0) {
                        move(a, delta, hit);
                    }
                }
            }
            if (!actor::bJustTeleported(a)) {
                actor::Velocity(a) = (actor::Location(a) - actor::OldLocation(a)) / deltaTime;
            }
            if (!waterZone(zoneOf(a))) {
                if (actor::Physics(a) == PHYS_Swimming) {
                    setPhysics(a, PHYS_Falling, nullptr);
                }
                Vector& velocity = actor::Velocity(a);
                if (velocity.z < 160.0f && velocity.z > 0) {
                    velocity.z = 40.0f + velocity.size2D() * 0.4f;
                }
            }
            (void)iterations;
        }

        void physProjectile(UObject* a, float deltaTime, int iterations)
        {
            if (region(a).zoneNumber == 0) {
                world().destroy(a);
                return;
            }
            float remaining = deltaTime;
            int bounces = 0;
            actor::OldLocation(a) = actor::Location(a);
            actor::bJustTeleported(a) = 0;
            CheckResult hit;
            while (remaining > 0 && iterations < 8) {
                iterations++;
                if (waterZone(zoneOf(a))) {
                    actor::Velocity(a) = actor::Velocity(a) * (1 - 0.2f * zoneFluidFriction(a) * remaining);
                }
                actor::Velocity(a) = actor::Velocity(a) + actor::Acceleration(a) * remaining;
                float tick = remaining;
                remaining = 0;
                if (a->isA(engineClasses().projectile)) {
                    float maxSpeed = projectile::MaxSpeed(a);
                    if (actor::Velocity(a).sizeSquared() > maxSpeed * maxSpeed) {
                        actor::Velocity(a) = actor::Velocity(a).normal() * maxSpeed;
                    }
                }
                Vector adjusted = actor::Velocity(a) * deltaTime;
                move(a, adjusted, hit);
                if (hit.time < 1.0f && !a->destroyed && !actor::bJustTeleported(a)) {
                    vm::Call(a, Name("HitWall")).v(hit.normal).obj(hit.actor).run();
                    if (a->destroyed) {
                        return;
                    }
                    if (actor::bBounce(a)) {
                        if (bounces < 2) {
                            remaining = tick * (1.0f - hit.time);
                        }
                        bounces++;
                        if (actor::Physics(a) == PHYS_Falling) {
                            physFalling(a, remaining, iterations);
                        }
                    }
                }
            }
            if (!actor::bBounce(a) && !actor::bJustTeleported(a) && !a->destroyed) {
                actor::Velocity(a) = (actor::Location(a) - actor::OldLocation(a)) / deltaTime;
            }
        }

        float splerp(float f)
        {
            float s = f * f;
            return (1.0f / 16.0f) * s * s - 0.5f * s + 1;
        }

        Rotator mixRotators(const Rotator* rotators, const float* weights, int count)
        {
            float pitch = 0;
            float yaw = 0;
            float roll = 0;
            for (int i = 0; i < count; i++) {
                pitch += static_cast<float>(rotators[i].pitch) * weights[i];
                yaw += static_cast<float>(rotators[i].yaw) * weights[i];
                roll += static_cast<float>(rotators[i].roll) * weights[i];
            }
            return { static_cast<int32_t>(pitch), static_cast<int32_t>(yaw), static_cast<int32_t>(roll) };
        }

        void physPathing(UObject* a, float deltaTime)
        {
            UClass* interpolationClass = engineClasses().interpolationPoint;
            int guard = 0;
            while (actor::PhysRate(a) != 0 && actor::bInterpolating(a) && deltaTime > 0 && guard++ < 16) {
                UObject* target = actor::Target(a);
                UObject* dest = (target && interpolationClass && target->isA(interpolationClass)) ? target : nullptr;
                UObject* next = dest ? dest->propertyAddress(Name("Next")) ? *reinterpret_cast<UObject**>(dest->propertyAddress(Name("Next"))) : nullptr : nullptr;
                UObject* prev = dest ? dest->propertyAddress(Name("Prev")) ? *reinterpret_cast<UObject**>(dest->propertyAddress(Name("Prev"))) : nullptr : nullptr;
                float rateModifier = 1.0f;
                float& alpha = actor::PhysAlpha(a);
                if (dest && next) {
                    float r0 = *reinterpret_cast<float*>(dest->propertyAddress(Name("RateModifier")));
                    float r1 = *reinterpret_cast<float*>(next->propertyAddress(Name("RateModifier")));
                    rateModifier = r0 * (1 - alpha) + r1 * alpha;
                }
                float oldAlpha = alpha;
                float destAlpha = alpha + actor::PhysRate(a) * rateModifier * deltaTime;
                alpha = std::clamp(destAlpha, 0.0f, 1.0f);
                if (dest && next) {
                    UObject* nextNext = *reinterpret_cast<UObject**>(next->propertyAddress(Name("Next")));
                    Vector newLocation;
                    Rotator newRotation;
                    if (prev && nextNext) {
                        float w[4] = { splerp(alpha + 1), splerp(alpha), splerp(alpha - 1), splerp(alpha - 2) };
                        float inv = 1.0f / (w[0] + w[1] + w[2] + w[3]);
                        for (float& weight : w) {
                            weight *= inv;
                        }
                        newLocation = actor::Location(prev) * w[0] + actor::Location(dest) * w[1] + actor::Location(next) * w[2] + actor::Location(nextNext) * w[3];
                        Rotator rotators[4] = { actor::Rotation(prev), actor::Rotation(dest), actor::Rotation(next), actor::Rotation(nextNext) };
                        newRotation = mixRotators(rotators, w, 4);
                    } else {
                        float w[2] = { 1 - alpha, alpha };
                        newLocation = actor::Location(dest) * w[0] + actor::Location(next) * w[1];
                        Rotator rotators[2] = { actor::Rotation(dest), actor::Rotation(next) };
                        newRotation = mixRotators(rotators, w, 2);
                    }
                    CheckResult hit;
                    world().moveActor(a, newLocation - actor::Location(a), newRotation, hit, false, false, false, false);
                    if (isPawn(a)) {
                        pawn::ViewRotation(a) = actor::Rotation(a);
                    }
                }
                if (actor::PhysRate(a) > 0 && destAlpha > 1.0f) {
                    alpha = 0;
                    deltaTime *= (destAlpha - 1.0f) / (destAlpha - oldAlpha);
                    if (target) {
                        vm::Call(target, Name("InterpolateEnd")).obj(a).run();
                        vm::Call(a, Name("InterpolateEnd")).obj(target).run();
                        if (dest) {
                            actor::Target(a) = next;
                        }
                    }
                } else if (actor::PhysRate(a) < 0 && destAlpha < 0) {
                    alpha = 1;
                    deltaTime *= (0 - destAlpha) / (oldAlpha - destAlpha);
                    if (target) {
                        vm::Call(target, Name("InterpolateEnd")).obj(a).run();
                        vm::Call(a, Name("InterpolateEnd")).obj(target).run();
                        if (dest) {
                            actor::Target(a) = prev;
                        }
                    }
                    vm::Call(a, Name("InterpolateEnd")).obj(nullptr).run();
                } else {
                    deltaTime = 0;
                }
            }
        }

        void physMovingBrush(UObject* a, float deltaTime)
        {
            if (!isMover(a)) {
                return;
            }
            int32_t keyNum = std::clamp<int32_t>(mover::KeyNum(a), 0, 7);
            int guard = 0;
            while (actor::bInterpolating(a) && deltaTime > 0 && guard++ < 8) {
                float alpha = actor::PhysAlpha(a);
                float newAlpha = alpha + deltaTime * actor::PhysRate(a);
                if (newAlpha > 1.0f) {
                    deltaTime *= (newAlpha - 1.0f) / (newAlpha - alpha);
                    newAlpha = 1.0f;
                } else {
                    deltaTime = 0;
                }
                float renderAlpha = newAlpha;
                if (mover::MoverGlideType(a) == 1) {
                    renderAlpha = 3.0f * newAlpha * newAlpha - 2.0f * newAlpha * newAlpha * newAlpha;
                }
                Vector oldPos = mover::OldPos(a);
                Rotator oldRot = mover::OldRot(a);
                Vector targetPos = mover::BasePos(a) + mover::KeyPos(a, keyNum);
                Rotator baseRot = mover::BaseRot(a);
                Rotator keyRot = mover::KeyRot(a, keyNum);
                Rotator targetRot { baseRot.pitch + keyRot.pitch, baseRot.yaw + keyRot.yaw, baseRot.roll + keyRot.roll };
                Vector newLocation = oldPos + (targetPos - oldPos) * renderAlpha;
                Rotator newRotation {
                    oldRot.pitch + static_cast<int32_t>(static_cast<float>(targetRot.pitch - oldRot.pitch) * renderAlpha),
                    oldRot.yaw + static_cast<int32_t>(static_cast<float>(targetRot.yaw - oldRot.yaw) * renderAlpha),
                    oldRot.roll + static_cast<int32_t>(static_cast<float>(targetRot.roll - oldRot.roll) * renderAlpha),
                };
                CheckResult hit;
                if (world().moveActor(a, newLocation - actor::Location(a), newRotation, hit, false, false, false, false)) {
                    actor::PhysAlpha(a) = newAlpha;
                    if (newAlpha == 1.0f) {
                        actor::bInterpolating(a) = 0;
                        vm::Call(a, Name("InterpolateEnd")).obj(nullptr).run();
                    }
                }
            }
        }

        bool nearlyZero(const Vector& v)
        {
            return std::abs(v.x) < 1e-4f && std::abs(v.y) < 1e-4f && std::abs(v.z) < 1e-4f;
        }

        bool checkFloor(UObject* a, const Vector& direction, CheckResult& hit)
        {
            Vector location = actor::Location(a);
            world().lineCheck(hit, location, location - direction * pawn::MaxStepHeight(a), cylinderExtent(a), nullptr, TRACE_World);
            if (hit.time < 1.0f) {
                pawn::Floor(a) = hit.normal;
                return true;
            }
            return false;
        }

        bool findNewFloor(UObject* a, const Vector& oldLocation, float deltaTime, float remainingTime, int iterations)
        {
            CheckResult hit;
            for (const Vector& direction : { Vector { 0, 0, 1 }, Vector { 0, 1, 0 }, Vector { 0, -1, 0 }, Vector { 1, 0, 0 }, Vector { -1, 0, 0 } }) {
                if (checkFloor(a, direction, hit)) {
                    return true;
                }
            }
            vm::event(a, Name("Falling"));
            if (actor::Physics(a) == PHYS_Spider) {
                setPhysics(a, PHYS_Falling, nullptr);
            }
            if (actor::Physics(a) == PHYS_Falling) {
                float velocityZ = actor::Velocity(a).z;
                if (!actor::bJustTeleported(a) && deltaTime > remainingTime) {
                    actor::Velocity(a) = (actor::Location(a) - oldLocation) / (deltaTime - remainingTime);
                }
                actor::Velocity(a).z = velocityZ;
                if (remainingTime > 0.005f) {
                    physFalling(a, remainingTime, iterations);
                }
            }
            return false;
        }

        void physSpider(UObject* a, float deltaTime, int iterations)
        {
            Vector& velocity = actor::Velocity(a);
            Vector& acceleration = actor::Acceleration(a);
            Vector accelDir;
            if (acceleration.zero()) {
                Vector oldVelocity = velocity;
                velocity = velocity - velocity * (2 * deltaTime * zoneGroundFriction(a));
                if (oldVelocity.dot(velocity) < 0) {
                    velocity = acceleration;
                }
            } else {
                accelDir = acceleration.normal();
                float velocitySize = velocity.size();
                float accelRate = pawn::AccelRate(a);
                if (acceleration.sizeSquared() > accelRate * accelRate) {
                    acceleration = accelDir * accelRate;
                }
                velocity = velocity - (velocity - accelDir * velocitySize) * (deltaTime * zoneGroundFriction(a));
            }
            velocity = velocity + acceleration * deltaTime;
            float maxSpeed = pawn::GroundSpeed(a) * pawn::DesiredSpeed(a);
            iterations++;
            if (velocity.sizeSquared() > maxSpeed * maxSpeed) {
                velocity = velocity.normal() * maxSpeed;
            }
            Vector zoneVel = zoneVelocity(a).sizeSquared() > 90000.0f ? zoneVelocity(a) : Vector {};
            Vector desiredMove = velocity + zoneVel;
            float moveSize = desiredMove.size();
            Vector desiredDir = desiredMove / moveSize;
            bool needFloor = nearlyZero(pawn::Floor(a));
            CheckResult hit;
            Vector gravDir = pawn::Floor(a) * -1.0f;
            Vector down = gravDir * (pawn::MaxStepHeight(a) + 4.0f);
            actor::DesiredRotation(a) = actor::Rotation(a);
            if (!needFloor) {
                Vector location = actor::Location(a);
                world().lineCheck(hit, location, location + down, cylinderExtent(a), nullptr, TRACE_World);
                needFloor = hit.time == 1.0f;
            }
            if (needFloor) {
                if (!findNewFloor(a, actor::Location(a), deltaTime, deltaTime, iterations)) {
                    return;
                }
                gravDir = pawn::Floor(a) * -1.0f;
                down = gravDir * (pawn::MaxStepHeight(a) + 4.0f);
            }
            Rotator desiredRotation = vectorRotation(pawn::Floor(a));
            desiredRotation.pitch -= 16384;
            desiredRotation.roll = 0;
            actor::DesiredRotation(a) = desiredRotation;
            float dotp = accelDir.dot(pawn::Floor(a));
            Vector realDir = desiredDir;
            if (pawn::Floor(a).z < 0.6f && dotp > 0.9f) {
                pawn::Floor(a) = {};
                vm::event(a, Name("Falling"));
                setPhysics(a, PHYS_Falling, nullptr);
                physFalling(a, deltaTime, iterations);
                return;
            }
            desiredDir = (desiredDir - pawn::Floor(a) * desiredDir.dot(pawn::Floor(a))).normal();
            actor::OldLocation(a) = actor::Location(a);
            actor::bJustTeleported(a) = 0;
            float remaining = deltaTime;
            desiredMove = desiredDir * moveSize;
            while (remaining > 0 && iterations < 8) {
                iterations++;
                float tick = remaining > 0.05f ? std::min(0.05f, remaining * 0.5f) : remaining;
                remaining -= tick;
                Vector delta = desiredMove * tick;
                if (!nearlyZero(delta)) {
                    world().moveActor(a, delta, actor::DesiredRotation(a), hit, false, false, false, false);
                    if (a->destroyed) {
                        return;
                    }
                    if (hit.time < 1.0f) {
                        if (hit.normal.z >= 0) {
                            if (hit.normal.dot(realDir) < 0 && pawn::Floor(a).dot(realDir) < 0) {
                                vm::Call(a, Name("HitWall")).v(hit.normal).obj(hit.actor).run();
                            } else {
                                Vector combo = (hit.normal + pawn::Floor(a)).normal();
                                if (realDir.dot(combo) > 0.9f) {
                                    vm::Call(a, Name("HitWall")).v(hit.normal).obj(hit.actor).run();
                                }
                            }
                            pawn::Floor(a) = hit.normal;
                            gravDir = pawn::Floor(a) * -1.0f;
                            down = gravDir * (pawn::MaxStepHeight(a) + 4.0f);
                        } else if (hit.normal.dot(realDir) < 0) {
                            vm::Call(a, Name("HitWall")).v(hit.normal).obj(hit.actor).run();
                        } else if (pawn::Floor(a).dot(realDir) > 0.7f) {
                            vm::event(a, Name("Falling"));
                            if (actor::Physics(a) == PHYS_Spider) {
                                setPhysics(a, PHYS_Falling, nullptr);
                            }
                            if (actor::Physics(a) == PHYS_Falling) {
                                float velocityZ = actor::Velocity(a).z;
                                if (!actor::bJustTeleported(a) && deltaTime > remaining) {
                                    actor::Velocity(a) = (actor::Location(a) - actor::OldLocation(a)) / (deltaTime - remaining);
                                }
                                actor::Velocity(a).z = velocityZ;
                                if (remaining > 0.005f) {
                                    physFalling(a, remaining, iterations);
                                }
                                return;
                            }
                        }
                        stepUp(a, gravDir, delta.normal(), delta * (1.0f - hit.time), hit);
                        if (actor::Physics(a) == PHYS_Falling) {
                            if (remaining > 0.005f) {
                                physFalling(a, remaining, iterations);
                            }
                            return;
                        }
                    }
                }
                move(a, down, hit);
                if (a->destroyed) {
                    return;
                }
                if (hit.time == 1.0f) {
                    if (!findNewFloor(a, actor::OldLocation(a), deltaTime, remaining, iterations)) {
                        return;
                    }
                    gravDir = pawn::Floor(a) * -1.0f;
                    down = gravDir * (pawn::MaxStepHeight(a) + 4.0f);
                } else {
                    pawn::Floor(a) = hit.normal;
                    if (hit.actor != actor::Base(a) && !isPawn(hit.actor)) {
                        setSupportBase(a, hit.actor);
                    }
                }
            }
            if (!actor::bJustTeleported(a)) {
                actor::Velocity(a) = (actor::Location(a) - actor::OldLocation(a)) / deltaTime;
            }
        }

        void physTrailer(UObject* a)
        {
            UObject* owner = actor::Owner(a);
            if (!owner) {
                return;
            }
            world().farMoveActor(a, actor::Location(owner), false, true);
            Rotator rotation { 16384, 0, 0 };
            Vector velocity = actor::Velocity(owner);
            if (velocity.sizeSquared() > 1e-4f) {
                rotation = vectorRotation(velocity * -1.0f);
            }
            CheckResult hit;
            world().moveActor(a, {}, rotation, hit, false, false, false, false);
        }

        void physRolling(UObject* a, float deltaTime, int iterations)
        {
            Vector& velocity = actor::Velocity(a);
            Vector velDir = velocity.normal();
            Vector accelDir = actor::Acceleration(a).normal();
            velocity = velocity - (velDir - accelDir) * (velocity.size() * deltaTime * zoneGroundFriction(a));
            velocity = velocity * (1 - zoneFluidFriction(a) * deltaTime) + actor::Acceleration(a) * deltaTime;
            Vector desiredMove = velocity + zoneVelocity(a);
            actor::OldLocation(a) = actor::Location(a);
            actor::bJustTeleported(a) = 0;
            float remaining = deltaTime;
            Vector gravDir { 0, 0, zoneGravity(a).z > 0 ? 1.0f : -1.0f };
            Vector down = gravDir * 16.0f;
            CheckResult hit;
            int bounces = 0;
            while (remaining > 0 && iterations < 8) {
                iterations++;
                float tick = remaining > 0.1f ? std::min(0.1f, remaining * 0.5f) : remaining;
                remaining -= tick;
                Vector delta = desiredMove * tick;
                Vector subMove = delta;
                Vector subLocation = actor::Location(a);
                if (!(std::abs(delta.x) < 1e-4f && std::abs(delta.y) < 1e-4f && std::abs(delta.z) < 1e-4f)) {
                    move(a, delta, hit);
                    if (hit.time < 1.0f) {
                        vm::Call(a, Name("HitWall")).v(hit.normal).obj(hit.actor).run();
                        if (actor::bBounce(a)) {
                            if (bounces < 2) {
                                remaining += tick * (1.0f - hit.time);
                            }
                            bounces++;
                        } else {
                            Vector original = delta;
                            Vector oldNormal = hit.normal;
                            delta = (delta - hit.normal * delta.dot(hit.normal)) * (1.0f - hit.time);
                            if (delta.dot(original) >= 0) {
                                move(a, delta, hit);
                                if (hit.time < 1.0f) {
                                    vm::Call(a, Name("HitWall")).v(hit.normal).obj(hit.actor).run();
                                    twoWallAdjust(desiredMove.normal(), delta, hit.normal, oldNormal, hit.time);
                                    move(a, delta, hit);
                                }
                            }
                        }
                    }
                }
                if (a->destroyed) {
                    return;
                }
                move(a, down, hit);
                float dropTime = hit.time;
                float dropHitZ = hit.normal.z;
                if (dropTime < 1.0f && hit.normal.z < 1.0f && hit.normal.z * zoneGroundFriction(a) < 3.3f) {
                    Vector slide = zoneGravity(a) * (deltaTime / (2 * std::max(0.5f, zoneGroundFriction(a))) * deltaTime);
                    delta = slide - hit.normal * slide.dot(hit.normal);
                    if (delta.dot(slide) >= 0) {
                        move(a, delta, hit);
                        dropHitZ = std::max(dropHitZ, hit.normal.z);
                    }
                }
                if (dropTime == 1.0f || dropHitZ < 0.7f) {
                    move(a, down * (-dropTime), hit);
                    float desiredDist = subMove.size();
                    float actualDist = (actor::Location(a) - subLocation).size2D();
                    remaining += tick * (1 - std::min(1.0f, actualDist / desiredDist));
                    vm::event(a, Name("Falling"));
                    if (actor::Physics(a) == PHYS_Rolling) {
                        setPhysics(a, PHYS_Falling, nullptr);
                    }
                    if (actor::Physics(a) == PHYS_Falling) {
                        if (!actor::bJustTeleported(a) && deltaTime > remaining) {
                            actor::Velocity(a) = (actor::Location(a) - actor::OldLocation(a)) / (deltaTime - remaining);
                        }
                        actor::Velocity(a).z = 0;
                        if (remaining > 0.005f) {
                            physFalling(a, remaining, iterations);
                        }
                        return;
                    }
                    move(a, desiredMove * remaining, hit);
                } else if (hit.actor != actor::Base(a)) {
                    setSupportBase(a, hit.actor);
                }
            }
            if (!actor::bJustTeleported(a)) {
                actor::Velocity(a) = (actor::Location(a) - actor::OldLocation(a)) / deltaTime;
            }
        }
    }

    namespace {
        void postTouch(UObject* a)
        {
            UObject* pending = actor::PendingTouch(a);
            if (!pending) {
                return;
            }
            vm::Call(pending, Name("PostTouch")).obj(a).run();
            pending = actor::PendingTouch(a);
            actor::PendingTouch(a) = actor::PendingTouch(pending);
            actor::PendingTouch(pending) = nullptr;
        }
    }

    void processHitWall(UObject* a, const Vector& hitNormal, UObject* hitActor)
    {
        if (hitActor && isPawn(hitActor)) {
            return;
        }
        if (isPawn(a)) {
            Vector direction = (pawn::Destination(a) - actor::Location(a)).normal();
            Vector normal = hitNormal;
            if (actor::Physics(a) == PHYS_Walking) {
                normal.z = 0;
                direction.z = 0;
            }
            if (pawn::MinHitWall(a) < direction.dot(normal)) {
                return;
            }
            if (actor::Acceleration(a).zero()) {
                return;
            }
            if (!probing(a, "HitWall") && actor::Physics(a) != PHYS_Falling) {
                pawn::MoveTimer(a) = -1.0f;
                pawn::bFromWall(a) = 1;
                return;
            }
        } else if (!probing(a, "HitWall")) {
            return;
        }
        vm::Call(a, Name("HitWall")).v(hitNormal).obj(hitActor).run();
    }

    void processLanded(UObject* a, const Vector& hitNormal, UObject* hitActor, float remainingTime, int iterations)
    {
        if (isPawn(a)) {
            CheckResult hit;
            Vector location = actor::Location(a);
            world().lineCheck(hit, location, location - Vector { 0, 0, 0.2f * actor::CollisionRadius(a) + 8 }, cylinderExtent(a) * 0.9f, a, TRACE_World | TRACE_Blocking | TRACE_OnlyProjActor);
            if (hit.time >= 1.0f) {
                Vector adjusted = location;
                if (world().findSpot(cylinderExtent(a) * 1.1f, adjusted, false, false) && !(adjusted == location)) {
                    world().farMoveActor(a, adjusted, false, false);
                    actor::Velocity(a).x += frand() * 60 - 30;
                    actor::Velocity(a).y += frand() * 60 - 30;
                    return;
                }
            }
        } else if (a->isA(engineClasses().decoration)) {
            decoration::numLandings(a) = 0;
        }
        vm::Call(a, Name("Landed")).v(hitNormal).run();
        if (a->destroyed) {
            return;
        }
        if (actor::Physics(a) == PHYS_Falling) {
            if (isPawn(a)) {
                setPhysics(a, PHYS_Walking, hitActor);
            } else {
                setPhysics(a, PHYS_None, hitActor);
                actor::Velocity(a) = {};
            }
        }
        if (actor::Physics(a) == PHYS_Walking && isPawn(a)) {
            actor::Acceleration(a) = actor::Acceleration(a).normal();
            if (remainingTime > 0.01f) {
                physWalking(a, remainingTime, iterations);
            }
        }
    }

    void performPhysics(UObject* a, float deltaSeconds)
    {
        uint8_t physics = actor::Physics(a);
        if (isPawn(a)) {
            switch (physics) {
            case PHYS_Walking:
                physWalking(a, deltaSeconds, 0);
                break;
            case PHYS_Falling:
                physFalling(a, deltaSeconds, 0);
                break;
            case PHYS_Flying:
                physFlying(a, deltaSeconds, 0);
                break;
            case PHYS_Swimming:
                physSwimming(a, deltaSeconds, 0);
                break;
            case PHYS_Spider:
                physSpider(a, deltaSeconds, 0);
                break;
            case PHYS_Interpolating:
                actor::OldLocation(a) = actor::Location(a);
                physPathing(a, deltaSeconds);
                actor::Velocity(a) = (actor::Location(a) - actor::OldLocation(a)) / deltaSeconds;
                break;
            default:
                break;
            }
            if (a->destroyed) {
                return;
            }
            Rotator rotation = actor::Rotation(a);
            if (actor::Physics(a) != PHYS_Spider && (isPlayerPawn(a) || !(rotation == actor::DesiredRotation(a)) || actor::RotationRate(a).roll > 0)) {
                pawnRotation(a, deltaSeconds);
            }
            pawn::MoveTimer(a) -= deltaSeconds;
            pawn::AvgPhysicsTime(a) = 0.8f * pawn::AvgPhysicsTime(a) + 0.2f * deltaSeconds;
            postTouch(a);
            return;
        }
        switch (physics) {
        case PHYS_Projectile:
            physProjectile(a, deltaSeconds, 0);
            break;
        case PHYS_Falling:
            physFalling(a, deltaSeconds, 0);
            break;
        case PHYS_Interpolating:
            actor::OldLocation(a) = actor::Location(a);
            physPathing(a, deltaSeconds);
            actor::Velocity(a) = (actor::Location(a) - actor::OldLocation(a)) / deltaSeconds;
            break;
        case PHYS_MovingBrush:
            actor::OldLocation(a) = actor::Location(a);
            physMovingBrush(a, deltaSeconds);
            actor::Velocity(a) = (actor::Location(a) - actor::OldLocation(a)) / deltaSeconds;
            break;
        case PHYS_Trailer:
            physTrailer(a);
            break;
        case PHYS_Rolling:
            physRolling(a, deltaSeconds, 0);
            break;
        default:
            break;
        }
        if (a->destroyed) {
            return;
        }
        Rotator rate = actor::RotationRate(a);
        if ((rate.pitch & 65535) != 0 || (rate.yaw & 65535) != 0 || (rate.roll & 65535) != 0) {
            actorRotation(a, deltaSeconds);
        }
        postTouch(a);
    }
}
