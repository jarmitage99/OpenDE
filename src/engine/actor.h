#pragma once

#include "core/math.h"
#include "core/object.h"
#include "engine/props_engine.h"

#include <cstdint>

namespace dx {
    struct PointRegion {
        UObject* zone = nullptr;
        int32_t iLeaf = -1;
        uint8_t zoneNumber = 0;
    };

    enum Physics : uint8_t {
        PHYS_None = 0,
        PHYS_Walking = 1,
        PHYS_Falling = 2,
        PHYS_Swimming = 3,
        PHYS_Flying = 4,
        PHYS_Rotating = 5,
        PHYS_Projectile = 6,
        PHYS_Rolling = 7,
        PHYS_Interpolating = 8,
        PHYS_MovingBrush = 9,
        PHYS_Spider = 10,
        PHYS_Trailer = 11,
    };

    enum DrawType : uint8_t {
        DT_None = 0,
        DT_Sprite = 1,
        DT_Mesh = 2,
        DT_Brush = 3,
        DT_RopeSprite = 4,
        DT_VerticalSprite = 5,
        DT_Terraform = 6,
        DT_SpriteAnimOnce = 7,
    };

    struct EngineClasses {
        UClass* actor = nullptr;
        UClass* pawn = nullptr;
        UClass* playerPawn = nullptr;
        UClass* decoration = nullptr;
        UClass* inventory = nullptr;
        UClass* weapon = nullptr;
        UClass* projectile = nullptr;
        UClass* mover = nullptr;
        UClass* brush = nullptr;
        UClass* levelInfo = nullptr;
        UClass* zoneInfo = nullptr;
        UClass* skyZoneInfo = nullptr;
        UClass* warpZoneInfo = nullptr;
        UClass* navigationPoint = nullptr;
        UClass* interpolationPoint = nullptr;
        UClass* carcass = nullptr;
        UClass* gameInfo = nullptr;
        UClass* camera = nullptr;
        UClass* light = nullptr;
        UClass* viewport = nullptr;
        UClass* canvas = nullptr;
        UClass* playerPawnExt = nullptr;
        UClass* scriptedPawn = nullptr;
        UClass* deusExPlayer = nullptr;
    };

    EngineClasses& engineClasses();
    void bindEngineClasses();

    inline PointRegion& region(UObject* actor)
    {
        return *reinterpret_cast<PointRegion*>(actor::Region(actor));
    }

    inline Vector cylinderExtent(UObject* a)
    {
        return { actor::CollisionRadius(a), actor::CollisionRadius(a), actor::CollisionHeight(a) };
    }

    inline bool isPawn(UObject* a)
    {
        return a && actor::bIsPawn(a);
    }

    inline bool isPlayerPawn(UObject* a)
    {
        return a && a->isA(engineClasses().playerPawn);
    }

    inline bool isBrush(UObject* a)
    {
        return a && a->isA(engineClasses().brush);
    }

    inline bool isMover(UObject* a)
    {
        return a && a->isA(engineClasses().mover);
    }

    inline bool isMovingBrush(UObject* a)
    {
        return isMover(a);
    }

    inline UObject* zoneOf(UObject* a)
    {
        return region(a).zone;
    }

    UObject* playerPawnOf(UObject* a);
    bool isBlockedBy(UObject* a, UObject* other);
    bool isBasedOn(UObject* a, UObject* other);
    bool isOverlapping(UObject* a, UObject* other);
    void setOwner(UObject* a, UObject* newOwner);
    void setBase(UObject* a, UObject* newBase, bool notify);
    void setSupportBase(UObject* a, UObject* newBase);
    void beginTouch(UObject* a, UObject* other);
    void endTouch(UObject* a, UObject* other, bool noNotifySelf);
    void setPhysics(UObject* a, uint8_t newPhysics, UObject* newFloor);
    void findBase(UObject* a);
    void setCollision(UObject* a, bool collideActors, bool blockActors, bool blockPlayers);
    bool moveSmooth(UObject* a, const Vector& delta);
    void performPhysics(UObject* a, float deltaSeconds);
    void updateAnimation(UObject* a, float deltaSeconds);
    void processHitWall(UObject* a, const Vector& hitNormal, UObject* hitActor);
    void processLanded(UObject* a, const Vector& hitNormal, UObject* hitActor, float remainingTime, int iterations);
}
