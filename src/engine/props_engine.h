#pragma once

#include "core/math.h"
#include "core/object.h"

#include <string>

namespace dx {
    namespace actor {
        struct Offsets {
            size_t bStatic = 0;
            size_t bHidden = 0;
            size_t bNoDelete = 0;
            size_t bAnimFinished = 0;
            size_t bAnimLoop = 0;
            size_t bAnimNotify = 0;
            size_t bAnimByOwner = 0;
            size_t bDeleteMe = 0;
            size_t bAssimilated = 0;
            size_t bTicked = 0;
            size_t bLightChanged = 0;
            size_t bDynamicLight = 0;
            size_t bTimerLoop = 0;
            size_t bCanTeleport = 0;
            size_t bIsSecretGoal = 0;
            size_t bIsKillGoal = 0;
            size_t bIsItemGoal = 0;
            size_t bCollideWhenPlacing = 0;
            size_t bTravel = 0;
            size_t bMovable = 0;
            size_t bHighDetail = 0;
            size_t bStasis = 0;
            size_t bForceStasis = 0;
            size_t bIsPawn = 0;
            size_t bNetTemporary = 0;
            size_t bNetOptional = 0;
            size_t bReplicateInstigator = 0;
            size_t bTrailerSameRotation = 0;
            size_t bTrailerPrePivot = 0;
            size_t bClientAnim = 0;
            size_t bSimFall = 0;
            size_t bBlockSight = 0;
            size_t bDetectable = 0;
            size_t bTransient = 0;
            size_t bIgnore = 0;
            size_t Physics = 0;
            size_t Role = 0;
            size_t RemoteRole = 0;
            size_t LastRenderTime = 0;
            size_t DistanceFromPlayer = 0;
            size_t Owner = 0;
            size_t InitialState = 0;
            size_t Group = 0;
            size_t TimerRate = 0;
            size_t TimerCounter = 0;
            size_t LifeSpan = 0;
            size_t AnimSequence = 0;
            size_t AnimFrame = 0;
            size_t AnimRate = 0;
            size_t TweenRate = 0;
            size_t LODBias = 0;
            size_t BlendAnimSequence = 0;
            size_t BlendAnimFrame = 0;
            size_t BlendAnimRate = 0;
            size_t BlendTweenRate = 0;
            size_t Level = 0;
            size_t XLevel = 0;
            size_t Tag = 0;
            size_t Event = 0;
            size_t Target = 0;
            size_t Instigator = 0;
            size_t Inventory = 0;
            size_t Base = 0;
            size_t Region = 0;
            size_t AttachTag = 0;
            size_t StandingCount = 0;
            size_t MiscNumber = 0;
            size_t LatentByte = 0;
            size_t LatentInt = 0;
            size_t LatentFloat = 0;
            size_t LatentActor = 0;
            size_t Touching = 0;
            size_t Deleted = 0;
            size_t CollisionTag = 0;
            size_t LightingTag = 0;
            size_t NetTag = 0;
            size_t OtherTag = 0;
            size_t ExtraTag = 0;
            size_t SpecialTag = 0;
            size_t Location = 0;
            size_t Rotation = 0;
            size_t OldLocation = 0;
            size_t ColLocation = 0;
            size_t Velocity = 0;
            size_t Acceleration = 0;
            size_t bHiddenEd = 0;
            size_t bDirectional = 0;
            size_t bSelected = 0;
            size_t bMemorized = 0;
            size_t bHighlighted = 0;
            size_t bEdLocked = 0;
            size_t bEdShouldSnap = 0;
            size_t bEdSnap = 0;
            size_t bTempEditor = 0;
            size_t bDifficulty0 = 0;
            size_t bDifficulty1 = 0;
            size_t bDifficulty2 = 0;
            size_t bDifficulty3 = 0;
            size_t bSinglePlayer = 0;
            size_t bNet = 0;
            size_t bNetSpecial = 0;
            size_t OddsOfAppearing = 0;
            size_t DrawType = 0;
            size_t Style = 0;
            size_t Sprite = 0;
            size_t Texture = 0;
            size_t Skin = 0;
            size_t Mesh = 0;
            size_t Brush = 0;
            size_t DrawScale = 0;
            size_t PrePivot = 0;
            size_t ScaleGlow = 0;
            size_t AmbientGlow = 0;
            size_t Fatness = 0;
            size_t bUnlit = 0;
            size_t bNoSmooth = 0;
            size_t bParticles = 0;
            size_t bRandomFrame = 0;
            size_t bMeshEnviroMap = 0;
            size_t bMeshCurvy = 0;
            size_t VisibilityRadius = 0;
            size_t VisibilityHeight = 0;
            size_t bShadowCast = 0;
            size_t bOwnerNoSee = 0;
            size_t bOnlyOwnerSee = 0;
            size_t bIsMover = 0;
            size_t bAlwaysRelevant = 0;
            size_t bAlwaysTick = 0;
            size_t bHurtEntry = 0;
            size_t bGameRelevant = 0;
            size_t bCarriedItem = 0;
            size_t bForcePhysicsUpdate = 0;
            size_t MultiSkins = 0;
            size_t SoundRadius = 0;
            size_t SoundVolume = 0;
            size_t SoundPitch = 0;
            size_t AmbientSound = 0;
            size_t TransientSoundVolume = 0;
            size_t TransientSoundRadius = 0;
            size_t CollisionRadius = 0;
            size_t CollisionHeight = 0;
            size_t bCollideActors = 0;
            size_t bCollideWorld = 0;
            size_t bBlockActors = 0;
            size_t bBlockPlayers = 0;
            size_t bProjTarget = 0;
            size_t LightType = 0;
            size_t LightEffect = 0;
            size_t LightBrightness = 0;
            size_t LightHue = 0;
            size_t LightSaturation = 0;
            size_t LightRadius = 0;
            size_t LightPeriod = 0;
            size_t LightPhase = 0;
            size_t LightCone = 0;
            size_t VolumeBrightness = 0;
            size_t VolumeRadius = 0;
            size_t VolumeFog = 0;
            size_t bSpecialLit = 0;
            size_t bActorShadows = 0;
            size_t bCorona = 0;
            size_t bLensFlare = 0;
            size_t bBounce = 0;
            size_t bFixedRotationDir = 0;
            size_t bRotateToDesired = 0;
            size_t bInterpolating = 0;
            size_t bJustTeleported = 0;
            size_t DodgeDir = 0;
            size_t Mass = 0;
            size_t Buoyancy = 0;
            size_t RotationRate = 0;
            size_t DesiredRotation = 0;
            size_t PhysAlpha = 0;
            size_t PhysRate = 0;
            size_t PendingTouch = 0;
            size_t AnimLast = 0;
            size_t AnimMinRate = 0;
            size_t OldAnimRate = 0;
            size_t SimAnim = 0;
            size_t BlendAnimLast = 0;
            size_t BlendAnimMinRate = 0;
            size_t OldBlendAnimRate = 0;
            size_t SimBlendAnim = 0;
            size_t BindName = 0;
            size_t BarkBindName = 0;
            size_t FamiliarName = 0;
            size_t UnfamiliarName = 0;
            size_t ConListItems = 0;
            size_t LastConEndTime = 0;
            size_t ConStartInterval = 0;
            size_t VisUpdateTime = 0;
            size_t CurrentVisibility = 0;
            size_t LastVisibility = 0;
            size_t SmellClass = 0;
            size_t LastSmellNode = 0;
            size_t bOwned = 0;
            size_t bVisionImportant = 0;
            size_t NetPriority = 0;
            size_t NetUpdateFrequency = 0;
            size_t RelevantRadius = 0;
            size_t bNetInitial = 0;
            size_t bNetOwner = 0;
            size_t bNetRelevant = 0;
            size_t bNetSee = 0;
            size_t bNetHear = 0;
            size_t bNetFeel = 0;
            size_t bSimulatedPawn = 0;
            size_t bDemoRecording = 0;
            size_t bClientDemoRecording = 0;
            size_t bClientDemoNetFunc = 0;
            size_t RenderIteratorClass = 0;
            size_t RenderInterface = 0;
        };
    }

    extern actor::Offsets offsets_actor;

    namespace actor {
        inline uint32_t& bStatic(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bStatic);
        }

        inline uint32_t& bHidden(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bHidden);
        }

        inline uint32_t& bNoDelete(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bNoDelete);
        }

        inline uint32_t& bAnimFinished(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bAnimFinished);
        }

        inline uint32_t& bAnimLoop(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bAnimLoop);
        }

        inline uint32_t& bAnimNotify(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bAnimNotify);
        }

        inline uint32_t& bAnimByOwner(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bAnimByOwner);
        }

        inline uint32_t& bDeleteMe(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bDeleteMe);
        }

        inline uint32_t& bAssimilated(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bAssimilated);
        }

        inline uint32_t& bTicked(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bTicked);
        }

        inline uint32_t& bLightChanged(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bLightChanged);
        }

        inline uint32_t& bDynamicLight(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bDynamicLight);
        }

        inline uint32_t& bTimerLoop(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bTimerLoop);
        }

        inline uint32_t& bCanTeleport(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bCanTeleport);
        }

        inline uint32_t& bIsSecretGoal(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bIsSecretGoal);
        }

        inline uint32_t& bIsKillGoal(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bIsKillGoal);
        }

        inline uint32_t& bIsItemGoal(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bIsItemGoal);
        }

        inline uint32_t& bCollideWhenPlacing(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bCollideWhenPlacing);
        }

        inline uint32_t& bTravel(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bTravel);
        }

        inline uint32_t& bMovable(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bMovable);
        }

        inline uint32_t& bHighDetail(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bHighDetail);
        }

        inline uint32_t& bStasis(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bStasis);
        }

        inline uint32_t& bForceStasis(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bForceStasis);
        }

        inline uint32_t& bIsPawn(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bIsPawn);
        }

        inline uint32_t& bNetTemporary(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bNetTemporary);
        }

        inline uint32_t& bNetOptional(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bNetOptional);
        }

        inline uint32_t& bReplicateInstigator(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bReplicateInstigator);
        }

        inline uint32_t& bTrailerSameRotation(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bTrailerSameRotation);
        }

        inline uint32_t& bTrailerPrePivot(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bTrailerPrePivot);
        }

        inline uint32_t& bClientAnim(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bClientAnim);
        }

        inline uint32_t& bSimFall(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bSimFall);
        }

        inline uint32_t& bBlockSight(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bBlockSight);
        }

        inline uint32_t& bDetectable(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bDetectable);
        }

        inline uint32_t& bTransient(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bTransient);
        }

        inline uint32_t& bIgnore(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bIgnore);
        }

        inline uint8_t& Physics(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_actor.Physics);
        }

        inline uint8_t& Role(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_actor.Role);
        }

        inline uint8_t& RemoteRole(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_actor.RemoteRole);
        }

        inline float& LastRenderTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.LastRenderTime);
        }

        inline float& DistanceFromPlayer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.DistanceFromPlayer);
        }

        inline UObject*& Owner(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_actor.Owner);
        }

        inline Name& InitialState(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_actor.InitialState);
        }

        inline Name& Group(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_actor.Group);
        }

        inline float& TimerRate(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.TimerRate);
        }

        inline float& TimerCounter(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.TimerCounter);
        }

        inline float& LifeSpan(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.LifeSpan);
        }

        inline Name& AnimSequence(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_actor.AnimSequence);
        }

        inline float& AnimFrame(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.AnimFrame);
        }

        inline float& AnimRate(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.AnimRate);
        }

        inline float& TweenRate(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.TweenRate);
        }

        inline float& LODBias(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.LODBias);
        }

        inline Name& BlendAnimSequence(UObject* o, int index)
        {
            return reinterpret_cast<Name*>(o->data + offsets_actor.BlendAnimSequence)[index];
        }

        inline float& BlendAnimFrame(UObject* o, int index)
        {
            return reinterpret_cast<float*>(o->data + offsets_actor.BlendAnimFrame)[index];
        }

        inline float& BlendAnimRate(UObject* o, int index)
        {
            return reinterpret_cast<float*>(o->data + offsets_actor.BlendAnimRate)[index];
        }

        inline float& BlendTweenRate(UObject* o, int index)
        {
            return reinterpret_cast<float*>(o->data + offsets_actor.BlendTweenRate)[index];
        }

        inline UObject*& Level(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_actor.Level);
        }

        inline UObject*& XLevel(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_actor.XLevel);
        }

        inline Name& Tag(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_actor.Tag);
        }

        inline Name& Event(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_actor.Event);
        }

        inline UObject*& Target(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_actor.Target);
        }

        inline UObject*& Instigator(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_actor.Instigator);
        }

        inline UObject*& Inventory(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_actor.Inventory);
        }

        inline UObject*& Base(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_actor.Base);
        }

        inline uint8_t* Region(UObject* o)
        {
            return o->data + offsets_actor.Region;
        }

        inline Name& AttachTag(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_actor.AttachTag);
        }

        inline uint8_t& StandingCount(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_actor.StandingCount);
        }

        inline uint8_t& MiscNumber(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_actor.MiscNumber);
        }

        inline uint8_t& LatentByte(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_actor.LatentByte);
        }

        inline int32_t& LatentInt(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_actor.LatentInt);
        }

        inline float& LatentFloat(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.LatentFloat);
        }

        inline UObject*& LatentActor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_actor.LatentActor);
        }

        inline UObject*& Touching(UObject* o, int index)
        {
            return reinterpret_cast<UObject**>(o->data + offsets_actor.Touching)[index];
        }

        inline UObject*& Deleted(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_actor.Deleted);
        }

        inline int32_t& CollisionTag(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_actor.CollisionTag);
        }

        inline int32_t& LightingTag(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_actor.LightingTag);
        }

        inline int32_t& NetTag(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_actor.NetTag);
        }

        inline int32_t& OtherTag(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_actor.OtherTag);
        }

        inline int32_t& ExtraTag(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_actor.ExtraTag);
        }

        inline int32_t& SpecialTag(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_actor.SpecialTag);
        }

        inline Vector& Location(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_actor.Location);
        }

        inline Rotator& Rotation(UObject* o)
        {
            return *reinterpret_cast<Rotator*>(o->data + offsets_actor.Rotation);
        }

        inline Vector& OldLocation(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_actor.OldLocation);
        }

        inline Vector& ColLocation(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_actor.ColLocation);
        }

        inline Vector& Velocity(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_actor.Velocity);
        }

        inline Vector& Acceleration(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_actor.Acceleration);
        }

        inline uint32_t& bHiddenEd(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bHiddenEd);
        }

        inline uint32_t& bDirectional(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bDirectional);
        }

        inline uint32_t& bSelected(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bSelected);
        }

        inline uint32_t& bMemorized(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bMemorized);
        }

        inline uint32_t& bHighlighted(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bHighlighted);
        }

        inline uint32_t& bEdLocked(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bEdLocked);
        }

        inline uint32_t& bEdShouldSnap(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bEdShouldSnap);
        }

        inline uint32_t& bEdSnap(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bEdSnap);
        }

        inline uint32_t& bTempEditor(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bTempEditor);
        }

        inline uint32_t& bDifficulty0(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bDifficulty0);
        }

        inline uint32_t& bDifficulty1(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bDifficulty1);
        }

        inline uint32_t& bDifficulty2(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bDifficulty2);
        }

        inline uint32_t& bDifficulty3(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bDifficulty3);
        }

        inline uint32_t& bSinglePlayer(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bSinglePlayer);
        }

        inline uint32_t& bNet(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bNet);
        }

        inline uint32_t& bNetSpecial(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bNetSpecial);
        }

        inline float& OddsOfAppearing(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.OddsOfAppearing);
        }

        inline uint8_t& DrawType(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_actor.DrawType);
        }

        inline uint8_t& Style(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_actor.Style);
        }

        inline UObject*& Sprite(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_actor.Sprite);
        }

        inline UObject*& Texture(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_actor.Texture);
        }

        inline UObject*& Skin(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_actor.Skin);
        }

        inline UObject*& Mesh(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_actor.Mesh);
        }

        inline UObject*& Brush(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_actor.Brush);
        }

        inline float& DrawScale(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.DrawScale);
        }

        inline Vector& PrePivot(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_actor.PrePivot);
        }

        inline float& ScaleGlow(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.ScaleGlow);
        }

        inline uint8_t& AmbientGlow(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_actor.AmbientGlow);
        }

        inline uint8_t& Fatness(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_actor.Fatness);
        }

        inline uint32_t& bUnlit(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bUnlit);
        }

        inline uint32_t& bNoSmooth(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bNoSmooth);
        }

        inline uint32_t& bParticles(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bParticles);
        }

        inline uint32_t& bRandomFrame(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bRandomFrame);
        }

        inline uint32_t& bMeshEnviroMap(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bMeshEnviroMap);
        }

        inline uint32_t& bMeshCurvy(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bMeshCurvy);
        }

        inline float& VisibilityRadius(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.VisibilityRadius);
        }

        inline float& VisibilityHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.VisibilityHeight);
        }

        inline uint32_t& bShadowCast(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bShadowCast);
        }

        inline uint32_t& bOwnerNoSee(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bOwnerNoSee);
        }

        inline uint32_t& bOnlyOwnerSee(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bOnlyOwnerSee);
        }

        inline uint32_t& bIsMover(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bIsMover);
        }

        inline uint32_t& bAlwaysRelevant(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bAlwaysRelevant);
        }

        inline uint32_t& bAlwaysTick(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bAlwaysTick);
        }

        inline uint32_t& bHurtEntry(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bHurtEntry);
        }

        inline uint32_t& bGameRelevant(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bGameRelevant);
        }

        inline uint32_t& bCarriedItem(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bCarriedItem);
        }

        inline uint32_t& bForcePhysicsUpdate(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bForcePhysicsUpdate);
        }

        inline UObject*& MultiSkins(UObject* o, int index)
        {
            return reinterpret_cast<UObject**>(o->data + offsets_actor.MultiSkins)[index];
        }

        inline uint8_t& SoundRadius(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_actor.SoundRadius);
        }

        inline uint8_t& SoundVolume(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_actor.SoundVolume);
        }

        inline uint8_t& SoundPitch(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_actor.SoundPitch);
        }

        inline UObject*& AmbientSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_actor.AmbientSound);
        }

        inline float& TransientSoundVolume(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.TransientSoundVolume);
        }

        inline float& TransientSoundRadius(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.TransientSoundRadius);
        }

        inline float& CollisionRadius(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.CollisionRadius);
        }

        inline float& CollisionHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.CollisionHeight);
        }

        inline uint32_t& bCollideActors(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bCollideActors);
        }

        inline uint32_t& bCollideWorld(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bCollideWorld);
        }

        inline uint32_t& bBlockActors(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bBlockActors);
        }

        inline uint32_t& bBlockPlayers(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bBlockPlayers);
        }

        inline uint32_t& bProjTarget(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bProjTarget);
        }

        inline uint8_t& LightType(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_actor.LightType);
        }

        inline uint8_t& LightEffect(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_actor.LightEffect);
        }

        inline uint8_t& LightBrightness(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_actor.LightBrightness);
        }

        inline uint8_t& LightHue(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_actor.LightHue);
        }

        inline uint8_t& LightSaturation(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_actor.LightSaturation);
        }

        inline uint8_t& LightRadius(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_actor.LightRadius);
        }

        inline uint8_t& LightPeriod(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_actor.LightPeriod);
        }

        inline uint8_t& LightPhase(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_actor.LightPhase);
        }

        inline uint8_t& LightCone(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_actor.LightCone);
        }

        inline uint8_t& VolumeBrightness(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_actor.VolumeBrightness);
        }

        inline uint8_t& VolumeRadius(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_actor.VolumeRadius);
        }

        inline uint8_t& VolumeFog(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_actor.VolumeFog);
        }

        inline uint32_t& bSpecialLit(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bSpecialLit);
        }

        inline uint32_t& bActorShadows(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bActorShadows);
        }

        inline uint32_t& bCorona(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bCorona);
        }

        inline uint32_t& bLensFlare(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bLensFlare);
        }

        inline uint32_t& bBounce(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bBounce);
        }

        inline uint32_t& bFixedRotationDir(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bFixedRotationDir);
        }

        inline uint32_t& bRotateToDesired(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bRotateToDesired);
        }

        inline uint32_t& bInterpolating(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bInterpolating);
        }

        inline uint32_t& bJustTeleported(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bJustTeleported);
        }

        inline uint8_t& DodgeDir(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_actor.DodgeDir);
        }

        inline float& Mass(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.Mass);
        }

        inline float& Buoyancy(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.Buoyancy);
        }

        inline Rotator& RotationRate(UObject* o)
        {
            return *reinterpret_cast<Rotator*>(o->data + offsets_actor.RotationRate);
        }

        inline Rotator& DesiredRotation(UObject* o)
        {
            return *reinterpret_cast<Rotator*>(o->data + offsets_actor.DesiredRotation);
        }

        inline float& PhysAlpha(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.PhysAlpha);
        }

        inline float& PhysRate(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.PhysRate);
        }

        inline UObject*& PendingTouch(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_actor.PendingTouch);
        }

        inline float& AnimLast(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.AnimLast);
        }

        inline float& AnimMinRate(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.AnimMinRate);
        }

        inline float& OldAnimRate(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.OldAnimRate);
        }

        inline Plane& SimAnim(UObject* o)
        {
            return *reinterpret_cast<Plane*>(o->data + offsets_actor.SimAnim);
        }

        inline float& BlendAnimLast(UObject* o, int index)
        {
            return reinterpret_cast<float*>(o->data + offsets_actor.BlendAnimLast)[index];
        }

        inline float& BlendAnimMinRate(UObject* o, int index)
        {
            return reinterpret_cast<float*>(o->data + offsets_actor.BlendAnimMinRate)[index];
        }

        inline float& OldBlendAnimRate(UObject* o, int index)
        {
            return reinterpret_cast<float*>(o->data + offsets_actor.OldBlendAnimRate)[index];
        }

        inline Plane& SimBlendAnim(UObject* o, int index)
        {
            return reinterpret_cast<Plane*>(o->data + offsets_actor.SimBlendAnim)[index];
        }

        inline std::string& BindName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_actor.BindName);
        }

        inline std::string& BarkBindName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_actor.BarkBindName);
        }

        inline std::string& FamiliarName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_actor.FamiliarName);
        }

        inline std::string& UnfamiliarName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_actor.UnfamiliarName);
        }

        inline UObject*& ConListItems(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_actor.ConListItems);
        }

        inline float& LastConEndTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.LastConEndTime);
        }

        inline float& ConStartInterval(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.ConStartInterval);
        }

        inline float& VisUpdateTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.VisUpdateTime);
        }

        inline float& CurrentVisibility(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.CurrentVisibility);
        }

        inline float& LastVisibility(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.LastVisibility);
        }

        inline UObject*& SmellClass(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_actor.SmellClass);
        }

        inline UObject*& LastSmellNode(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_actor.LastSmellNode);
        }

        inline uint32_t& bOwned(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bOwned);
        }

        inline uint32_t& bVisionImportant(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bVisionImportant);
        }

        inline float& NetPriority(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.NetPriority);
        }

        inline float& NetUpdateFrequency(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.NetUpdateFrequency);
        }

        inline float& RelevantRadius(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_actor.RelevantRadius);
        }

        inline uint32_t& bNetInitial(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bNetInitial);
        }

        inline uint32_t& bNetOwner(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bNetOwner);
        }

        inline uint32_t& bNetRelevant(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bNetRelevant);
        }

        inline uint32_t& bNetSee(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bNetSee);
        }

        inline uint32_t& bNetHear(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bNetHear);
        }

        inline uint32_t& bNetFeel(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bNetFeel);
        }

        inline uint32_t& bSimulatedPawn(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bSimulatedPawn);
        }

        inline uint32_t& bDemoRecording(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bDemoRecording);
        }

        inline uint32_t& bClientDemoRecording(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bClientDemoRecording);
        }

        inline uint32_t& bClientDemoNetFunc(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_actor.bClientDemoNetFunc);
        }

        inline UObject*& RenderIteratorClass(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_actor.RenderIteratorClass);
        }

        inline UObject*& RenderInterface(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_actor.RenderInterface);
        }

    }

    namespace pawn {
        struct Offsets {
            size_t bBehindView = 0;
            size_t bIsPlayer = 0;
            size_t bJustLanded = 0;
            size_t bUpAndOut = 0;
            size_t bIsWalking = 0;
            size_t bHitSlopedWall = 0;
            size_t bNeverSwitchOnPickup = 0;
            size_t bWarping = 0;
            size_t bUpdatingDisplay = 0;
            size_t bCanStrafe = 0;
            size_t bFixedStart = 0;
            size_t bReducedSpeed = 0;
            size_t bCanJump = 0;
            size_t bCanWalk = 0;
            size_t bCanSwim = 0;
            size_t bCanFly = 0;
            size_t bCanOpenDoors = 0;
            size_t bCanDoSpecial = 0;
            size_t bDrowning = 0;
            size_t bLOSflag = 0;
            size_t bFromWall = 0;
            size_t bHunting = 0;
            size_t bAvoidLedges = 0;
            size_t bStopAtLedges = 0;
            size_t bJumpOffPawn = 0;
            size_t bShootSpecial = 0;
            size_t bAutoActivate = 0;
            size_t bIsHuman = 0;
            size_t bIsFemale = 0;
            size_t bIsMultiSkinned = 0;
            size_t bCountJumps = 0;
            size_t bAdvancedTactics = 0;
            size_t bViewTarget = 0;
            size_t bCanGlide = 0;
            size_t SightCounter = 0;
            size_t PainTime = 0;
            size_t SpeechTime = 0;
            size_t AvgPhysicsTime = 0;
            size_t FootRegion = 0;
            size_t HeadRegion = 0;
            size_t MoveTimer = 0;
            size_t MoveTarget = 0;
            size_t FaceTarget = 0;
            size_t Destination = 0;
            size_t Focus = 0;
            size_t DesiredSpeed = 0;
            size_t MaxDesiredSpeed = 0;
            size_t MeleeRange = 0;
            size_t GroundSpeed = 0;
            size_t WaterSpeed = 0;
            size_t AirSpeed = 0;
            size_t AccelRate = 0;
            size_t JumpZ = 0;
            size_t MaxStepHeight = 0;
            size_t AirControl = 0;
            size_t MinHitWall = 0;
            size_t Visibility = 0;
            size_t Alertness = 0;
            size_t Stimulus = 0;
            size_t SightRadius = 0;
            size_t PeripheralVision = 0;
            size_t HearingThreshold = 0;
            size_t LastSeenPos = 0;
            size_t LastSeeingPos = 0;
            size_t LastSeenTime = 0;
            size_t Enemy = 0;
            size_t Weapon = 0;
            size_t PendingWeapon = 0;
            size_t SelectedItem = 0;
            size_t ViewRotation = 0;
            size_t WalkBob = 0;
            size_t BaseEyeHeight = 0;
            size_t EyeHeight = 0;
            size_t Floor = 0;
            size_t SplashTime = 0;
            size_t OrthoZoom = 0;
            size_t FovAngle = 0;
            size_t DieCount = 0;
            size_t ItemCount = 0;
            size_t KillCount = 0;
            size_t SecretCount = 0;
            size_t Spree = 0;
            size_t Health = 0;
            size_t SelectionMesh = 0;
            size_t SpecialMesh = 0;
            size_t ReducedDamageType = 0;
            size_t ReducedDamagePct = 0;
            size_t DropWhenKilled = 0;
            size_t UnderWaterTime = 0;
            size_t AttitudeToPlayer = 0;
            size_t Intelligence = 0;
            size_t Skill = 0;
            size_t SpecialGoal = 0;
            size_t SpecialPause = 0;
            size_t noise1spot = 0;
            size_t noise1time = 0;
            size_t noise1other = 0;
            size_t noise1loudness = 0;
            size_t noise2spot = 0;
            size_t noise2time = 0;
            size_t noise2other = 0;
            size_t noise2loudness = 0;
            size_t LastPainSound = 0;
            size_t nextPawn = 0;
            size_t HitSound1 = 0;
            size_t HitSound2 = 0;
            size_t Land = 0;
            size_t Die = 0;
            size_t WaterStep = 0;
            size_t bZoom = 0;
            size_t bRun = 0;
            size_t bLook = 0;
            size_t bDuck = 0;
            size_t bSnapLevel = 0;
            size_t bStrafe = 0;
            size_t bFire = 0;
            size_t bAltFire = 0;
            size_t bFreeLook = 0;
            size_t bExtra0 = 0;
            size_t bExtra1 = 0;
            size_t bExtra2 = 0;
            size_t bExtra3 = 0;
            size_t CombatStyle = 0;
            size_t home = 0;
            size_t NextState = 0;
            size_t NextLabel = 0;
            size_t SoundDampening = 0;
            size_t DamageScaling = 0;
            size_t AlarmTag = 0;
            size_t SharedAlarmTag = 0;
            size_t carriedDecoration = 0;
            size_t PlayerReStartState = 0;
            size_t MenuName = 0;
            size_t NameArticle = 0;
            size_t VoicePitch = 0;
            size_t VoiceType = 0;
            size_t OldMessageTime = 0;
            size_t RouteCache = 0;
            size_t PlayerReplicationInfoClass = 0;
            size_t PlayerReplicationInfo = 0;
            size_t Shadow = 0;
            size_t HealthHead = 0;
            size_t HealthTorso = 0;
            size_t HealthLegLeft = 0;
            size_t HealthLegRight = 0;
            size_t HealthArmLeft = 0;
            size_t HealthArmRight = 0;
            size_t bIsSpeaking = 0;
            size_t bWasSpeaking = 0;
            size_t lastPhoneme = 0;
            size_t nextPhoneme = 0;
            size_t animTimer = 0;
            size_t bOnFire = 0;
            size_t burnTimer = 0;
            size_t AIHorizontalFov = 0;
            size_t AspectRatio = 0;
            size_t AngularResolution = 0;
            size_t MinAngularSize = 0;
            size_t VisibilityThreshold = 0;
            size_t SmellThreshold = 0;
            size_t Alliance = 0;
            size_t AIAddViewRotation = 0;
        };
    }

    extern pawn::Offsets offsets_pawn;

    namespace pawn {
        inline uint32_t& bBehindView(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bBehindView);
        }

        inline uint32_t& bIsPlayer(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bIsPlayer);
        }

        inline uint32_t& bJustLanded(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bJustLanded);
        }

        inline uint32_t& bUpAndOut(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bUpAndOut);
        }

        inline uint32_t& bIsWalking(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bIsWalking);
        }

        inline uint32_t& bHitSlopedWall(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bHitSlopedWall);
        }

        inline uint32_t& bNeverSwitchOnPickup(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bNeverSwitchOnPickup);
        }

        inline uint32_t& bWarping(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bWarping);
        }

        inline uint32_t& bUpdatingDisplay(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bUpdatingDisplay);
        }

        inline uint32_t& bCanStrafe(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bCanStrafe);
        }

        inline uint32_t& bFixedStart(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bFixedStart);
        }

        inline uint32_t& bReducedSpeed(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bReducedSpeed);
        }

        inline uint32_t& bCanJump(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bCanJump);
        }

        inline uint32_t& bCanWalk(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bCanWalk);
        }

        inline uint32_t& bCanSwim(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bCanSwim);
        }

        inline uint32_t& bCanFly(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bCanFly);
        }

        inline uint32_t& bCanOpenDoors(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bCanOpenDoors);
        }

        inline uint32_t& bCanDoSpecial(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bCanDoSpecial);
        }

        inline uint32_t& bDrowning(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bDrowning);
        }

        inline uint32_t& bLOSflag(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bLOSflag);
        }

        inline uint32_t& bFromWall(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bFromWall);
        }

        inline uint32_t& bHunting(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bHunting);
        }

        inline uint32_t& bAvoidLedges(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bAvoidLedges);
        }

        inline uint32_t& bStopAtLedges(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bStopAtLedges);
        }

        inline uint32_t& bJumpOffPawn(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bJumpOffPawn);
        }

        inline uint32_t& bShootSpecial(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bShootSpecial);
        }

        inline uint32_t& bAutoActivate(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bAutoActivate);
        }

        inline uint32_t& bIsHuman(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bIsHuman);
        }

        inline uint32_t& bIsFemale(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bIsFemale);
        }

        inline uint32_t& bIsMultiSkinned(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bIsMultiSkinned);
        }

        inline uint32_t& bCountJumps(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bCountJumps);
        }

        inline uint32_t& bAdvancedTactics(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bAdvancedTactics);
        }

        inline uint32_t& bViewTarget(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bViewTarget);
        }

        inline uint32_t& bCanGlide(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bCanGlide);
        }

        inline float& SightCounter(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.SightCounter);
        }

        inline float& PainTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.PainTime);
        }

        inline float& SpeechTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.SpeechTime);
        }

        inline float& AvgPhysicsTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.AvgPhysicsTime);
        }

        inline uint8_t* FootRegion(UObject* o)
        {
            return o->data + offsets_pawn.FootRegion;
        }

        inline uint8_t* HeadRegion(UObject* o)
        {
            return o->data + offsets_pawn.HeadRegion;
        }

        inline float& MoveTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.MoveTimer);
        }

        inline UObject*& MoveTarget(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_pawn.MoveTarget);
        }

        inline UObject*& FaceTarget(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_pawn.FaceTarget);
        }

        inline Vector& Destination(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_pawn.Destination);
        }

        inline Vector& Focus(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_pawn.Focus);
        }

        inline float& DesiredSpeed(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.DesiredSpeed);
        }

        inline float& MaxDesiredSpeed(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.MaxDesiredSpeed);
        }

        inline float& MeleeRange(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.MeleeRange);
        }

        inline float& GroundSpeed(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.GroundSpeed);
        }

        inline float& WaterSpeed(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.WaterSpeed);
        }

        inline float& AirSpeed(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.AirSpeed);
        }

        inline float& AccelRate(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.AccelRate);
        }

        inline float& JumpZ(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.JumpZ);
        }

        inline float& MaxStepHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.MaxStepHeight);
        }

        inline float& AirControl(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.AirControl);
        }

        inline float& MinHitWall(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.MinHitWall);
        }

        inline uint8_t& Visibility(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_pawn.Visibility);
        }

        inline float& Alertness(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.Alertness);
        }

        inline float& Stimulus(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.Stimulus);
        }

        inline float& SightRadius(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.SightRadius);
        }

        inline float& PeripheralVision(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.PeripheralVision);
        }

        inline float& HearingThreshold(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.HearingThreshold);
        }

        inline Vector& LastSeenPos(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_pawn.LastSeenPos);
        }

        inline Vector& LastSeeingPos(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_pawn.LastSeeingPos);
        }

        inline float& LastSeenTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.LastSeenTime);
        }

        inline UObject*& Enemy(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_pawn.Enemy);
        }

        inline UObject*& Weapon(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_pawn.Weapon);
        }

        inline UObject*& PendingWeapon(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_pawn.PendingWeapon);
        }

        inline UObject*& SelectedItem(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_pawn.SelectedItem);
        }

        inline Rotator& ViewRotation(UObject* o)
        {
            return *reinterpret_cast<Rotator*>(o->data + offsets_pawn.ViewRotation);
        }

        inline Vector& WalkBob(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_pawn.WalkBob);
        }

        inline float& BaseEyeHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.BaseEyeHeight);
        }

        inline float& EyeHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.EyeHeight);
        }

        inline Vector& Floor(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_pawn.Floor);
        }

        inline float& SplashTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.SplashTime);
        }

        inline float& OrthoZoom(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.OrthoZoom);
        }

        inline float& FovAngle(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.FovAngle);
        }

        inline int32_t& DieCount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_pawn.DieCount);
        }

        inline int32_t& ItemCount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_pawn.ItemCount);
        }

        inline int32_t& KillCount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_pawn.KillCount);
        }

        inline int32_t& SecretCount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_pawn.SecretCount);
        }

        inline int32_t& Spree(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_pawn.Spree);
        }

        inline int32_t& Health(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_pawn.Health);
        }

        inline std::string& SelectionMesh(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_pawn.SelectionMesh);
        }

        inline std::string& SpecialMesh(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_pawn.SpecialMesh);
        }

        inline Name& ReducedDamageType(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_pawn.ReducedDamageType);
        }

        inline float& ReducedDamagePct(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.ReducedDamagePct);
        }

        inline UObject*& DropWhenKilled(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_pawn.DropWhenKilled);
        }

        inline float& UnderWaterTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.UnderWaterTime);
        }

        inline uint8_t& AttitudeToPlayer(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_pawn.AttitudeToPlayer);
        }

        inline uint8_t& Intelligence(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_pawn.Intelligence);
        }

        inline float& Skill(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.Skill);
        }

        inline UObject*& SpecialGoal(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_pawn.SpecialGoal);
        }

        inline float& SpecialPause(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.SpecialPause);
        }

        inline Vector& noise1spot(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_pawn.noise1spot);
        }

        inline float& noise1time(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.noise1time);
        }

        inline UObject*& noise1other(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_pawn.noise1other);
        }

        inline float& noise1loudness(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.noise1loudness);
        }

        inline Vector& noise2spot(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_pawn.noise2spot);
        }

        inline float& noise2time(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.noise2time);
        }

        inline UObject*& noise2other(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_pawn.noise2other);
        }

        inline float& noise2loudness(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.noise2loudness);
        }

        inline float& LastPainSound(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.LastPainSound);
        }

        inline UObject*& nextPawn(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_pawn.nextPawn);
        }

        inline UObject*& HitSound1(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_pawn.HitSound1);
        }

        inline UObject*& HitSound2(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_pawn.HitSound2);
        }

        inline UObject*& Land(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_pawn.Land);
        }

        inline UObject*& Die(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_pawn.Die);
        }

        inline UObject*& WaterStep(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_pawn.WaterStep);
        }

        inline uint8_t& bZoom(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_pawn.bZoom);
        }

        inline uint8_t& bRun(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_pawn.bRun);
        }

        inline uint8_t& bLook(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_pawn.bLook);
        }

        inline uint8_t& bDuck(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_pawn.bDuck);
        }

        inline uint8_t& bSnapLevel(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_pawn.bSnapLevel);
        }

        inline uint8_t& bStrafe(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_pawn.bStrafe);
        }

        inline uint8_t& bFire(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_pawn.bFire);
        }

        inline uint8_t& bAltFire(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_pawn.bAltFire);
        }

        inline uint8_t& bFreeLook(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_pawn.bFreeLook);
        }

        inline uint8_t& bExtra0(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_pawn.bExtra0);
        }

        inline uint8_t& bExtra1(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_pawn.bExtra1);
        }

        inline uint8_t& bExtra2(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_pawn.bExtra2);
        }

        inline uint8_t& bExtra3(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_pawn.bExtra3);
        }

        inline float& CombatStyle(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.CombatStyle);
        }

        inline UObject*& home(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_pawn.home);
        }

        inline Name& NextState(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_pawn.NextState);
        }

        inline Name& NextLabel(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_pawn.NextLabel);
        }

        inline float& SoundDampening(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.SoundDampening);
        }

        inline float& DamageScaling(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.DamageScaling);
        }

        inline Name& AlarmTag(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_pawn.AlarmTag);
        }

        inline Name& SharedAlarmTag(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_pawn.SharedAlarmTag);
        }

        inline UObject*& carriedDecoration(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_pawn.carriedDecoration);
        }

        inline Name& PlayerReStartState(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_pawn.PlayerReStartState);
        }

        inline std::string& MenuName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_pawn.MenuName);
        }

        inline std::string& NameArticle(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_pawn.NameArticle);
        }

        inline uint8_t& VoicePitch(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_pawn.VoicePitch);
        }

        inline std::string& VoiceType(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_pawn.VoiceType);
        }

        inline float& OldMessageTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.OldMessageTime);
        }

        inline UObject*& RouteCache(UObject* o, int index)
        {
            return reinterpret_cast<UObject**>(o->data + offsets_pawn.RouteCache)[index];
        }

        inline UObject*& PlayerReplicationInfoClass(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_pawn.PlayerReplicationInfoClass);
        }

        inline UObject*& PlayerReplicationInfo(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_pawn.PlayerReplicationInfo);
        }

        inline UObject*& Shadow(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_pawn.Shadow);
        }

        inline int32_t& HealthHead(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_pawn.HealthHead);
        }

        inline int32_t& HealthTorso(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_pawn.HealthTorso);
        }

        inline int32_t& HealthLegLeft(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_pawn.HealthLegLeft);
        }

        inline int32_t& HealthLegRight(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_pawn.HealthLegRight);
        }

        inline int32_t& HealthArmLeft(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_pawn.HealthArmLeft);
        }

        inline int32_t& HealthArmRight(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_pawn.HealthArmRight);
        }

        inline uint32_t& bIsSpeaking(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bIsSpeaking);
        }

        inline uint32_t& bWasSpeaking(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bWasSpeaking);
        }

        inline std::string& lastPhoneme(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_pawn.lastPhoneme);
        }

        inline std::string& nextPhoneme(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_pawn.nextPhoneme);
        }

        inline float& animTimer(UObject* o, int index)
        {
            return reinterpret_cast<float*>(o->data + offsets_pawn.animTimer)[index];
        }

        inline uint32_t& bOnFire(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pawn.bOnFire);
        }

        inline float& burnTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.burnTimer);
        }

        inline float& AIHorizontalFov(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.AIHorizontalFov);
        }

        inline float& AspectRatio(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.AspectRatio);
        }

        inline float& AngularResolution(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.AngularResolution);
        }

        inline float& MinAngularSize(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.MinAngularSize);
        }

        inline float& VisibilityThreshold(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.VisibilityThreshold);
        }

        inline float& SmellThreshold(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_pawn.SmellThreshold);
        }

        inline Name& Alliance(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_pawn.Alliance);
        }

        inline Rotator& AIAddViewRotation(UObject* o)
        {
            return *reinterpret_cast<Rotator*>(o->data + offsets_pawn.AIAddViewRotation);
        }

    }

    namespace playerpawn {
        struct Offsets {
            size_t Player = 0;
            size_t Password = 0;
            size_t DodgeClickTimer = 0;
            size_t DodgeClickTime = 0;
            size_t Bob = 0;
            size_t LandBob = 0;
            size_t AppliedBob = 0;
            size_t bobtime = 0;
            size_t ShowFlags = 0;
            size_t RendMap = 0;
            size_t Misc1 = 0;
            size_t Misc2 = 0;
            size_t ViewTarget = 0;
            size_t FlashScale = 0;
            size_t FlashFog = 0;
            size_t myHUD = 0;
            size_t Scoring = 0;
            size_t HUDType = 0;
            size_t ScoringType = 0;
            size_t DesiredFlashScale = 0;
            size_t ConstantGlowScale = 0;
            size_t InstantFlash = 0;
            size_t DesiredFlashFog = 0;
            size_t ConstantGlowFog = 0;
            size_t InstantFog = 0;
            size_t DesiredFOV = 0;
            size_t DefaultFOV = 0;
            size_t Song = 0;
            size_t SongSection = 0;
            size_t CdTrack = 0;
            size_t Transition = 0;
            size_t shaketimer = 0;
            size_t shakemag = 0;
            size_t shakevert = 0;
            size_t maxshake = 0;
            size_t verttimer = 0;
            size_t CarcassType = 0;
            size_t MyAutoAim = 0;
            size_t Handedness = 0;
            size_t JumpSound = 0;
            size_t bAdmin = 0;
            size_t bLookUpStairs = 0;
            size_t bSnapToLevel = 0;
            size_t bAlwaysMouseLook = 0;
            size_t bKeyboardLook = 0;
            size_t bWasForward = 0;
            size_t bWasBack = 0;
            size_t bWasLeft = 0;
            size_t bWasRight = 0;
            size_t bEdgeForward = 0;
            size_t bEdgeBack = 0;
            size_t bEdgeLeft = 0;
            size_t bEdgeRight = 0;
            size_t bIsCrouching = 0;
            size_t bShakeDir = 0;
            size_t bAnimTransition = 0;
            size_t bIsTurning = 0;
            size_t bFrozen = 0;
            size_t bBadConnectionAlert = 0;
            size_t bInvertMouse = 0;
            size_t bShowScores = 0;
            size_t bShowMenu = 0;
            size_t bSpecialMenu = 0;
            size_t bWokeUp = 0;
            size_t bPressedJump = 0;
            size_t bUpdatePosition = 0;
            size_t bDelayedCommand = 0;
            size_t bRising = 0;
            size_t bReducedVis = 0;
            size_t bCenterView = 0;
            size_t bMaxMouseSmoothing = 0;
            size_t bMouseZeroed = 0;
            size_t bReadyToPlay = 0;
            size_t bNoFlash = 0;
            size_t bNoVoices = 0;
            size_t bMessageBeep = 0;
            size_t bZooming = 0;
            size_t bSinglePlayer = 0;
            size_t bJustFired = 0;
            size_t bJustAltFired = 0;
            size_t bIsTyping = 0;
            size_t bFixedCamera = 0;
            size_t bNeverAutoSwitch = 0;
            size_t bJumpStatus = 0;
            size_t bUpdating = 0;
            size_t bCheatsEnabled = 0;
            size_t ZoomLevel = 0;
            size_t AdditionalViews = 0;
            size_t SpecialMenu = 0;
            size_t DelayedCommand = 0;
            size_t MouseSensitivity = 0;
            size_t WeaponPriority = 0;
            size_t SmoothMouseX = 0;
            size_t SmoothMouseY = 0;
            size_t BorrowedMouseX = 0;
            size_t BorrowedMouseY = 0;
            size_t MouseSmoothThreshold = 0;
            size_t MouseZeroTime = 0;
            size_t aBaseX = 0;
            size_t aBaseY = 0;
            size_t aBaseZ = 0;
            size_t aMouseX = 0;
            size_t aMouseY = 0;
            size_t aForward = 0;
            size_t aTurn = 0;
            size_t aStrafe = 0;
            size_t aUp = 0;
            size_t aLookUp = 0;
            size_t aExtra4 = 0;
            size_t aExtra3 = 0;
            size_t aExtra2 = 0;
            size_t aExtra1 = 0;
            size_t aExtra0 = 0;
            size_t SavedMoves = 0;
            size_t FreeMoves = 0;
            size_t PendingMove = 0;
            size_t CurrentTimeStamp = 0;
            size_t LastUpdateTime = 0;
            size_t ServerTimeStamp = 0;
            size_t TimeMargin = 0;
            size_t ClientUpdateTime = 0;
            size_t MaxTimeMargin = 0;
            size_t ProgressMessage = 0;
            size_t ProgressColor = 0;
            size_t ProgressTimeOut = 0;
            size_t QuickSaveString = 0;
            size_t NoPauseMessage = 0;
            size_t ViewingFrom = 0;
            size_t OwnCamera = 0;
            size_t FailedView = 0;
            size_t GameReplicationInfo = 0;
            size_t ngWorldSecret = 0;
            size_t ngSecretSet = 0;
            size_t TargetViewRotation = 0;
            size_t TargetEyeHeight = 0;
            size_t TargetWeaponViewOffset = 0;
            size_t DemoViewPitch = 0;
            size_t DemoViewYaw = 0;
            size_t LastPlaySound = 0;
            size_t TurnRateAdjuster = 0;
        };
    }

    extern playerpawn::Offsets offsets_playerpawn;

    namespace playerpawn {
        inline UObject*& Player(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_playerpawn.Player);
        }

        inline std::string& Password(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_playerpawn.Password);
        }

        inline float& DodgeClickTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.DodgeClickTimer);
        }

        inline float& DodgeClickTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.DodgeClickTime);
        }

        inline float& Bob(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.Bob);
        }

        inline float& LandBob(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.LandBob);
        }

        inline float& AppliedBob(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.AppliedBob);
        }

        inline float& bobtime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.bobtime);
        }

        inline int32_t& ShowFlags(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_playerpawn.ShowFlags);
        }

        inline int32_t& RendMap(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_playerpawn.RendMap);
        }

        inline int32_t& Misc1(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_playerpawn.Misc1);
        }

        inline int32_t& Misc2(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_playerpawn.Misc2);
        }

        inline UObject*& ViewTarget(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_playerpawn.ViewTarget);
        }

        inline Vector& FlashScale(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_playerpawn.FlashScale);
        }

        inline Vector& FlashFog(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_playerpawn.FlashFog);
        }

        inline UObject*& myHUD(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_playerpawn.myHUD);
        }

        inline UObject*& Scoring(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_playerpawn.Scoring);
        }

        inline UObject*& HUDType(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_playerpawn.HUDType);
        }

        inline UObject*& ScoringType(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_playerpawn.ScoringType);
        }

        inline float& DesiredFlashScale(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.DesiredFlashScale);
        }

        inline float& ConstantGlowScale(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.ConstantGlowScale);
        }

        inline float& InstantFlash(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.InstantFlash);
        }

        inline Vector& DesiredFlashFog(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_playerpawn.DesiredFlashFog);
        }

        inline Vector& ConstantGlowFog(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_playerpawn.ConstantGlowFog);
        }

        inline Vector& InstantFog(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_playerpawn.InstantFog);
        }

        inline float& DesiredFOV(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.DesiredFOV);
        }

        inline float& DefaultFOV(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.DefaultFOV);
        }

        inline UObject*& Song(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_playerpawn.Song);
        }

        inline uint8_t& SongSection(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_playerpawn.SongSection);
        }

        inline uint8_t& CdTrack(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_playerpawn.CdTrack);
        }

        inline uint8_t& Transition(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_playerpawn.Transition);
        }

        inline float& shaketimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.shaketimer);
        }

        inline int32_t& shakemag(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_playerpawn.shakemag);
        }

        inline float& shakevert(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.shakevert);
        }

        inline float& maxshake(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.maxshake);
        }

        inline float& verttimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.verttimer);
        }

        inline UObject*& CarcassType(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_playerpawn.CarcassType);
        }

        inline float& MyAutoAim(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.MyAutoAim);
        }

        inline float& Handedness(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.Handedness);
        }

        inline UObject*& JumpSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_playerpawn.JumpSound);
        }

        inline uint32_t& bAdmin(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bAdmin);
        }

        inline uint32_t& bLookUpStairs(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bLookUpStairs);
        }

        inline uint32_t& bSnapToLevel(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bSnapToLevel);
        }

        inline uint32_t& bAlwaysMouseLook(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bAlwaysMouseLook);
        }

        inline uint32_t& bKeyboardLook(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bKeyboardLook);
        }

        inline uint32_t& bWasForward(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bWasForward);
        }

        inline uint32_t& bWasBack(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bWasBack);
        }

        inline uint32_t& bWasLeft(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bWasLeft);
        }

        inline uint32_t& bWasRight(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bWasRight);
        }

        inline uint32_t& bEdgeForward(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bEdgeForward);
        }

        inline uint32_t& bEdgeBack(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bEdgeBack);
        }

        inline uint32_t& bEdgeLeft(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bEdgeLeft);
        }

        inline uint32_t& bEdgeRight(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bEdgeRight);
        }

        inline uint32_t& bIsCrouching(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bIsCrouching);
        }

        inline uint32_t& bShakeDir(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bShakeDir);
        }

        inline uint32_t& bAnimTransition(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bAnimTransition);
        }

        inline uint32_t& bIsTurning(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bIsTurning);
        }

        inline uint32_t& bFrozen(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bFrozen);
        }

        inline uint32_t& bBadConnectionAlert(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bBadConnectionAlert);
        }

        inline uint32_t& bInvertMouse(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bInvertMouse);
        }

        inline uint32_t& bShowScores(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bShowScores);
        }

        inline uint32_t& bShowMenu(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bShowMenu);
        }

        inline uint32_t& bSpecialMenu(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bSpecialMenu);
        }

        inline uint32_t& bWokeUp(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bWokeUp);
        }

        inline uint32_t& bPressedJump(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bPressedJump);
        }

        inline uint32_t& bUpdatePosition(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bUpdatePosition);
        }

        inline uint32_t& bDelayedCommand(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bDelayedCommand);
        }

        inline uint32_t& bRising(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bRising);
        }

        inline uint32_t& bReducedVis(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bReducedVis);
        }

        inline uint32_t& bCenterView(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bCenterView);
        }

        inline uint32_t& bMaxMouseSmoothing(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bMaxMouseSmoothing);
        }

        inline uint32_t& bMouseZeroed(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bMouseZeroed);
        }

        inline uint32_t& bReadyToPlay(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bReadyToPlay);
        }

        inline uint32_t& bNoFlash(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bNoFlash);
        }

        inline uint32_t& bNoVoices(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bNoVoices);
        }

        inline uint32_t& bMessageBeep(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bMessageBeep);
        }

        inline uint32_t& bZooming(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bZooming);
        }

        inline uint32_t& bSinglePlayer(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bSinglePlayer);
        }

        inline uint32_t& bJustFired(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bJustFired);
        }

        inline uint32_t& bJustAltFired(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bJustAltFired);
        }

        inline uint32_t& bIsTyping(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bIsTyping);
        }

        inline uint32_t& bFixedCamera(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bFixedCamera);
        }

        inline uint32_t& bNeverAutoSwitch(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bNeverAutoSwitch);
        }

        inline uint32_t& bJumpStatus(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bJumpStatus);
        }

        inline uint32_t& bUpdating(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bUpdating);
        }

        inline uint32_t& bCheatsEnabled(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.bCheatsEnabled);
        }

        inline float& ZoomLevel(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.ZoomLevel);
        }

        inline UObject*& AdditionalViews(UObject* o, int index)
        {
            return reinterpret_cast<UObject**>(o->data + offsets_playerpawn.AdditionalViews)[index];
        }

        inline UObject*& SpecialMenu(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_playerpawn.SpecialMenu);
        }

        inline std::string& DelayedCommand(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_playerpawn.DelayedCommand);
        }

        inline float& MouseSensitivity(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.MouseSensitivity);
        }

        inline Name& WeaponPriority(UObject* o, int index)
        {
            return reinterpret_cast<Name*>(o->data + offsets_playerpawn.WeaponPriority)[index];
        }

        inline float& SmoothMouseX(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.SmoothMouseX);
        }

        inline float& SmoothMouseY(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.SmoothMouseY);
        }

        inline float& BorrowedMouseX(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.BorrowedMouseX);
        }

        inline float& BorrowedMouseY(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.BorrowedMouseY);
        }

        inline float& MouseSmoothThreshold(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.MouseSmoothThreshold);
        }

        inline float& MouseZeroTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.MouseZeroTime);
        }

        inline float& aBaseX(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.aBaseX);
        }

        inline float& aBaseY(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.aBaseY);
        }

        inline float& aBaseZ(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.aBaseZ);
        }

        inline float& aMouseX(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.aMouseX);
        }

        inline float& aMouseY(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.aMouseY);
        }

        inline float& aForward(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.aForward);
        }

        inline float& aTurn(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.aTurn);
        }

        inline float& aStrafe(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.aStrafe);
        }

        inline float& aUp(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.aUp);
        }

        inline float& aLookUp(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.aLookUp);
        }

        inline float& aExtra4(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.aExtra4);
        }

        inline float& aExtra3(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.aExtra3);
        }

        inline float& aExtra2(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.aExtra2);
        }

        inline float& aExtra1(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.aExtra1);
        }

        inline float& aExtra0(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.aExtra0);
        }

        inline UObject*& SavedMoves(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_playerpawn.SavedMoves);
        }

        inline UObject*& FreeMoves(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_playerpawn.FreeMoves);
        }

        inline UObject*& PendingMove(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_playerpawn.PendingMove);
        }

        inline float& CurrentTimeStamp(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.CurrentTimeStamp);
        }

        inline float& LastUpdateTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.LastUpdateTime);
        }

        inline float& ServerTimeStamp(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.ServerTimeStamp);
        }

        inline float& TimeMargin(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.TimeMargin);
        }

        inline float& ClientUpdateTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.ClientUpdateTime);
        }

        inline float& MaxTimeMargin(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.MaxTimeMargin);
        }

        inline std::string& ProgressMessage(UObject* o, int index)
        {
            return reinterpret_cast<std::string*>(o->data + offsets_playerpawn.ProgressMessage)[index];
        }

        inline Color& ProgressColor(UObject* o, int index)
        {
            return reinterpret_cast<Color*>(o->data + offsets_playerpawn.ProgressColor)[index];
        }

        inline float& ProgressTimeOut(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.ProgressTimeOut);
        }

        inline std::string& QuickSaveString(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_playerpawn.QuickSaveString);
        }

        inline std::string& NoPauseMessage(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_playerpawn.NoPauseMessage);
        }

        inline std::string& ViewingFrom(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_playerpawn.ViewingFrom);
        }

        inline std::string& OwnCamera(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_playerpawn.OwnCamera);
        }

        inline std::string& FailedView(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_playerpawn.FailedView);
        }

        inline UObject*& GameReplicationInfo(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_playerpawn.GameReplicationInfo);
        }

        inline std::string& ngWorldSecret(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_playerpawn.ngWorldSecret);
        }

        inline uint32_t& ngSecretSet(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerpawn.ngSecretSet);
        }

        inline Rotator& TargetViewRotation(UObject* o)
        {
            return *reinterpret_cast<Rotator*>(o->data + offsets_playerpawn.TargetViewRotation);
        }

        inline float& TargetEyeHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.TargetEyeHeight);
        }

        inline Vector& TargetWeaponViewOffset(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_playerpawn.TargetWeaponViewOffset);
        }

        inline int32_t& DemoViewPitch(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_playerpawn.DemoViewPitch);
        }

        inline int32_t& DemoViewYaw(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_playerpawn.DemoViewYaw);
        }

        inline float& LastPlaySound(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.LastPlaySound);
        }

        inline float& TurnRateAdjuster(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerpawn.TurnRateAdjuster);
        }

    }

    namespace levelinfo {
        struct Offsets {
            size_t TimeDilation = 0;
            size_t TimeSeconds = 0;
            size_t Year = 0;
            size_t Month = 0;
            size_t Day = 0;
            size_t DayOfWeek = 0;
            size_t Hour = 0;
            size_t Minute = 0;
            size_t Second = 0;
            size_t Millisecond = 0;
            size_t Title = 0;
            size_t Author = 0;
            size_t IdealPlayerCount = 0;
            size_t RecommendedEnemies = 0;
            size_t RecommendedTeammates = 0;
            size_t LevelEnterText = 0;
            size_t LocalizedPkg = 0;
            size_t Pauser = 0;
            size_t Summary = 0;
            size_t bLonePlayer = 0;
            size_t bBegunPlay = 0;
            size_t bPlayersOnly = 0;
            size_t bHighDetailMode = 0;
            size_t bDropDetail = 0;
            size_t bAggressiveLOD = 0;
            size_t bStartup = 0;
            size_t bHumansOnly = 0;
            size_t bNoCheating = 0;
            size_t bAllowFOV = 0;
            size_t Song = 0;
            size_t SongSection = 0;
            size_t CdTrack = 0;
            size_t PlayerDoppler = 0;
            size_t Brightness = 0;
            size_t Screenshot = 0;
            size_t DefaultTexture = 0;
            size_t HubStackLevel = 0;
            size_t LevelAction = 0;
            size_t bNeverPrecache = 0;
            size_t NetMode = 0;
            size_t ComputerName = 0;
            size_t EngineVersion = 0;
            size_t MinNetVersion = 0;
            size_t DefaultGameType = 0;
            size_t Game = 0;
            size_t NavigationPointList = 0;
            size_t PawnList = 0;
            size_t EventManager = 0;
            size_t NextURL = 0;
            size_t bNextItems = 0;
            size_t NextSwitchCountdown = 0;
            size_t AIProfile = 0;
            size_t AvgAITime = 0;
            size_t bCheckWalkSurfaces = 0;
            size_t SpawnNotify = 0;
        };
    }

    extern levelinfo::Offsets offsets_levelinfo;

    namespace levelinfo {
        inline float& TimeDilation(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_levelinfo.TimeDilation);
        }

        inline float& TimeSeconds(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_levelinfo.TimeSeconds);
        }

        inline int32_t& Year(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_levelinfo.Year);
        }

        inline int32_t& Month(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_levelinfo.Month);
        }

        inline int32_t& Day(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_levelinfo.Day);
        }

        inline int32_t& DayOfWeek(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_levelinfo.DayOfWeek);
        }

        inline int32_t& Hour(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_levelinfo.Hour);
        }

        inline int32_t& Minute(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_levelinfo.Minute);
        }

        inline int32_t& Second(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_levelinfo.Second);
        }

        inline int32_t& Millisecond(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_levelinfo.Millisecond);
        }

        inline std::string& Title(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_levelinfo.Title);
        }

        inline std::string& Author(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_levelinfo.Author);
        }

        inline std::string& IdealPlayerCount(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_levelinfo.IdealPlayerCount);
        }

        inline int32_t& RecommendedEnemies(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_levelinfo.RecommendedEnemies);
        }

        inline int32_t& RecommendedTeammates(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_levelinfo.RecommendedTeammates);
        }

        inline std::string& LevelEnterText(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_levelinfo.LevelEnterText);
        }

        inline std::string& LocalizedPkg(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_levelinfo.LocalizedPkg);
        }

        inline std::string& Pauser(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_levelinfo.Pauser);
        }

        inline UObject*& Summary(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_levelinfo.Summary);
        }

        inline uint32_t& bLonePlayer(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_levelinfo.bLonePlayer);
        }

        inline uint32_t& bBegunPlay(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_levelinfo.bBegunPlay);
        }

        inline uint32_t& bPlayersOnly(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_levelinfo.bPlayersOnly);
        }

        inline uint32_t& bHighDetailMode(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_levelinfo.bHighDetailMode);
        }

        inline uint32_t& bDropDetail(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_levelinfo.bDropDetail);
        }

        inline uint32_t& bAggressiveLOD(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_levelinfo.bAggressiveLOD);
        }

        inline uint32_t& bStartup(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_levelinfo.bStartup);
        }

        inline uint32_t& bHumansOnly(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_levelinfo.bHumansOnly);
        }

        inline uint32_t& bNoCheating(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_levelinfo.bNoCheating);
        }

        inline uint32_t& bAllowFOV(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_levelinfo.bAllowFOV);
        }

        inline UObject*& Song(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_levelinfo.Song);
        }

        inline uint8_t& SongSection(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_levelinfo.SongSection);
        }

        inline uint8_t& CdTrack(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_levelinfo.CdTrack);
        }

        inline float& PlayerDoppler(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_levelinfo.PlayerDoppler);
        }

        inline float& Brightness(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_levelinfo.Brightness);
        }

        inline UObject*& Screenshot(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_levelinfo.Screenshot);
        }

        inline UObject*& DefaultTexture(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_levelinfo.DefaultTexture);
        }

        inline int32_t& HubStackLevel(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_levelinfo.HubStackLevel);
        }

        inline uint8_t& LevelAction(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_levelinfo.LevelAction);
        }

        inline uint32_t& bNeverPrecache(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_levelinfo.bNeverPrecache);
        }

        inline uint8_t& NetMode(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_levelinfo.NetMode);
        }

        inline std::string& ComputerName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_levelinfo.ComputerName);
        }

        inline std::string& EngineVersion(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_levelinfo.EngineVersion);
        }

        inline std::string& MinNetVersion(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_levelinfo.MinNetVersion);
        }

        inline UObject*& DefaultGameType(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_levelinfo.DefaultGameType);
        }

        inline UObject*& Game(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_levelinfo.Game);
        }

        inline UObject*& NavigationPointList(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_levelinfo.NavigationPointList);
        }

        inline UObject*& PawnList(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_levelinfo.PawnList);
        }

        inline UObject*& EventManager(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_levelinfo.EventManager);
        }

        inline std::string& NextURL(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_levelinfo.NextURL);
        }

        inline uint32_t& bNextItems(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_levelinfo.bNextItems);
        }

        inline float& NextSwitchCountdown(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_levelinfo.NextSwitchCountdown);
        }

        inline int32_t& AIProfile(UObject* o, int index)
        {
            return reinterpret_cast<int32_t*>(o->data + offsets_levelinfo.AIProfile)[index];
        }

        inline float& AvgAITime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_levelinfo.AvgAITime);
        }

        inline uint32_t& bCheckWalkSurfaces(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_levelinfo.bCheckWalkSurfaces);
        }

        inline UObject*& SpawnNotify(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_levelinfo.SpawnNotify);
        }

    }

    namespace zoneinfo {
        struct Offsets {
            size_t ZoneTag = 0;
            size_t ZoneGravity = 0;
            size_t ZoneVelocity = 0;
            size_t ZoneGroundFriction = 0;
            size_t ZoneFluidFriction = 0;
            size_t ZoneTerminalVelocity = 0;
            size_t ZonePlayerEvent = 0;
            size_t ZonePlayerCount = 0;
            size_t NumCarcasses = 0;
            size_t DamagePerSec = 0;
            size_t DamageType = 0;
            size_t DamageString = 0;
            size_t ZoneName = 0;
            size_t locationid = 0;
            size_t MaxCarcasses = 0;
            size_t EntrySound = 0;
            size_t ExitSound = 0;
            size_t EntryActor = 0;
            size_t ExitActor = 0;
            size_t SkyZone = 0;
            size_t bWaterZone = 0;
            size_t bFogZone = 0;
            size_t bKillZone = 0;
            size_t bNeutralZone = 0;
            size_t bGravityZone = 0;
            size_t bPainZone = 0;
            size_t bDestructive = 0;
            size_t bNoInventory = 0;
            size_t bMoveProjectiles = 0;
            size_t bBounceVelocity = 0;
            size_t AmbientBrightness = 0;
            size_t AmbientHue = 0;
            size_t AmbientSaturation = 0;
            size_t FogColor = 0;
            size_t FogDistance = 0;
            size_t EnvironmentMap = 0;
            size_t TexUPanSpeed = 0;
            size_t TexVPanSpeed = 0;
            size_t ViewFlash = 0;
            size_t ViewFog = 0;
            size_t bReverbZone = 0;
            size_t bRaytraceReverb = 0;
            size_t SpeedOfSound = 0;
            size_t MasterGain = 0;
            size_t CutoffHz = 0;
            size_t Delay = 0;
            size_t Gain = 0;
            size_t LensFlare = 0;
            size_t LensFlareOffset = 0;
            size_t LensFlareScale = 0;
            size_t MinLightCount = 0;
            size_t MaxLightCount = 0;
            size_t MinLightingPolyCount = 0;
            size_t MaxLightingPolyCount = 0;
        };
    }

    extern zoneinfo::Offsets offsets_zoneinfo;

    namespace zoneinfo {
        inline Name& ZoneTag(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_zoneinfo.ZoneTag);
        }

        inline Vector& ZoneGravity(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_zoneinfo.ZoneGravity);
        }

        inline Vector& ZoneVelocity(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_zoneinfo.ZoneVelocity);
        }

        inline float& ZoneGroundFriction(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_zoneinfo.ZoneGroundFriction);
        }

        inline float& ZoneFluidFriction(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_zoneinfo.ZoneFluidFriction);
        }

        inline float& ZoneTerminalVelocity(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_zoneinfo.ZoneTerminalVelocity);
        }

        inline Name& ZonePlayerEvent(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_zoneinfo.ZonePlayerEvent);
        }

        inline int32_t& ZonePlayerCount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_zoneinfo.ZonePlayerCount);
        }

        inline int32_t& NumCarcasses(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_zoneinfo.NumCarcasses);
        }

        inline int32_t& DamagePerSec(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_zoneinfo.DamagePerSec);
        }

        inline Name& DamageType(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_zoneinfo.DamageType);
        }

        inline std::string& DamageString(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_zoneinfo.DamageString);
        }

        inline std::string& ZoneName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_zoneinfo.ZoneName);
        }

        inline UObject*& locationid(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_zoneinfo.locationid);
        }

        inline int32_t& MaxCarcasses(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_zoneinfo.MaxCarcasses);
        }

        inline UObject*& EntrySound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_zoneinfo.EntrySound);
        }

        inline UObject*& ExitSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_zoneinfo.ExitSound);
        }

        inline UObject*& EntryActor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_zoneinfo.EntryActor);
        }

        inline UObject*& ExitActor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_zoneinfo.ExitActor);
        }

        inline UObject*& SkyZone(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_zoneinfo.SkyZone);
        }

        inline uint32_t& bWaterZone(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_zoneinfo.bWaterZone);
        }

        inline uint32_t& bFogZone(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_zoneinfo.bFogZone);
        }

        inline uint32_t& bKillZone(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_zoneinfo.bKillZone);
        }

        inline uint32_t& bNeutralZone(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_zoneinfo.bNeutralZone);
        }

        inline uint32_t& bGravityZone(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_zoneinfo.bGravityZone);
        }

        inline uint32_t& bPainZone(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_zoneinfo.bPainZone);
        }

        inline uint32_t& bDestructive(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_zoneinfo.bDestructive);
        }

        inline uint32_t& bNoInventory(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_zoneinfo.bNoInventory);
        }

        inline uint32_t& bMoveProjectiles(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_zoneinfo.bMoveProjectiles);
        }

        inline uint32_t& bBounceVelocity(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_zoneinfo.bBounceVelocity);
        }

        inline uint8_t& AmbientBrightness(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_zoneinfo.AmbientBrightness);
        }

        inline uint8_t& AmbientHue(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_zoneinfo.AmbientHue);
        }

        inline uint8_t& AmbientSaturation(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_zoneinfo.AmbientSaturation);
        }

        inline Color& FogColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_zoneinfo.FogColor);
        }

        inline float& FogDistance(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_zoneinfo.FogDistance);
        }

        inline UObject*& EnvironmentMap(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_zoneinfo.EnvironmentMap);
        }

        inline float& TexUPanSpeed(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_zoneinfo.TexUPanSpeed);
        }

        inline float& TexVPanSpeed(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_zoneinfo.TexVPanSpeed);
        }

        inline Vector& ViewFlash(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_zoneinfo.ViewFlash);
        }

        inline Vector& ViewFog(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_zoneinfo.ViewFog);
        }

        inline uint32_t& bReverbZone(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_zoneinfo.bReverbZone);
        }

        inline uint32_t& bRaytraceReverb(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_zoneinfo.bRaytraceReverb);
        }

        inline float& SpeedOfSound(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_zoneinfo.SpeedOfSound);
        }

        inline uint8_t& MasterGain(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_zoneinfo.MasterGain);
        }

        inline int32_t& CutoffHz(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_zoneinfo.CutoffHz);
        }

        inline uint8_t& Delay(UObject* o, int index)
        {
            return reinterpret_cast<uint8_t*>(o->data + offsets_zoneinfo.Delay)[index];
        }

        inline uint8_t& Gain(UObject* o, int index)
        {
            return reinterpret_cast<uint8_t*>(o->data + offsets_zoneinfo.Gain)[index];
        }

        inline UObject*& LensFlare(UObject* o, int index)
        {
            return reinterpret_cast<UObject**>(o->data + offsets_zoneinfo.LensFlare)[index];
        }

        inline float& LensFlareOffset(UObject* o, int index)
        {
            return reinterpret_cast<float*>(o->data + offsets_zoneinfo.LensFlareOffset)[index];
        }

        inline float& LensFlareScale(UObject* o, int index)
        {
            return reinterpret_cast<float*>(o->data + offsets_zoneinfo.LensFlareScale)[index];
        }

        inline uint8_t& MinLightCount(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_zoneinfo.MinLightCount);
        }

        inline uint8_t& MaxLightCount(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_zoneinfo.MaxLightCount);
        }

        inline int32_t& MinLightingPolyCount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_zoneinfo.MinLightingPolyCount);
        }

        inline int32_t& MaxLightingPolyCount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_zoneinfo.MaxLightingPolyCount);
        }

    }

    namespace skyzoneinfo {
        struct Offsets {
        };
    }

    extern skyzoneinfo::Offsets offsets_skyzoneinfo;

    namespace skyzoneinfo {

    }

    namespace warpzoneinfo {
        struct Offsets {
            size_t OtherSideURL = 0;
            size_t ThisTag = 0;
            size_t bNoTeleFrag = 0;
            size_t iWarpZone = 0;
            size_t WarpCoords = 0;
            size_t OtherSideActor = 0;
            size_t OtherSideLevel = 0;
            size_t Destinations = 0;
            size_t numDestinations = 0;
        };
    }

    extern warpzoneinfo::Offsets offsets_warpzoneinfo;

    namespace warpzoneinfo {
        inline std::string& OtherSideURL(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_warpzoneinfo.OtherSideURL);
        }

        inline Name& ThisTag(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_warpzoneinfo.ThisTag);
        }

        inline uint32_t& bNoTeleFrag(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_warpzoneinfo.bNoTeleFrag);
        }

        inline int32_t& iWarpZone(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_warpzoneinfo.iWarpZone);
        }

        inline Coords& WarpCoords(UObject* o)
        {
            return *reinterpret_cast<Coords*>(o->data + offsets_warpzoneinfo.WarpCoords);
        }

        inline UObject*& OtherSideActor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_warpzoneinfo.OtherSideActor);
        }

        inline UObject*& OtherSideLevel(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_warpzoneinfo.OtherSideLevel);
        }

        inline std::string& Destinations(UObject* o, int index)
        {
            return reinterpret_cast<std::string*>(o->data + offsets_warpzoneinfo.Destinations)[index];
        }

        inline int32_t& numDestinations(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_warpzoneinfo.numDestinations);
        }

    }

    namespace gameinfo {
        struct Offsets {
            size_t ItemGoals = 0;
            size_t KillGoals = 0;
            size_t SecretGoals = 0;
            size_t Difficulty = 0;
            size_t bNoMonsters = 0;
            size_t bMuteSpectators = 0;
            size_t bHumansOnly = 0;
            size_t bRestartLevel = 0;
            size_t bPauseable = 0;
            size_t bCoopWeaponMode = 0;
            size_t bClassicDeathMessages = 0;
            size_t bLowGore = 0;
            size_t bCanChangeSkin = 0;
            size_t bTeamGame = 0;
            size_t bVeryLowGore = 0;
            size_t bNoCheating = 0;
            size_t bAllowFOV = 0;
            size_t bDeathMatch = 0;
            size_t bGameEnded = 0;
            size_t bOverTime = 0;
            size_t bAlternateMode = 0;
            size_t bCanViewOthers = 0;
            size_t AutoAim = 0;
            size_t GameSpeed = 0;
            size_t StartTime = 0;
            size_t DefaultPlayerClass = 0;
            size_t DefaultWeapon = 0;
            size_t MaxSpectators = 0;
            size_t NumSpectators = 0;
            size_t AdminPassword = 0;
            size_t GamePassword = 0;
            size_t ScoreBoardType = 0;
            size_t GameMenuType = 0;
            size_t BotMenuType = 0;
            size_t RulesMenuType = 0;
            size_t SettingsMenuType = 0;
            size_t GameUMenuType = 0;
            size_t MultiplayerUMenuType = 0;
            size_t GameOptionsMenuType = 0;
            size_t HUDType = 0;
            size_t MapListType = 0;
            size_t MapPrefix = 0;
            size_t BeaconName = 0;
            size_t SpecialDamageString = 0;
            size_t SwitchLevelMessage = 0;
            size_t SentText = 0;
            size_t DefaultPlayerName = 0;
            size_t LeftMessage = 0;
            size_t FailedSpawnMessage = 0;
            size_t FailedPlaceMessage = 0;
            size_t FailedTeamMessage = 0;
            size_t NameChangedMessage = 0;
            size_t EnteredMessage = 0;
            size_t GameName = 0;
            size_t MaxedOutMessage = 0;
            size_t WrongPassword = 0;
            size_t NeedPassword = 0;
            size_t IPBanned = 0;
            size_t MaxPlayers = 0;
            size_t NumPlayers = 0;
            size_t CurrentID = 0;
            size_t IPPolicies = 0;
            size_t DeathMessageClass = 0;
            size_t DMMessageClass = 0;
            size_t MutatorClass = 0;
            size_t BaseMutator = 0;
            size_t DamageMutator = 0;
            size_t WaterZoneType = 0;
            size_t DefaultPlayerState = 0;
            size_t GameReplicationInfoClass = 0;
            size_t GameReplicationInfo = 0;
            size_t ServerLogName = 0;
            size_t LocalLog = 0;
            size_t WorldLog = 0;
            size_t bLocalLog = 0;
            size_t bWorldLog = 0;
            size_t bBatchLocal = 0;
            size_t bLoggingGame = 0;
            size_t LocalLogFileName = 0;
            size_t WorldLogFileName = 0;
            size_t StatLogClass = 0;
            size_t DemoBuild = 0;
            size_t DemoHasTuts = 0;
        };
    }

    extern gameinfo::Offsets offsets_gameinfo;

    namespace gameinfo {
        inline int32_t& ItemGoals(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_gameinfo.ItemGoals);
        }

        inline int32_t& KillGoals(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_gameinfo.KillGoals);
        }

        inline int32_t& SecretGoals(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_gameinfo.SecretGoals);
        }

        inline uint8_t& Difficulty(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_gameinfo.Difficulty);
        }

        inline uint32_t& bNoMonsters(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gameinfo.bNoMonsters);
        }

        inline uint32_t& bMuteSpectators(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gameinfo.bMuteSpectators);
        }

        inline uint32_t& bHumansOnly(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gameinfo.bHumansOnly);
        }

        inline uint32_t& bRestartLevel(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gameinfo.bRestartLevel);
        }

        inline uint32_t& bPauseable(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gameinfo.bPauseable);
        }

        inline uint32_t& bCoopWeaponMode(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gameinfo.bCoopWeaponMode);
        }

        inline uint32_t& bClassicDeathMessages(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gameinfo.bClassicDeathMessages);
        }

        inline uint32_t& bLowGore(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gameinfo.bLowGore);
        }

        inline uint32_t& bCanChangeSkin(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gameinfo.bCanChangeSkin);
        }

        inline uint32_t& bTeamGame(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gameinfo.bTeamGame);
        }

        inline uint32_t& bVeryLowGore(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gameinfo.bVeryLowGore);
        }

        inline uint32_t& bNoCheating(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gameinfo.bNoCheating);
        }

        inline uint32_t& bAllowFOV(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gameinfo.bAllowFOV);
        }

        inline uint32_t& bDeathMatch(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gameinfo.bDeathMatch);
        }

        inline uint32_t& bGameEnded(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gameinfo.bGameEnded);
        }

        inline uint32_t& bOverTime(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gameinfo.bOverTime);
        }

        inline uint32_t& bAlternateMode(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gameinfo.bAlternateMode);
        }

        inline uint32_t& bCanViewOthers(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gameinfo.bCanViewOthers);
        }

        inline float& AutoAim(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_gameinfo.AutoAim);
        }

        inline float& GameSpeed(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_gameinfo.GameSpeed);
        }

        inline float& StartTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_gameinfo.StartTime);
        }

        inline UObject*& DefaultPlayerClass(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_gameinfo.DefaultPlayerClass);
        }

        inline UObject*& DefaultWeapon(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_gameinfo.DefaultWeapon);
        }

        inline int32_t& MaxSpectators(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_gameinfo.MaxSpectators);
        }

        inline int32_t& NumSpectators(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_gameinfo.NumSpectators);
        }

        inline std::string& AdminPassword(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_gameinfo.AdminPassword);
        }

        inline std::string& GamePassword(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_gameinfo.GamePassword);
        }

        inline UObject*& ScoreBoardType(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_gameinfo.ScoreBoardType);
        }

        inline UObject*& GameMenuType(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_gameinfo.GameMenuType);
        }

        inline std::string& BotMenuType(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_gameinfo.BotMenuType);
        }

        inline std::string& RulesMenuType(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_gameinfo.RulesMenuType);
        }

        inline std::string& SettingsMenuType(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_gameinfo.SettingsMenuType);
        }

        inline std::string& GameUMenuType(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_gameinfo.GameUMenuType);
        }

        inline std::string& MultiplayerUMenuType(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_gameinfo.MultiplayerUMenuType);
        }

        inline std::string& GameOptionsMenuType(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_gameinfo.GameOptionsMenuType);
        }

        inline UObject*& HUDType(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_gameinfo.HUDType);
        }

        inline UObject*& MapListType(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_gameinfo.MapListType);
        }

        inline std::string& MapPrefix(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_gameinfo.MapPrefix);
        }

        inline std::string& BeaconName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_gameinfo.BeaconName);
        }

        inline std::string& SpecialDamageString(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_gameinfo.SpecialDamageString);
        }

        inline std::string& SwitchLevelMessage(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_gameinfo.SwitchLevelMessage);
        }

        inline int32_t& SentText(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_gameinfo.SentText);
        }

        inline std::string& DefaultPlayerName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_gameinfo.DefaultPlayerName);
        }

        inline std::string& LeftMessage(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_gameinfo.LeftMessage);
        }

        inline std::string& FailedSpawnMessage(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_gameinfo.FailedSpawnMessage);
        }

        inline std::string& FailedPlaceMessage(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_gameinfo.FailedPlaceMessage);
        }

        inline std::string& FailedTeamMessage(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_gameinfo.FailedTeamMessage);
        }

        inline std::string& NameChangedMessage(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_gameinfo.NameChangedMessage);
        }

        inline std::string& EnteredMessage(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_gameinfo.EnteredMessage);
        }

        inline std::string& GameName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_gameinfo.GameName);
        }

        inline std::string& MaxedOutMessage(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_gameinfo.MaxedOutMessage);
        }

        inline std::string& WrongPassword(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_gameinfo.WrongPassword);
        }

        inline std::string& NeedPassword(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_gameinfo.NeedPassword);
        }

        inline std::string& IPBanned(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_gameinfo.IPBanned);
        }

        inline int32_t& MaxPlayers(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_gameinfo.MaxPlayers);
        }

        inline int32_t& NumPlayers(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_gameinfo.NumPlayers);
        }

        inline int32_t& CurrentID(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_gameinfo.CurrentID);
        }

        inline std::string& IPPolicies(UObject* o, int index)
        {
            return reinterpret_cast<std::string*>(o->data + offsets_gameinfo.IPPolicies)[index];
        }

        inline UObject*& DeathMessageClass(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_gameinfo.DeathMessageClass);
        }

        inline UObject*& DMMessageClass(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_gameinfo.DMMessageClass);
        }

        inline UObject*& MutatorClass(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_gameinfo.MutatorClass);
        }

        inline UObject*& BaseMutator(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_gameinfo.BaseMutator);
        }

        inline UObject*& DamageMutator(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_gameinfo.DamageMutator);
        }

        inline UObject*& WaterZoneType(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_gameinfo.WaterZoneType);
        }

        inline Name& DefaultPlayerState(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_gameinfo.DefaultPlayerState);
        }

        inline UObject*& GameReplicationInfoClass(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_gameinfo.GameReplicationInfoClass);
        }

        inline UObject*& GameReplicationInfo(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_gameinfo.GameReplicationInfo);
        }

        inline std::string& ServerLogName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_gameinfo.ServerLogName);
        }

        inline UObject*& LocalLog(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_gameinfo.LocalLog);
        }

        inline UObject*& WorldLog(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_gameinfo.WorldLog);
        }

        inline uint32_t& bLocalLog(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gameinfo.bLocalLog);
        }

        inline uint32_t& bWorldLog(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gameinfo.bWorldLog);
        }

        inline uint32_t& bBatchLocal(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gameinfo.bBatchLocal);
        }

        inline uint32_t& bLoggingGame(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gameinfo.bLoggingGame);
        }

        inline std::string& LocalLogFileName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_gameinfo.LocalLogFileName);
        }

        inline std::string& WorldLogFileName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_gameinfo.WorldLogFileName);
        }

        inline UObject*& StatLogClass(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_gameinfo.StatLogClass);
        }

        inline int32_t& DemoBuild(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_gameinfo.DemoBuild);
        }

        inline int32_t& DemoHasTuts(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_gameinfo.DemoHasTuts);
        }

    }

    namespace brush {
        struct Offsets {
            size_t CsgOper = 0;
            size_t UnusedLightMesh = 0;
            size_t PostPivot = 0;
            size_t MainScale = 0;
            size_t PostScale = 0;
            size_t TempScale = 0;
            size_t BrushColor = 0;
            size_t PolyFlags = 0;
            size_t bColored = 0;
        };
    }

    extern brush::Offsets offsets_brush;

    namespace brush {
        inline uint8_t& CsgOper(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_brush.CsgOper);
        }

        inline UObject*& UnusedLightMesh(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_brush.UnusedLightMesh);
        }

        inline Vector& PostPivot(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_brush.PostPivot);
        }

        inline uint8_t* MainScale(UObject* o)
        {
            return o->data + offsets_brush.MainScale;
        }

        inline uint8_t* PostScale(UObject* o)
        {
            return o->data + offsets_brush.PostScale;
        }

        inline uint8_t* TempScale(UObject* o)
        {
            return o->data + offsets_brush.TempScale;
        }

        inline Color& BrushColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_brush.BrushColor);
        }

        inline int32_t& PolyFlags(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_brush.PolyFlags);
        }

        inline uint32_t& bColored(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_brush.bColored);
        }

    }

    namespace mover {
        struct Offsets {
            size_t MoverEncroachType = 0;
            size_t MoverGlideType = 0;
            size_t BumpType = 0;
            size_t KeyNum = 0;
            size_t PrevKeyNum = 0;
            size_t NumKeys = 0;
            size_t WorldRaytraceKey = 0;
            size_t BrushRaytraceKey = 0;
            size_t MoveTime = 0;
            size_t StayOpenTime = 0;
            size_t OtherTime = 0;
            size_t EncroachDamage = 0;
            size_t bTriggerOnceOnly = 0;
            size_t bSlave = 0;
            size_t bUseTriggered = 0;
            size_t bDamageTriggered = 0;
            size_t bDynamicLightMover = 0;
            size_t PlayerBumpEvent = 0;
            size_t BumpEvent = 0;
            size_t SavedTrigger = 0;
            size_t DamageThreshold = 0;
            size_t numTriggerEvents = 0;
            size_t Leader = 0;
            size_t Follower = 0;
            size_t ReturnGroup = 0;
            size_t DelayTime = 0;
            size_t OpeningSound = 0;
            size_t OpenedSound = 0;
            size_t ClosingSound = 0;
            size_t ClosedSound = 0;
            size_t MoveAmbientSound = 0;
            size_t KeyPos = 0;
            size_t KeyRot = 0;
            size_t BasePos = 0;
            size_t OldPos = 0;
            size_t OldPrePivot = 0;
            size_t SavedPos = 0;
            size_t BaseRot = 0;
            size_t OldRot = 0;
            size_t SavedRot = 0;
            size_t myMarker = 0;
            size_t TriggerActor = 0;
            size_t TriggerActor2 = 0;
            size_t WaitingPawn = 0;
            size_t bOpening = 0;
            size_t bDelaying = 0;
            size_t bClientPause = 0;
            size_t bPlayerOnly = 0;
            size_t RecommendedTrigger = 0;
            size_t SimOldPos = 0;
            size_t SimOldRotPitch = 0;
            size_t SimOldRotYaw = 0;
            size_t SimOldRotRoll = 0;
            size_t SimInterpolate = 0;
            size_t RealPosition = 0;
            size_t RealRotation = 0;
            size_t ClientUpdate = 0;
        };
    }

    extern mover::Offsets offsets_mover;

    namespace mover {
        inline uint8_t& MoverEncroachType(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_mover.MoverEncroachType);
        }

        inline uint8_t& MoverGlideType(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_mover.MoverGlideType);
        }

        inline uint8_t& BumpType(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_mover.BumpType);
        }

        inline uint8_t& KeyNum(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_mover.KeyNum);
        }

        inline uint8_t& PrevKeyNum(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_mover.PrevKeyNum);
        }

        inline uint8_t& NumKeys(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_mover.NumKeys);
        }

        inline uint8_t& WorldRaytraceKey(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_mover.WorldRaytraceKey);
        }

        inline uint8_t& BrushRaytraceKey(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_mover.BrushRaytraceKey);
        }

        inline float& MoveTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_mover.MoveTime);
        }

        inline float& StayOpenTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_mover.StayOpenTime);
        }

        inline float& OtherTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_mover.OtherTime);
        }

        inline int32_t& EncroachDamage(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_mover.EncroachDamage);
        }

        inline uint32_t& bTriggerOnceOnly(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_mover.bTriggerOnceOnly);
        }

        inline uint32_t& bSlave(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_mover.bSlave);
        }

        inline uint32_t& bUseTriggered(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_mover.bUseTriggered);
        }

        inline uint32_t& bDamageTriggered(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_mover.bDamageTriggered);
        }

        inline uint32_t& bDynamicLightMover(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_mover.bDynamicLightMover);
        }

        inline Name& PlayerBumpEvent(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_mover.PlayerBumpEvent);
        }

        inline Name& BumpEvent(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_mover.BumpEvent);
        }

        inline UObject*& SavedTrigger(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_mover.SavedTrigger);
        }

        inline float& DamageThreshold(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_mover.DamageThreshold);
        }

        inline int32_t& numTriggerEvents(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_mover.numTriggerEvents);
        }

        inline UObject*& Leader(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_mover.Leader);
        }

        inline UObject*& Follower(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_mover.Follower);
        }

        inline Name& ReturnGroup(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_mover.ReturnGroup);
        }

        inline float& DelayTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_mover.DelayTime);
        }

        inline UObject*& OpeningSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_mover.OpeningSound);
        }

        inline UObject*& OpenedSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_mover.OpenedSound);
        }

        inline UObject*& ClosingSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_mover.ClosingSound);
        }

        inline UObject*& ClosedSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_mover.ClosedSound);
        }

        inline UObject*& MoveAmbientSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_mover.MoveAmbientSound);
        }

        inline Vector& KeyPos(UObject* o, int index)
        {
            return reinterpret_cast<Vector*>(o->data + offsets_mover.KeyPos)[index];
        }

        inline Rotator& KeyRot(UObject* o, int index)
        {
            return reinterpret_cast<Rotator*>(o->data + offsets_mover.KeyRot)[index];
        }

        inline Vector& BasePos(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_mover.BasePos);
        }

        inline Vector& OldPos(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_mover.OldPos);
        }

        inline Vector& OldPrePivot(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_mover.OldPrePivot);
        }

        inline Vector& SavedPos(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_mover.SavedPos);
        }

        inline Rotator& BaseRot(UObject* o)
        {
            return *reinterpret_cast<Rotator*>(o->data + offsets_mover.BaseRot);
        }

        inline Rotator& OldRot(UObject* o)
        {
            return *reinterpret_cast<Rotator*>(o->data + offsets_mover.OldRot);
        }

        inline Rotator& SavedRot(UObject* o)
        {
            return *reinterpret_cast<Rotator*>(o->data + offsets_mover.SavedRot);
        }

        inline UObject*& myMarker(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_mover.myMarker);
        }

        inline UObject*& TriggerActor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_mover.TriggerActor);
        }

        inline UObject*& TriggerActor2(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_mover.TriggerActor2);
        }

        inline UObject*& WaitingPawn(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_mover.WaitingPawn);
        }

        inline uint32_t& bOpening(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_mover.bOpening);
        }

        inline uint32_t& bDelaying(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_mover.bDelaying);
        }

        inline uint32_t& bClientPause(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_mover.bClientPause);
        }

        inline uint32_t& bPlayerOnly(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_mover.bPlayerOnly);
        }

        inline UObject*& RecommendedTrigger(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_mover.RecommendedTrigger);
        }

        inline Vector& SimOldPos(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_mover.SimOldPos);
        }

        inline int32_t& SimOldRotPitch(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_mover.SimOldRotPitch);
        }

        inline int32_t& SimOldRotYaw(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_mover.SimOldRotYaw);
        }

        inline int32_t& SimOldRotRoll(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_mover.SimOldRotRoll);
        }

        inline Vector& SimInterpolate(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_mover.SimInterpolate);
        }

        inline Vector& RealPosition(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_mover.RealPosition);
        }

        inline Rotator& RealRotation(UObject* o)
        {
            return *reinterpret_cast<Rotator*>(o->data + offsets_mover.RealRotation);
        }

        inline int32_t& ClientUpdate(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_mover.ClientUpdate);
        }

    }

    namespace navigationpoint {
        struct Offsets {
            size_t ownerTeam = 0;
            size_t taken = 0;
            size_t upstreamPaths = 0;
            size_t Paths = 0;
            size_t PrunedPaths = 0;
            size_t VisNoReachPaths = 0;
            size_t visitedWeight = 0;
            size_t RouteCache = 0;
            size_t bestPathWeight = 0;
            size_t nextNavigationPoint = 0;
            size_t nextOrdered = 0;
            size_t prevOrdered = 0;
            size_t startPath = 0;
            size_t previousPath = 0;
            size_t cost = 0;
            size_t ExtraCost = 0;
            size_t bPlayerOnly = 0;
            size_t bEndPoint = 0;
            size_t bEndPointOnly = 0;
            size_t bSpecialCost = 0;
            size_t bOneWayPath = 0;
            size_t bNeverUseStrafing = 0;
        };
    }

    extern navigationpoint::Offsets offsets_navigationpoint;

    namespace navigationpoint {
        inline Name& ownerTeam(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_navigationpoint.ownerTeam);
        }

        inline uint32_t& taken(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_navigationpoint.taken);
        }

        inline int32_t& upstreamPaths(UObject* o, int index)
        {
            return reinterpret_cast<int32_t*>(o->data + offsets_navigationpoint.upstreamPaths)[index];
        }

        inline int32_t& Paths(UObject* o, int index)
        {
            return reinterpret_cast<int32_t*>(o->data + offsets_navigationpoint.Paths)[index];
        }

        inline int32_t& PrunedPaths(UObject* o, int index)
        {
            return reinterpret_cast<int32_t*>(o->data + offsets_navigationpoint.PrunedPaths)[index];
        }

        inline UObject*& VisNoReachPaths(UObject* o, int index)
        {
            return reinterpret_cast<UObject**>(o->data + offsets_navigationpoint.VisNoReachPaths)[index];
        }

        inline int32_t& visitedWeight(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_navigationpoint.visitedWeight);
        }

        inline UObject*& RouteCache(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_navigationpoint.RouteCache);
        }

        inline int32_t& bestPathWeight(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_navigationpoint.bestPathWeight);
        }

        inline UObject*& nextNavigationPoint(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_navigationpoint.nextNavigationPoint);
        }

        inline UObject*& nextOrdered(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_navigationpoint.nextOrdered);
        }

        inline UObject*& prevOrdered(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_navigationpoint.prevOrdered);
        }

        inline UObject*& startPath(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_navigationpoint.startPath);
        }

        inline UObject*& previousPath(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_navigationpoint.previousPath);
        }

        inline int32_t& cost(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_navigationpoint.cost);
        }

        inline int32_t& ExtraCost(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_navigationpoint.ExtraCost);
        }

        inline uint32_t& bPlayerOnly(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_navigationpoint.bPlayerOnly);
        }

        inline uint32_t& bEndPoint(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_navigationpoint.bEndPoint);
        }

        inline uint32_t& bEndPointOnly(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_navigationpoint.bEndPointOnly);
        }

        inline uint32_t& bSpecialCost(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_navigationpoint.bSpecialCost);
        }

        inline uint32_t& bOneWayPath(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_navigationpoint.bOneWayPath);
        }

        inline uint32_t& bNeverUseStrafing(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_navigationpoint.bNeverUseStrafing);
        }

    }

    namespace inventory {
        struct Offsets {
            size_t AutoSwitchPriority = 0;
            size_t InventoryGroup = 0;
            size_t bActivatable = 0;
            size_t bDisplayableInv = 0;
            size_t bActive = 0;
            size_t bSleepTouch = 0;
            size_t bHeldItem = 0;
            size_t bTossedOut = 0;
            size_t bAmbientGlow = 0;
            size_t bInstantRespawn = 0;
            size_t bRotatingPickup = 0;
            size_t PickupMessage = 0;
            size_t ItemName = 0;
            size_t ItemArticle = 0;
            size_t RespawnTime = 0;
            size_t PlayerLastTouched = 0;
            size_t PlayerViewOffset = 0;
            size_t PlayerViewMesh = 0;
            size_t PlayerViewScale = 0;
            size_t BobDamping = 0;
            size_t PickupViewMesh = 0;
            size_t PickupViewScale = 0;
            size_t ThirdPersonMesh = 0;
            size_t ThirdPersonScale = 0;
            size_t StatusIcon = 0;
            size_t ProtectionType1 = 0;
            size_t ProtectionType2 = 0;
            size_t Charge = 0;
            size_t ArmorAbsorption = 0;
            size_t bIsAnArmor = 0;
            size_t AbsorptionPriority = 0;
            size_t NextArmor = 0;
            size_t MaxDesireability = 0;
            size_t myMarker = 0;
            size_t bSteadyFlash3rd = 0;
            size_t bFirstFrame = 0;
            size_t bMuzzleFlashParticles = 0;
            size_t bToggleSteadyFlash = 0;
            size_t bSteadyToggle = 0;
            size_t FlashCount = 0;
            size_t OldFlashCount = 0;
            size_t MuzzleFlashStyle = 0;
            size_t MuzzleFlashMesh = 0;
            size_t MuzzleFlashScale = 0;
            size_t MuzzleFlashTexture = 0;
            size_t PickupSound = 0;
            size_t ActivateSound = 0;
            size_t DeActivateSound = 0;
            size_t RespawnSound = 0;
            size_t LandSound = 0;
            size_t Icon = 0;
            size_t M_Activated = 0;
            size_t M_Selected = 0;
            size_t M_Deactivated = 0;
            size_t PickupMessageClass = 0;
            size_t ItemMessageClass = 0;
            size_t bCanUseObjectBelt = 0;
            size_t largeIcon = 0;
            size_t largeIconWidth = 0;
            size_t largeIconHeight = 0;
            size_t invSlotsX = 0;
            size_t invSlotsY = 0;
            size_t invPosX = 0;
            size_t invPosY = 0;
            size_t bInObjectBelt = 0;
            size_t beltPos = 0;
            size_t Description = 0;
            size_t beltDescription = 0;
        };
    }

    extern inventory::Offsets offsets_inventory;

    namespace inventory {
        inline uint8_t& AutoSwitchPriority(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_inventory.AutoSwitchPriority);
        }

        inline uint8_t& InventoryGroup(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_inventory.InventoryGroup);
        }

        inline uint32_t& bActivatable(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_inventory.bActivatable);
        }

        inline uint32_t& bDisplayableInv(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_inventory.bDisplayableInv);
        }

        inline uint32_t& bActive(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_inventory.bActive);
        }

        inline uint32_t& bSleepTouch(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_inventory.bSleepTouch);
        }

        inline uint32_t& bHeldItem(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_inventory.bHeldItem);
        }

        inline uint32_t& bTossedOut(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_inventory.bTossedOut);
        }

        inline uint32_t& bAmbientGlow(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_inventory.bAmbientGlow);
        }

        inline uint32_t& bInstantRespawn(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_inventory.bInstantRespawn);
        }

        inline uint32_t& bRotatingPickup(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_inventory.bRotatingPickup);
        }

        inline std::string& PickupMessage(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_inventory.PickupMessage);
        }

        inline std::string& ItemName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_inventory.ItemName);
        }

        inline std::string& ItemArticle(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_inventory.ItemArticle);
        }

        inline float& RespawnTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_inventory.RespawnTime);
        }

        inline Name& PlayerLastTouched(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_inventory.PlayerLastTouched);
        }

        inline Vector& PlayerViewOffset(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_inventory.PlayerViewOffset);
        }

        inline UObject*& PlayerViewMesh(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_inventory.PlayerViewMesh);
        }

        inline float& PlayerViewScale(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_inventory.PlayerViewScale);
        }

        inline float& BobDamping(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_inventory.BobDamping);
        }

        inline UObject*& PickupViewMesh(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_inventory.PickupViewMesh);
        }

        inline float& PickupViewScale(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_inventory.PickupViewScale);
        }

        inline UObject*& ThirdPersonMesh(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_inventory.ThirdPersonMesh);
        }

        inline float& ThirdPersonScale(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_inventory.ThirdPersonScale);
        }

        inline UObject*& StatusIcon(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_inventory.StatusIcon);
        }

        inline Name& ProtectionType1(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_inventory.ProtectionType1);
        }

        inline Name& ProtectionType2(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_inventory.ProtectionType2);
        }

        inline int32_t& Charge(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_inventory.Charge);
        }

        inline int32_t& ArmorAbsorption(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_inventory.ArmorAbsorption);
        }

        inline uint32_t& bIsAnArmor(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_inventory.bIsAnArmor);
        }

        inline int32_t& AbsorptionPriority(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_inventory.AbsorptionPriority);
        }

        inline UObject*& NextArmor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_inventory.NextArmor);
        }

        inline float& MaxDesireability(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_inventory.MaxDesireability);
        }

        inline UObject*& myMarker(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_inventory.myMarker);
        }

        inline uint32_t& bSteadyFlash3rd(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_inventory.bSteadyFlash3rd);
        }

        inline uint32_t& bFirstFrame(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_inventory.bFirstFrame);
        }

        inline uint32_t& bMuzzleFlashParticles(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_inventory.bMuzzleFlashParticles);
        }

        inline uint32_t& bToggleSteadyFlash(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_inventory.bToggleSteadyFlash);
        }

        inline uint32_t& bSteadyToggle(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_inventory.bSteadyToggle);
        }

        inline uint8_t& FlashCount(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_inventory.FlashCount);
        }

        inline uint8_t& OldFlashCount(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_inventory.OldFlashCount);
        }

        inline uint8_t& MuzzleFlashStyle(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_inventory.MuzzleFlashStyle);
        }

        inline UObject*& MuzzleFlashMesh(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_inventory.MuzzleFlashMesh);
        }

        inline float& MuzzleFlashScale(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_inventory.MuzzleFlashScale);
        }

        inline UObject*& MuzzleFlashTexture(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_inventory.MuzzleFlashTexture);
        }

        inline UObject*& PickupSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_inventory.PickupSound);
        }

        inline UObject*& ActivateSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_inventory.ActivateSound);
        }

        inline UObject*& DeActivateSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_inventory.DeActivateSound);
        }

        inline UObject*& RespawnSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_inventory.RespawnSound);
        }

        inline UObject*& LandSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_inventory.LandSound);
        }

        inline UObject*& Icon(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_inventory.Icon);
        }

        inline std::string& M_Activated(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_inventory.M_Activated);
        }

        inline std::string& M_Selected(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_inventory.M_Selected);
        }

        inline std::string& M_Deactivated(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_inventory.M_Deactivated);
        }

        inline UObject*& PickupMessageClass(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_inventory.PickupMessageClass);
        }

        inline UObject*& ItemMessageClass(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_inventory.ItemMessageClass);
        }

        inline uint32_t& bCanUseObjectBelt(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_inventory.bCanUseObjectBelt);
        }

        inline UObject*& largeIcon(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_inventory.largeIcon);
        }

        inline int32_t& largeIconWidth(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_inventory.largeIconWidth);
        }

        inline int32_t& largeIconHeight(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_inventory.largeIconHeight);
        }

        inline int32_t& invSlotsX(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_inventory.invSlotsX);
        }

        inline int32_t& invSlotsY(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_inventory.invSlotsY);
        }

        inline int32_t& invPosX(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_inventory.invPosX);
        }

        inline int32_t& invPosY(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_inventory.invPosY);
        }

        inline uint32_t& bInObjectBelt(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_inventory.bInObjectBelt);
        }

        inline int32_t& beltPos(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_inventory.beltPos);
        }

        inline std::string& Description(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_inventory.Description);
        }

        inline std::string& beltDescription(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_inventory.beltDescription);
        }

    }

    namespace weapon {
        struct Offsets {
            size_t MaxTargetRange = 0;
            size_t AmmoName = 0;
            size_t ReloadCount = 0;
            size_t PickupAmmoCount = 0;
            size_t AmmoType = 0;
            size_t bPointing = 0;
            size_t bInstantHit = 0;
            size_t bAltInstantHit = 0;
            size_t bWarnTarget = 0;
            size_t bAltWarnTarget = 0;
            size_t bWeaponUp = 0;
            size_t bChangeWeapon = 0;
            size_t bLockedOn = 0;
            size_t bSplashDamage = 0;
            size_t bCanThrow = 0;
            size_t bRecommendSplashDamage = 0;
            size_t bRecommendAltSplashDamage = 0;
            size_t bWeaponStay = 0;
            size_t bOwnsCrosshair = 0;
            size_t bHideWeapon = 0;
            size_t bMeleeWeapon = 0;
            size_t bRapidFire = 0;
            size_t bSpecialIcon = 0;
            size_t FiringSpeed = 0;
            size_t FireOffset = 0;
            size_t ProjectileClass = 0;
            size_t AltProjectileClass = 0;
            size_t MyDamageType = 0;
            size_t AltDamageType = 0;
            size_t ProjectileSpeed = 0;
            size_t AltProjectileSpeed = 0;
            size_t aimerror = 0;
            size_t shakemag = 0;
            size_t shaketime = 0;
            size_t shakevert = 0;
            size_t AIRating = 0;
            size_t RefireRate = 0;
            size_t AltRefireRate = 0;
            size_t FireSound = 0;
            size_t AltFireSound = 0;
            size_t CockingSound = 0;
            size_t SelectSound = 0;
            size_t Misc1Sound = 0;
            size_t Misc2Sound = 0;
            size_t Misc3Sound = 0;
            size_t MessageNoAmmo = 0;
            size_t DeathMessage = 0;
            size_t NameColor = 0;
            size_t AdjustedAim = 0;
            size_t bSetFlashTime = 0;
            size_t bDrawMuzzleFlash = 0;
            size_t bMuzzleFlash = 0;
            size_t FlashTime = 0;
            size_t MuzzleScale = 0;
            size_t FlashY = 0;
            size_t FlashO = 0;
            size_t FlashC = 0;
            size_t FlashLength = 0;
            size_t FlashS = 0;
            size_t MFTexture = 0;
            size_t MuzzleFlare = 0;
            size_t FlareOffset = 0;
        };
    }

    extern weapon::Offsets offsets_weapon;

    namespace weapon {
        inline float& MaxTargetRange(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_weapon.MaxTargetRange);
        }

        inline UObject*& AmmoName(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_weapon.AmmoName);
        }

        inline uint8_t& ReloadCount(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_weapon.ReloadCount);
        }

        inline int32_t& PickupAmmoCount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_weapon.PickupAmmoCount);
        }

        inline UObject*& AmmoType(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_weapon.AmmoType);
        }

        inline uint32_t& bPointing(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_weapon.bPointing);
        }

        inline uint32_t& bInstantHit(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_weapon.bInstantHit);
        }

        inline uint32_t& bAltInstantHit(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_weapon.bAltInstantHit);
        }

        inline uint32_t& bWarnTarget(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_weapon.bWarnTarget);
        }

        inline uint32_t& bAltWarnTarget(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_weapon.bAltWarnTarget);
        }

        inline uint32_t& bWeaponUp(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_weapon.bWeaponUp);
        }

        inline uint32_t& bChangeWeapon(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_weapon.bChangeWeapon);
        }

        inline uint32_t& bLockedOn(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_weapon.bLockedOn);
        }

        inline uint32_t& bSplashDamage(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_weapon.bSplashDamage);
        }

        inline uint32_t& bCanThrow(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_weapon.bCanThrow);
        }

        inline uint32_t& bRecommendSplashDamage(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_weapon.bRecommendSplashDamage);
        }

        inline uint32_t& bRecommendAltSplashDamage(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_weapon.bRecommendAltSplashDamage);
        }

        inline uint32_t& bWeaponStay(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_weapon.bWeaponStay);
        }

        inline uint32_t& bOwnsCrosshair(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_weapon.bOwnsCrosshair);
        }

        inline uint32_t& bHideWeapon(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_weapon.bHideWeapon);
        }

        inline uint32_t& bMeleeWeapon(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_weapon.bMeleeWeapon);
        }

        inline uint32_t& bRapidFire(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_weapon.bRapidFire);
        }

        inline uint32_t& bSpecialIcon(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_weapon.bSpecialIcon);
        }

        inline float& FiringSpeed(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_weapon.FiringSpeed);
        }

        inline Vector& FireOffset(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_weapon.FireOffset);
        }

        inline UObject*& ProjectileClass(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_weapon.ProjectileClass);
        }

        inline UObject*& AltProjectileClass(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_weapon.AltProjectileClass);
        }

        inline Name& MyDamageType(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_weapon.MyDamageType);
        }

        inline Name& AltDamageType(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_weapon.AltDamageType);
        }

        inline float& ProjectileSpeed(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_weapon.ProjectileSpeed);
        }

        inline float& AltProjectileSpeed(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_weapon.AltProjectileSpeed);
        }

        inline float& aimerror(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_weapon.aimerror);
        }

        inline float& shakemag(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_weapon.shakemag);
        }

        inline float& shaketime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_weapon.shaketime);
        }

        inline float& shakevert(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_weapon.shakevert);
        }

        inline float& AIRating(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_weapon.AIRating);
        }

        inline float& RefireRate(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_weapon.RefireRate);
        }

        inline float& AltRefireRate(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_weapon.AltRefireRate);
        }

        inline UObject*& FireSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_weapon.FireSound);
        }

        inline UObject*& AltFireSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_weapon.AltFireSound);
        }

        inline UObject*& CockingSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_weapon.CockingSound);
        }

        inline UObject*& SelectSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_weapon.SelectSound);
        }

        inline UObject*& Misc1Sound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_weapon.Misc1Sound);
        }

        inline UObject*& Misc2Sound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_weapon.Misc2Sound);
        }

        inline UObject*& Misc3Sound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_weapon.Misc3Sound);
        }

        inline std::string& MessageNoAmmo(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_weapon.MessageNoAmmo);
        }

        inline std::string& DeathMessage(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_weapon.DeathMessage);
        }

        inline Color& NameColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_weapon.NameColor);
        }

        inline Rotator& AdjustedAim(UObject* o)
        {
            return *reinterpret_cast<Rotator*>(o->data + offsets_weapon.AdjustedAim);
        }

        inline uint32_t& bSetFlashTime(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_weapon.bSetFlashTime);
        }

        inline uint32_t& bDrawMuzzleFlash(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_weapon.bDrawMuzzleFlash);
        }

        inline uint8_t& bMuzzleFlash(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_weapon.bMuzzleFlash);
        }

        inline float& FlashTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_weapon.FlashTime);
        }

        inline float& MuzzleScale(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_weapon.MuzzleScale);
        }

        inline float& FlashY(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_weapon.FlashY);
        }

        inline float& FlashO(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_weapon.FlashO);
        }

        inline float& FlashC(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_weapon.FlashC);
        }

        inline float& FlashLength(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_weapon.FlashLength);
        }

        inline int32_t& FlashS(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_weapon.FlashS);
        }

        inline UObject*& MFTexture(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_weapon.MFTexture);
        }

        inline UObject*& MuzzleFlare(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_weapon.MuzzleFlare);
        }

        inline float& FlareOffset(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_weapon.FlareOffset);
        }

    }

    namespace projectile {
        struct Offsets {
            size_t speed = 0;
            size_t MaxSpeed = 0;
            size_t Damage = 0;
            size_t MomentumTransfer = 0;
            size_t MyDamageType = 0;
            size_t SpawnSound = 0;
            size_t ImpactSound = 0;
            size_t MiscSound = 0;
            size_t ExploWallOut = 0;
            size_t ExplosionDecal = 0;
        };
    }

    extern projectile::Offsets offsets_projectile;

    namespace projectile {
        inline float& speed(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_projectile.speed);
        }

        inline float& MaxSpeed(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_projectile.MaxSpeed);
        }

        inline float& Damage(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_projectile.Damage);
        }

        inline int32_t& MomentumTransfer(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_projectile.MomentumTransfer);
        }

        inline Name& MyDamageType(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_projectile.MyDamageType);
        }

        inline UObject*& SpawnSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_projectile.SpawnSound);
        }

        inline UObject*& ImpactSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_projectile.ImpactSound);
        }

        inline UObject*& MiscSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_projectile.MiscSound);
        }

        inline float& ExploWallOut(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_projectile.ExploWallOut);
        }

        inline UObject*& ExplosionDecal(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_projectile.ExplosionDecal);
        }

    }

    namespace decoration {
        struct Offsets {
            size_t EffectWhenDestroyed = 0;
            size_t bPushable = 0;
            size_t bOnlyTriggerable = 0;
            size_t bSplash = 0;
            size_t bBobbing = 0;
            size_t bWasCarried = 0;
            size_t PushSound = 0;
            size_t numLandings = 0;
            size_t contents = 0;
            size_t content2 = 0;
            size_t content3 = 0;
            size_t EndPushSound = 0;
            size_t bPushSoundPlaying = 0;
            size_t BaseEyeHeight = 0;
        };
    }

    extern decoration::Offsets offsets_decoration;

    namespace decoration {
        inline UObject*& EffectWhenDestroyed(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_decoration.EffectWhenDestroyed);
        }

        inline uint32_t& bPushable(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_decoration.bPushable);
        }

        inline uint32_t& bOnlyTriggerable(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_decoration.bOnlyTriggerable);
        }

        inline uint32_t& bSplash(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_decoration.bSplash);
        }

        inline uint32_t& bBobbing(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_decoration.bBobbing);
        }

        inline uint32_t& bWasCarried(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_decoration.bWasCarried);
        }

        inline UObject*& PushSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_decoration.PushSound);
        }

        inline int32_t& numLandings(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_decoration.numLandings);
        }

        inline UObject*& contents(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_decoration.contents);
        }

        inline UObject*& content2(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_decoration.content2);
        }

        inline UObject*& content3(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_decoration.content3);
        }

        inline UObject*& EndPushSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_decoration.EndPushSound);
        }

        inline uint32_t& bPushSoundPlaying(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_decoration.bPushSoundPlaying);
        }

        inline float& BaseEyeHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_decoration.BaseEyeHeight);
        }

    }

    namespace canvas {
        struct Offsets {
            size_t Font = 0;
            size_t SpaceX = 0;
            size_t SpaceY = 0;
            size_t OrgX = 0;
            size_t OrgY = 0;
            size_t ClipX = 0;
            size_t ClipY = 0;
            size_t CurX = 0;
            size_t CurY = 0;
            size_t Z = 0;
            size_t Style = 0;
            size_t CurYL = 0;
            size_t DrawColor = 0;
            size_t bCenter = 0;
            size_t bNoSmooth = 0;
            size_t SizeX = 0;
            size_t SizeY = 0;
            size_t SmallFont = 0;
            size_t MedFont = 0;
            size_t BigFont = 0;
            size_t LargeFont = 0;
            size_t Viewport = 0;
            size_t FramePtr = 0;
            size_t RenderPtr = 0;
        };
    }

    extern canvas::Offsets offsets_canvas;

    namespace canvas {
        inline UObject*& Font(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_canvas.Font);
        }

        inline float& SpaceX(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_canvas.SpaceX);
        }

        inline float& SpaceY(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_canvas.SpaceY);
        }

        inline float& OrgX(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_canvas.OrgX);
        }

        inline float& OrgY(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_canvas.OrgY);
        }

        inline float& ClipX(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_canvas.ClipX);
        }

        inline float& ClipY(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_canvas.ClipY);
        }

        inline float& CurX(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_canvas.CurX);
        }

        inline float& CurY(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_canvas.CurY);
        }

        inline float& Z(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_canvas.Z);
        }

        inline uint8_t& Style(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_canvas.Style);
        }

        inline float& CurYL(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_canvas.CurYL);
        }

        inline Color& DrawColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_canvas.DrawColor);
        }

        inline uint32_t& bCenter(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_canvas.bCenter);
        }

        inline uint32_t& bNoSmooth(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_canvas.bNoSmooth);
        }

        inline int32_t& SizeX(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_canvas.SizeX);
        }

        inline int32_t& SizeY(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_canvas.SizeY);
        }

        inline UObject*& SmallFont(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_canvas.SmallFont);
        }

        inline UObject*& MedFont(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_canvas.MedFont);
        }

        inline UObject*& BigFont(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_canvas.BigFont);
        }

        inline UObject*& LargeFont(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_canvas.LargeFont);
        }

        inline UObject*& Viewport(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_canvas.Viewport);
        }

        inline int32_t& FramePtr(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_canvas.FramePtr);
        }

        inline int32_t& RenderPtr(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_canvas.RenderPtr);
        }

    }

    namespace console {
        struct Offsets {
            size_t vtblOut = 0;
            size_t Viewport = 0;
            size_t HistoryTop = 0;
            size_t HistoryBot = 0;
            size_t HistoryCur = 0;
            size_t TypedStr = 0;
            size_t History = 0;
            size_t Scrollback = 0;
            size_t numLines = 0;
            size_t TopLine = 0;
            size_t TextLines = 0;
            size_t MsgTime = 0;
            size_t MsgTickTime = 0;
            size_t MsgText = 0;
            size_t MsgType = 0;
            size_t MsgPlayer = 0;
            size_t MsgTick = 0;
            size_t BorderSize = 0;
            size_t ConsoleLines = 0;
            size_t BorderLines = 0;
            size_t BorderPixels = 0;
            size_t ConsolePos = 0;
            size_t ConsoleDest = 0;
            size_t FrameX = 0;
            size_t FrameY = 0;
            size_t ConBackground = 0;
            size_t Border = 0;
            size_t bNoStuff = 0;
            size_t bTyping = 0;
            size_t bNoDrawWorld = 0;
            size_t bTimeDemo = 0;
            size_t bStartTimeDemo = 0;
            size_t bRestartTimeDemo = 0;
            size_t bSaveTimeDemoToFile = 0;
            size_t StartTime = 0;
            size_t ExtraTime = 0;
            size_t LastFrameTime = 0;
            size_t LastSecondStartTime = 0;
            size_t FrameCount = 0;
            size_t LastSecondFrameCount = 0;
            size_t MinFPS = 0;
            size_t MaxFPS = 0;
            size_t LastSecFPS = 0;
            size_t TimeDemoFont = 0;
            size_t LoadingMessage = 0;
            size_t SavingMessage = 0;
            size_t ConnectingMessage = 0;
            size_t PausedMessage = 0;
            size_t PrecachingMessage = 0;
            size_t FrameRateText = 0;
            size_t AvgText = 0;
            size_t LastSecText = 0;
            size_t MinText = 0;
            size_t MaxText = 0;
            size_t fpsText = 0;
            size_t SecondsText = 0;
            size_t FramesText = 0;
        };
    }

    extern console::Offsets offsets_console;

    namespace console {
        inline int32_t& vtblOut(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_console.vtblOut);
        }

        inline UObject*& Viewport(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_console.Viewport);
        }

        inline int32_t& HistoryTop(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_console.HistoryTop);
        }

        inline int32_t& HistoryBot(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_console.HistoryBot);
        }

        inline int32_t& HistoryCur(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_console.HistoryCur);
        }

        inline std::string& TypedStr(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_console.TypedStr);
        }

        inline std::string& History(UObject* o, int index)
        {
            return reinterpret_cast<std::string*>(o->data + offsets_console.History)[index];
        }

        inline int32_t& Scrollback(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_console.Scrollback);
        }

        inline int32_t& numLines(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_console.numLines);
        }

        inline int32_t& TopLine(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_console.TopLine);
        }

        inline int32_t& TextLines(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_console.TextLines);
        }

        inline float& MsgTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_console.MsgTime);
        }

        inline float& MsgTickTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_console.MsgTickTime);
        }

        inline std::string& MsgText(UObject* o, int index)
        {
            return reinterpret_cast<std::string*>(o->data + offsets_console.MsgText)[index];
        }

        inline Name& MsgType(UObject* o, int index)
        {
            return reinterpret_cast<Name*>(o->data + offsets_console.MsgType)[index];
        }

        inline UObject*& MsgPlayer(UObject* o, int index)
        {
            return reinterpret_cast<UObject**>(o->data + offsets_console.MsgPlayer)[index];
        }

        inline float& MsgTick(UObject* o, int index)
        {
            return reinterpret_cast<float*>(o->data + offsets_console.MsgTick)[index];
        }

        inline int32_t& BorderSize(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_console.BorderSize);
        }

        inline int32_t& ConsoleLines(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_console.ConsoleLines);
        }

        inline int32_t& BorderLines(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_console.BorderLines);
        }

        inline int32_t& BorderPixels(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_console.BorderPixels);
        }

        inline float& ConsolePos(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_console.ConsolePos);
        }

        inline float& ConsoleDest(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_console.ConsoleDest);
        }

        inline float& FrameX(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_console.FrameX);
        }

        inline float& FrameY(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_console.FrameY);
        }

        inline UObject*& ConBackground(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_console.ConBackground);
        }

        inline UObject*& Border(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_console.Border);
        }

        inline uint32_t& bNoStuff(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_console.bNoStuff);
        }

        inline uint32_t& bTyping(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_console.bTyping);
        }

        inline uint32_t& bNoDrawWorld(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_console.bNoDrawWorld);
        }

        inline uint32_t& bTimeDemo(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_console.bTimeDemo);
        }

        inline uint32_t& bStartTimeDemo(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_console.bStartTimeDemo);
        }

        inline uint32_t& bRestartTimeDemo(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_console.bRestartTimeDemo);
        }

        inline uint32_t& bSaveTimeDemoToFile(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_console.bSaveTimeDemoToFile);
        }

        inline float& StartTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_console.StartTime);
        }

        inline float& ExtraTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_console.ExtraTime);
        }

        inline float& LastFrameTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_console.LastFrameTime);
        }

        inline float& LastSecondStartTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_console.LastSecondStartTime);
        }

        inline int32_t& FrameCount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_console.FrameCount);
        }

        inline int32_t& LastSecondFrameCount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_console.LastSecondFrameCount);
        }

        inline float& MinFPS(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_console.MinFPS);
        }

        inline float& MaxFPS(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_console.MaxFPS);
        }

        inline float& LastSecFPS(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_console.LastSecFPS);
        }

        inline UObject*& TimeDemoFont(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_console.TimeDemoFont);
        }

        inline std::string& LoadingMessage(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_console.LoadingMessage);
        }

        inline std::string& SavingMessage(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_console.SavingMessage);
        }

        inline std::string& ConnectingMessage(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_console.ConnectingMessage);
        }

        inline std::string& PausedMessage(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_console.PausedMessage);
        }

        inline std::string& PrecachingMessage(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_console.PrecachingMessage);
        }

        inline std::string& FrameRateText(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_console.FrameRateText);
        }

        inline std::string& AvgText(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_console.AvgText);
        }

        inline std::string& LastSecText(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_console.LastSecText);
        }

        inline std::string& MinText(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_console.MinText);
        }

        inline std::string& MaxText(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_console.MaxText);
        }

        inline std::string& fpsText(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_console.fpsText);
        }

        inline std::string& SecondsText(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_console.SecondsText);
        }

        inline std::string& FramesText(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_console.FramesText);
        }

    }

    namespace hud {
        struct Offsets {
            size_t HudMode = 0;
            size_t Crosshair = 0;
            size_t MainMenuType = 0;
            size_t HUDConfigWindowType = 0;
            size_t WhiteColor = 0;
            size_t MainMenu = 0;
            size_t HUDMutator = 0;
            size_t PlayerOwner = 0;
        };
    }

    extern hud::Offsets offsets_hud;

    namespace hud {
        inline int32_t& HudMode(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_hud.HudMode);
        }

        inline int32_t& Crosshair(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_hud.Crosshair);
        }

        inline UObject*& MainMenuType(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_hud.MainMenuType);
        }

        inline std::string& HUDConfigWindowType(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_hud.HUDConfigWindowType);
        }

        inline Color& WhiteColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_hud.WhiteColor);
        }

        inline UObject*& MainMenu(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_hud.MainMenu);
        }

        inline UObject*& HUDMutator(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_hud.HUDMutator);
        }

        inline UObject*& PlayerOwner(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_hud.PlayerOwner);
        }

    }

    namespace bitmap {
        struct Offsets {
            size_t Format = 0;
            size_t Palette = 0;
            size_t UBits = 0;
            size_t VBits = 0;
            size_t USize = 0;
            size_t VSize = 0;
            size_t UClamp = 0;
            size_t VClamp = 0;
            size_t MipZero = 0;
            size_t MaxColor = 0;
            size_t InternalTime = 0;
        };
    }

    extern bitmap::Offsets offsets_bitmap;

    namespace bitmap {
        inline uint8_t& Format(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_bitmap.Format);
        }

        inline UObject*& Palette(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_bitmap.Palette);
        }

        inline uint8_t& UBits(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_bitmap.UBits);
        }

        inline uint8_t& VBits(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_bitmap.VBits);
        }

        inline int32_t& USize(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_bitmap.USize);
        }

        inline int32_t& VSize(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_bitmap.VSize);
        }

        inline int32_t& UClamp(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_bitmap.UClamp);
        }

        inline int32_t& VClamp(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_bitmap.VClamp);
        }

        inline Color& MipZero(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_bitmap.MipZero);
        }

        inline Color& MaxColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_bitmap.MaxColor);
        }

        inline int32_t& InternalTime(UObject* o, int index)
        {
            return reinterpret_cast<int32_t*>(o->data + offsets_bitmap.InternalTime)[index];
        }

    }

    namespace texture {
        struct Offsets {
            size_t BumpMap = 0;
            size_t DetailTexture = 0;
            size_t MacroTexture = 0;
            size_t Diffuse = 0;
            size_t Specular = 0;
            size_t Alpha = 0;
            size_t DrawScale = 0;
            size_t Friction = 0;
            size_t MipMult = 0;
            size_t FootstepSound = 0;
            size_t HitSound = 0;
            size_t bInvisible = 0;
            size_t bMasked = 0;
            size_t bTransparent = 0;
            size_t bNotSolid = 0;
            size_t bEnvironment = 0;
            size_t bSemisolid = 0;
            size_t bModulate = 0;
            size_t bFakeBackdrop = 0;
            size_t bTwoSided = 0;
            size_t bAutoUPan = 0;
            size_t bAutoVPan = 0;
            size_t bNoSmooth = 0;
            size_t bBigWavy = 0;
            size_t bSmallWavy = 0;
            size_t bWaterWavy = 0;
            size_t bLowShadowDetail = 0;
            size_t bNoMerge = 0;
            size_t bCloudWavy = 0;
            size_t bDirtyShadows = 0;
            size_t bHighLedge = 0;
            size_t bSpecialLit = 0;
            size_t bGouraud = 0;
            size_t bUnlit = 0;
            size_t bHighShadowDetail = 0;
            size_t bPortal = 0;
            size_t bMirrored = 0;
            size_t bX2 = 0;
            size_t bX3 = 0;
            size_t bX4 = 0;
            size_t bX5 = 0;
            size_t bX6 = 0;
            size_t bX7 = 0;
            size_t bHighColorQuality = 0;
            size_t bHighTextureQuality = 0;
            size_t bRealtime = 0;
            size_t bParametric = 0;
            size_t bRealtimeChanged = 0;
            size_t bHasComp = 0;
            size_t LODSet = 0;
            size_t AnimNext = 0;
            size_t AnimCurrent = 0;
            size_t PrimeCount = 0;
            size_t PrimeCurrent = 0;
            size_t MinFrameRate = 0;
            size_t MaxFrameRate = 0;
            size_t Accumulator = 0;
            size_t Mips = 0;
            size_t CompMips = 0;
            size_t CompFormat = 0;
        };
    }

    extern texture::Offsets offsets_texture;

    namespace texture {
        inline UObject*& BumpMap(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_texture.BumpMap);
        }

        inline UObject*& DetailTexture(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_texture.DetailTexture);
        }

        inline UObject*& MacroTexture(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_texture.MacroTexture);
        }

        inline float& Diffuse(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_texture.Diffuse);
        }

        inline float& Specular(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_texture.Specular);
        }

        inline float& Alpha(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_texture.Alpha);
        }

        inline float& DrawScale(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_texture.DrawScale);
        }

        inline float& Friction(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_texture.Friction);
        }

        inline float& MipMult(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_texture.MipMult);
        }

        inline UObject*& FootstepSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_texture.FootstepSound);
        }

        inline UObject*& HitSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_texture.HitSound);
        }

        inline uint32_t& bInvisible(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bInvisible);
        }

        inline uint32_t& bMasked(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bMasked);
        }

        inline uint32_t& bTransparent(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bTransparent);
        }

        inline uint32_t& bNotSolid(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bNotSolid);
        }

        inline uint32_t& bEnvironment(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bEnvironment);
        }

        inline uint32_t& bSemisolid(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bSemisolid);
        }

        inline uint32_t& bModulate(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bModulate);
        }

        inline uint32_t& bFakeBackdrop(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bFakeBackdrop);
        }

        inline uint32_t& bTwoSided(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bTwoSided);
        }

        inline uint32_t& bAutoUPan(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bAutoUPan);
        }

        inline uint32_t& bAutoVPan(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bAutoVPan);
        }

        inline uint32_t& bNoSmooth(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bNoSmooth);
        }

        inline uint32_t& bBigWavy(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bBigWavy);
        }

        inline uint32_t& bSmallWavy(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bSmallWavy);
        }

        inline uint32_t& bWaterWavy(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bWaterWavy);
        }

        inline uint32_t& bLowShadowDetail(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bLowShadowDetail);
        }

        inline uint32_t& bNoMerge(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bNoMerge);
        }

        inline uint32_t& bCloudWavy(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bCloudWavy);
        }

        inline uint32_t& bDirtyShadows(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bDirtyShadows);
        }

        inline uint32_t& bHighLedge(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bHighLedge);
        }

        inline uint32_t& bSpecialLit(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bSpecialLit);
        }

        inline uint32_t& bGouraud(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bGouraud);
        }

        inline uint32_t& bUnlit(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bUnlit);
        }

        inline uint32_t& bHighShadowDetail(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bHighShadowDetail);
        }

        inline uint32_t& bPortal(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bPortal);
        }

        inline uint32_t& bMirrored(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bMirrored);
        }

        inline uint32_t& bX2(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bX2);
        }

        inline uint32_t& bX3(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bX3);
        }

        inline uint32_t& bX4(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bX4);
        }

        inline uint32_t& bX5(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bX5);
        }

        inline uint32_t& bX6(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bX6);
        }

        inline uint32_t& bX7(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bX7);
        }

        inline uint32_t& bHighColorQuality(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bHighColorQuality);
        }

        inline uint32_t& bHighTextureQuality(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bHighTextureQuality);
        }

        inline uint32_t& bRealtime(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bRealtime);
        }

        inline uint32_t& bParametric(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bParametric);
        }

        inline uint32_t& bRealtimeChanged(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bRealtimeChanged);
        }

        inline uint32_t& bHasComp(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_texture.bHasComp);
        }

        inline uint8_t& LODSet(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_texture.LODSet);
        }

        inline UObject*& AnimNext(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_texture.AnimNext);
        }

        inline UObject*& AnimCurrent(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_texture.AnimCurrent);
        }

        inline uint8_t& PrimeCount(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_texture.PrimeCount);
        }

        inline uint8_t& PrimeCurrent(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_texture.PrimeCurrent);
        }

        inline float& MinFrameRate(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_texture.MinFrameRate);
        }

        inline float& MaxFrameRate(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_texture.MaxFrameRate);
        }

        inline float& Accumulator(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_texture.Accumulator);
        }

        inline uint8_t* Mips(UObject* o)
        {
            return o->data + offsets_texture.Mips;
        }

        inline uint8_t* CompMips(UObject* o)
        {
            return o->data + offsets_texture.CompMips;
        }

        inline uint8_t& CompFormat(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_texture.CompFormat);
        }

    }

    namespace player {
        struct Offsets {
            size_t vfOut = 0;
            size_t vfExec = 0;
            size_t Actor = 0;
            size_t Console = 0;
            size_t bWindowsMouseAvailable = 0;
            size_t bShowWindowsMouse = 0;
            size_t bSuspendPrecaching = 0;
            size_t WindowsMouseX = 0;
            size_t WindowsMouseY = 0;
            size_t CurrentNetSpeed = 0;
            size_t ConfiguredInternetSpeed = 0;
            size_t ConfiguredLanSpeed = 0;
            size_t StaticUpdateInterval = 0;
            size_t DynamicUpdateInterval = 0;
            size_t SelectedCursor = 0;
        };
    }

    extern player::Offsets offsets_player;

    namespace player {
        inline int32_t& vfOut(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_player.vfOut);
        }

        inline int32_t& vfExec(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_player.vfExec);
        }

        inline UObject*& Actor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_player.Actor);
        }

        inline UObject*& Console(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_player.Console);
        }

        inline uint32_t& bWindowsMouseAvailable(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_player.bWindowsMouseAvailable);
        }

        inline uint32_t& bShowWindowsMouse(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_player.bShowWindowsMouse);
        }

        inline uint32_t& bSuspendPrecaching(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_player.bSuspendPrecaching);
        }

        inline float& WindowsMouseX(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_player.WindowsMouseX);
        }

        inline float& WindowsMouseY(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_player.WindowsMouseY);
        }

        inline int32_t& CurrentNetSpeed(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_player.CurrentNetSpeed);
        }

        inline int32_t& ConfiguredInternetSpeed(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_player.ConfiguredInternetSpeed);
        }

        inline int32_t& ConfiguredLanSpeed(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_player.ConfiguredLanSpeed);
        }

        inline float& StaticUpdateInterval(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_player.StaticUpdateInterval);
        }

        inline float& DynamicUpdateInterval(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_player.DynamicUpdateInterval);
        }

        inline uint8_t& SelectedCursor(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_player.SelectedCursor);
        }

    }

    namespace light {
        struct Offsets {
        };
    }

    extern light::Offsets offsets_light;

    namespace light {

    }

    namespace info {
        struct Offsets {
        };
    }

    extern info::Offsets offsets_info;

    namespace info {

    }

    namespace playerreplicationinfo {
        struct Offsets {
            size_t PlayerName = 0;
            size_t OldName = 0;
            size_t PlayerID = 0;
            size_t TeamName = 0;
            size_t Team = 0;
            size_t TeamID = 0;
            size_t Score = 0;
            size_t Deaths = 0;
            size_t VoiceType = 0;
            size_t HasFlag = 0;
            size_t Ping = 0;
            size_t PacketLoss = 0;
            size_t bIsFemale = 0;
            size_t bIsABot = 0;
            size_t bFeigningDeath = 0;
            size_t bIsSpectator = 0;
            size_t bWaitingPlayer = 0;
            size_t bAdmin = 0;
            size_t TalkTexture = 0;
            size_t PlayerZone = 0;
            size_t PlayerLocation = 0;
            size_t StartTime = 0;
            size_t TimeAcc = 0;
            size_t Streak = 0;
        };
    }

    extern playerreplicationinfo::Offsets offsets_playerreplicationinfo;

    namespace playerreplicationinfo {
        inline std::string& PlayerName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_playerreplicationinfo.PlayerName);
        }

        inline std::string& OldName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_playerreplicationinfo.OldName);
        }

        inline int32_t& PlayerID(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_playerreplicationinfo.PlayerID);
        }

        inline std::string& TeamName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_playerreplicationinfo.TeamName);
        }

        inline uint8_t& Team(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_playerreplicationinfo.Team);
        }

        inline int32_t& TeamID(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_playerreplicationinfo.TeamID);
        }

        inline float& Score(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerreplicationinfo.Score);
        }

        inline float& Deaths(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerreplicationinfo.Deaths);
        }

        inline UObject*& VoiceType(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_playerreplicationinfo.VoiceType);
        }

        inline UObject*& HasFlag(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_playerreplicationinfo.HasFlag);
        }

        inline int32_t& Ping(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_playerreplicationinfo.Ping);
        }

        inline uint8_t& PacketLoss(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_playerreplicationinfo.PacketLoss);
        }

        inline uint32_t& bIsFemale(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerreplicationinfo.bIsFemale);
        }

        inline uint32_t& bIsABot(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerreplicationinfo.bIsABot);
        }

        inline uint32_t& bFeigningDeath(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerreplicationinfo.bFeigningDeath);
        }

        inline uint32_t& bIsSpectator(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerreplicationinfo.bIsSpectator);
        }

        inline uint32_t& bWaitingPlayer(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerreplicationinfo.bWaitingPlayer);
        }

        inline uint32_t& bAdmin(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_playerreplicationinfo.bAdmin);
        }

        inline UObject*& TalkTexture(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_playerreplicationinfo.TalkTexture);
        }

        inline UObject*& PlayerZone(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_playerreplicationinfo.PlayerZone);
        }

        inline UObject*& PlayerLocation(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_playerreplicationinfo.PlayerLocation);
        }

        inline int32_t& StartTime(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_playerreplicationinfo.StartTime);
        }

        inline int32_t& TimeAcc(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_playerreplicationinfo.TimeAcc);
        }

        inline float& Streak(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_playerreplicationinfo.Streak);
        }

    }

    namespace decal {
        struct Offsets {
            size_t MultiDecalLevel = 0;
            size_t LastRenderedTime = 0;
            size_t SurfList = 0;
        };
    }

    extern decal::Offsets offsets_decal;

    namespace decal {
        inline int32_t& MultiDecalLevel(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_decal.MultiDecalLevel);
        }

        inline float& LastRenderedTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_decal.LastRenderedTime);
        }

        inline uint8_t* SurfList(UObject* o)
        {
            return o->data + offsets_decal.SurfList;
        }

    }

    namespace teleporter {
        struct Offsets {
            size_t URL = 0;
            size_t ProductRequired = 0;
            size_t bChangesVelocity = 0;
            size_t bChangesYaw = 0;
            size_t bReversesX = 0;
            size_t bReversesY = 0;
            size_t bReversesZ = 0;
            size_t bEnabled = 0;
            size_t TargetVelocity = 0;
            size_t TriggerActor = 0;
            size_t TriggerActor2 = 0;
            size_t LastFired = 0;
        };
    }

    extern teleporter::Offsets offsets_teleporter;

    namespace teleporter {
        inline std::string& URL(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_teleporter.URL);
        }

        inline Name& ProductRequired(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_teleporter.ProductRequired);
        }

        inline uint32_t& bChangesVelocity(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_teleporter.bChangesVelocity);
        }

        inline uint32_t& bChangesYaw(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_teleporter.bChangesYaw);
        }

        inline uint32_t& bReversesX(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_teleporter.bReversesX);
        }

        inline uint32_t& bReversesY(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_teleporter.bReversesY);
        }

        inline uint32_t& bReversesZ(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_teleporter.bReversesZ);
        }

        inline uint32_t& bEnabled(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_teleporter.bEnabled);
        }

        inline Vector& TargetVelocity(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_teleporter.TargetVelocity);
        }

        inline UObject*& TriggerActor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_teleporter.TriggerActor);
        }

        inline UObject*& TriggerActor2(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_teleporter.TriggerActor2);
        }

        inline float& LastFired(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_teleporter.LastFired);
        }

    }

    namespace scriptedtexture {
        struct Offsets {
            size_t NotifyActor = 0;
            size_t SourceTexture = 0;
            size_t Junk1 = 0;
            size_t Junk2 = 0;
            size_t Junk3 = 0;
            size_t LocalTime = 0;
        };
    }

    extern scriptedtexture::Offsets offsets_scriptedtexture;

    namespace scriptedtexture {
        inline UObject*& NotifyActor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scriptedtexture.NotifyActor);
        }

        inline UObject*& SourceTexture(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scriptedtexture.SourceTexture);
        }

        inline int32_t& Junk1(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_scriptedtexture.Junk1);
        }

        inline int32_t& Junk2(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_scriptedtexture.Junk2);
        }

        inline int32_t& Junk3(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_scriptedtexture.Junk3);
        }

        inline float& LocalTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedtexture.LocalTime);
        }

    }

    namespace carcass {
        struct Offsets {
            size_t bPlayerCarcass = 0;
            size_t flies = 0;
            size_t rats = 0;
            size_t bReducedHeight = 0;
            size_t bDecorative = 0;
            size_t bSlidingCarcass = 0;
            size_t CumulativeDamage = 0;
            size_t PlayerOwner = 0;
            size_t Bugs = 0;
        };
    }

    extern carcass::Offsets offsets_carcass;

    namespace carcass {
        inline uint32_t& bPlayerCarcass(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_carcass.bPlayerCarcass);
        }

        inline uint8_t& flies(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_carcass.flies);
        }

        inline uint8_t& rats(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_carcass.rats);
        }

        inline uint32_t& bReducedHeight(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_carcass.bReducedHeight);
        }

        inline uint32_t& bDecorative(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_carcass.bDecorative);
        }

        inline uint32_t& bSlidingCarcass(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_carcass.bSlidingCarcass);
        }

        inline int32_t& CumulativeDamage(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_carcass.CumulativeDamage);
        }

        inline UObject*& PlayerOwner(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_carcass.PlayerOwner);
        }

        inline UObject*& Bugs(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_carcass.Bugs);
        }

    }

    namespace ammo {
        struct Offsets {
            size_t AmmoAmount = 0;
            size_t MaxAmmo = 0;
            size_t ParentAmmo = 0;
            size_t UsedInWeaponSlot = 0;
            size_t PAmmo = 0;
        };
    }

    extern ammo::Offsets offsets_ammo;

    namespace ammo {
        inline int32_t& AmmoAmount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_ammo.AmmoAmount);
        }

        inline int32_t& MaxAmmo(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_ammo.MaxAmmo);
        }

        inline UObject*& ParentAmmo(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_ammo.ParentAmmo);
        }

        inline uint8_t& UsedInWeaponSlot(UObject* o, int index)
        {
            return reinterpret_cast<uint8_t*>(o->data + offsets_ammo.UsedInWeaponSlot)[index];
        }

        inline UObject*& PAmmo(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_ammo.PAmmo);
        }

    }

    namespace pickup {
        struct Offsets {
            size_t Inv = 0;
            size_t NumCopies = 0;
            size_t bCanHaveMultipleCopies = 0;
            size_t bCanActivate = 0;
            size_t ExpireMessage = 0;
            size_t bAutoActivate = 0;
        };
    }

    extern pickup::Offsets offsets_pickup;

    namespace pickup {
        inline UObject*& Inv(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_pickup.Inv);
        }

        inline int32_t& NumCopies(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_pickup.NumCopies);
        }

        inline uint32_t& bCanHaveMultipleCopies(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pickup.bCanHaveMultipleCopies);
        }

        inline uint32_t& bCanActivate(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pickup.bCanActivate);
        }

        inline std::string& ExpireMessage(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_pickup.ExpireMessage);
        }

        inline uint32_t& bAutoActivate(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_pickup.bAutoActivate);
        }

    }

    namespace pathnode {
        struct Offsets {
        };
    }

    extern pathnode::Offsets offsets_pathnode;

    namespace pathnode {

    }

    namespace trigger {
        struct Offsets {
            size_t TriggerType = 0;
            size_t Message = 0;
            size_t bTriggerOnceOnly = 0;
            size_t bInitiallyActive = 0;
            size_t ClassProximityType = 0;
            size_t RepeatTriggerTime = 0;
            size_t ReTriggerDelay = 0;
            size_t TriggerTime = 0;
            size_t DamageThreshold = 0;
            size_t TriggerActor = 0;
            size_t TriggerActor2 = 0;
        };
    }

    extern trigger::Offsets offsets_trigger;

    namespace trigger {
        inline uint8_t& TriggerType(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_trigger.TriggerType);
        }

        inline std::string& Message(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_trigger.Message);
        }

        inline uint32_t& bTriggerOnceOnly(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_trigger.bTriggerOnceOnly);
        }

        inline uint32_t& bInitiallyActive(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_trigger.bInitiallyActive);
        }

        inline UObject*& ClassProximityType(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_trigger.ClassProximityType);
        }

        inline float& RepeatTriggerTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_trigger.RepeatTriggerTime);
        }

        inline float& ReTriggerDelay(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_trigger.ReTriggerDelay);
        }

        inline float& TriggerTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_trigger.TriggerTime);
        }

        inline float& DamageThreshold(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_trigger.DamageThreshold);
        }

        inline UObject*& TriggerActor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_trigger.TriggerActor);
        }

        inline UObject*& TriggerActor2(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_trigger.TriggerActor2);
        }

    }

    namespace effects {
        struct Offsets {
            size_t EffectSound1 = 0;
            size_t EffectSound2 = 0;
            size_t bOnlyTriggerable = 0;
        };
    }

    extern effects::Offsets offsets_effects;

    namespace effects {
        inline UObject*& EffectSound1(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_effects.EffectSound1);
        }

        inline UObject*& EffectSound2(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_effects.EffectSound2);
        }

        inline uint32_t& bOnlyTriggerable(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_effects.bOnlyTriggerable);
        }

    }

    namespace spawnnotify {
        struct Offsets {
            size_t ActorClass = 0;
            size_t Next = 0;
        };
    }

    extern spawnnotify::Offsets offsets_spawnnotify;

    namespace spawnnotify {
        inline UObject*& ActorClass(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_spawnnotify.ActorClass);
        }

        inline UObject*& Next(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_spawnnotify.Next);
        }

    }

    namespace keypoint {
        struct Offsets {
        };
    }

    extern keypoint::Offsets offsets_keypoint;

    namespace keypoint {

    }

    namespace mesh {
        struct Offsets {
        };
    }

    extern mesh::Offsets offsets_mesh;

    namespace mesh {

    }

    void bindProps_engine();
}
