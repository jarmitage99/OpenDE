#pragma once

#include "core/math.h"
#include "core/object.h"

#include <string>

namespace dx {
    namespace deusexplayer {
        struct Offsets {
            size_t TruePlayerName = 0;
            size_t PlayerSkin = 0;
            size_t CombatDifficulty = 0;
            size_t AugmentationSystem = 0;
            size_t SkillSystem = 0;
            size_t SkillPointsTotal = 0;
            size_t SkillPointsAvail = 0;
            size_t Credits = 0;
            size_t Energy = 0;
            size_t EnergyMax = 0;
            size_t EnergyDrain = 0;
            size_t EnergyDrainTotal = 0;
            size_t MaxRegenPoint = 0;
            size_t RegenRate = 0;
            size_t KeyRing = 0;
            size_t KeyList = 0;
            size_t MaxFrobDistance = 0;
            size_t FrobTarget = 0;
            size_t FrobTime = 0;
            size_t LastRefreshTime = 0;
            size_t ConPlay = 0;
            size_t DataLinkPlay = 0;
            size_t ConHistory = 0;
            size_t invSlots = 0;
            size_t maxInvRows = 0;
            size_t maxInvCols = 0;
            size_t inHand = 0;
            size_t inHandPending = 0;
            size_t ClientinHandPending = 0;
            size_t LastinHand = 0;
            size_t bInHandTransition = 0;
            size_t bBeltIsMPInventory = 0;
            size_t FirstGoal = 0;
            size_t LastGoal = 0;
            size_t FirstNote = 0;
            size_t LastNote = 0;
            size_t FirstImage = 0;
            size_t FirstLog = 0;
            size_t LastLog = 0;
            size_t ViewModelActor = 0;
            size_t bFirstOptionsSynced = 0;
            size_t bSecondOptionsSynced = 0;
            size_t bForceDuck = 0;
            size_t bCrouchOn = 0;
            size_t bWasCrouchOn = 0;
            size_t lastbDuck = 0;
            size_t bCanLean = 0;
            size_t curLeanDist = 0;
            size_t prevLeanDist = 0;
            size_t bToggleWalk = 0;
            size_t RunSilentValue = 0;
            size_t bWarrenEMPField = 0;
            size_t WarrenTimer = 0;
            size_t WarrenSlot = 0;
            size_t FloorMaterial = 0;
            size_t WallMaterial = 0;
            size_t WallNormal = 0;
            size_t drugEffectTimer = 0;
            size_t JoltMagnitude = 0;
            size_t poisonTimer = 0;
            size_t poisonCounter = 0;
            size_t poisonDamage = 0;
            size_t BleedRate = 0;
            size_t DropCounter = 0;
            size_t ClotPeriod = 0;
            size_t FlashTimer = 0;
            size_t swimDuration = 0;
            size_t swimTimer = 0;
            size_t swimBubbleTimer = 0;
            size_t ConversationActor = 0;
            size_t lastThirdPersonConvoActor = 0;
            size_t lastThirdPersonConvoTime = 0;
            size_t lastFirstPersonConvoActor = 0;
            size_t lastFirstPersonConvoTime = 0;
            size_t bStartingNewGame = 0;
            size_t bSavingSkillsAugs = 0;
            size_t bSpyDroneActive = 0;
            size_t spyDroneLevel = 0;
            size_t spyDroneLevelValue = 0;
            size_t aDrone = 0;
            size_t bBuySkills = 0;
            size_t bKillerProfile = 0;
            size_t mpMsgFlags = 0;
            size_t mpMsgServerFlags = 0;
            size_t mpMsgCode = 0;
            size_t mpMsgTime = 0;
            size_t mpMsgOptionalParam = 0;
            size_t mpMsgOptionalString = 0;
            size_t strStartMap = 0;
            size_t bStartNewGameAfterIntro = 0;
            size_t bIgnoreNextShowMenu = 0;
            size_t NextMap = 0;
            size_t bObjectNames = 0;
            size_t bNPCHighlighting = 0;
            size_t bSubtitles = 0;
            size_t bAlwaysRun = 0;
            size_t bToggleCrouch = 0;
            size_t logTimeout = 0;
            size_t maxLogLines = 0;
            size_t bHelpMessages = 0;
            size_t translucencyLevel = 0;
            size_t bObjectBeltVisible = 0;
            size_t bHitDisplayVisible = 0;
            size_t bAmmoDisplayVisible = 0;
            size_t bAugDisplayVisible = 0;
            size_t bDisplayAmmoByClip = 0;
            size_t bCompassVisible = 0;
            size_t bCrosshairVisible = 0;
            size_t bAutoReload = 0;
            size_t bDisplayAllGoals = 0;
            size_t bHUDShowAllAugs = 0;
            size_t UIBackground = 0;
            size_t bDisplayCompletedGoals = 0;
            size_t bShowAmmoDescriptions = 0;
            size_t bConfirmSaveDeletes = 0;
            size_t bConfirmNoteDeletes = 0;
            size_t bAskedToTrain = 0;
            size_t AugPrefs = 0;
            size_t BarkManager = 0;
            size_t ThemeManager = 0;
            size_t MenuThemeName = 0;
            size_t HUDThemeName = 0;
            size_t bHUDBordersVisible = 0;
            size_t bHUDBordersTranslucent = 0;
            size_t bHUDBackgroundTranslucent = 0;
            size_t bMenusTranslucent = 0;
            size_t InventoryFull = 0;
            size_t TooMuchAmmo = 0;
            size_t TooHeavyToLift = 0;
            size_t CannotLift = 0;
            size_t NoRoomToLift = 0;
            size_t CanCarryOnlyOne = 0;
            size_t CannotDropHere = 0;
            size_t HandsFull = 0;
            size_t NoteAdded = 0;
            size_t GoalAdded = 0;
            size_t PrimaryGoalCompleted = 0;
            size_t SecondaryGoalCompleted = 0;
            size_t EnergyDepleted = 0;
            size_t AddedNanoKey = 0;
            size_t HealedPointsLabel = 0;
            size_t HealedPointLabel = 0;
            size_t SkillPointsAward = 0;
            size_t QuickSaveGameTitle = 0;
            size_t WeaponUnCloak = 0;
            size_t TakenOverString = 0;
            size_t HeadString = 0;
            size_t TorsoString = 0;
            size_t LegsString = 0;
            size_t WithTheString = 0;
            size_t WithString = 0;
            size_t PoisonString = 0;
            size_t BurnString = 0;
            size_t NoneString = 0;
            size_t DamageShield = 0;
            size_t ShieldTimer = 0;
            size_t ShieldStatus = 0;
            size_t myBurner = 0;
            size_t myPoisoner = 0;
            size_t myProjKiller = 0;
            size_t myTurretKiller = 0;
            size_t myKiller = 0;
            size_t killProfile = 0;
            size_t invulnSph = 0;
            size_t musicMode = 0;
            size_t savedSection = 0;
            size_t musicCheckTimer = 0;
            size_t musicChangeTimer = 0;
            size_t saveCount = 0;
            size_t saveTime = 0;
            size_t GlobalDebugObj = 0;
            size_t bQuotesEnabled = 0;
            size_t DXGame = 0;
            size_t ServerTimeDiff = 0;
            size_t ServerTimeLastRefresh = 0;
            size_t MPDamageMult = 0;
            size_t NintendoImmunityTime = 0;
            size_t NintendoImmunityTimeLeft = 0;
            size_t bNintendoImmunity = 0;
            size_t ActiveComputer = 0;
        };
    }

    extern deusexplayer::Offsets offsets_deusexplayer;

    namespace deusexplayer {
        inline std::string& TruePlayerName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.TruePlayerName);
        }

        inline int32_t& PlayerSkin(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexplayer.PlayerSkin);
        }

        inline float& CombatDifficulty(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.CombatDifficulty);
        }

        inline UObject*& AugmentationSystem(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.AugmentationSystem);
        }

        inline UObject*& SkillSystem(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.SkillSystem);
        }

        inline int32_t& SkillPointsTotal(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexplayer.SkillPointsTotal);
        }

        inline int32_t& SkillPointsAvail(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexplayer.SkillPointsAvail);
        }

        inline int32_t& Credits(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexplayer.Credits);
        }

        inline float& Energy(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.Energy);
        }

        inline float& EnergyMax(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.EnergyMax);
        }

        inline float& EnergyDrain(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.EnergyDrain);
        }

        inline float& EnergyDrainTotal(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.EnergyDrainTotal);
        }

        inline float& MaxRegenPoint(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.MaxRegenPoint);
        }

        inline float& RegenRate(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.RegenRate);
        }

        inline UObject*& KeyRing(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.KeyRing);
        }

        inline UObject*& KeyList(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.KeyList);
        }

        inline float& MaxFrobDistance(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.MaxFrobDistance);
        }

        inline UObject*& FrobTarget(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.FrobTarget);
        }

        inline float& FrobTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.FrobTime);
        }

        inline float& LastRefreshTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.LastRefreshTime);
        }

        inline UObject*& ConPlay(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.ConPlay);
        }

        inline UObject*& DataLinkPlay(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.DataLinkPlay);
        }

        inline UObject*& ConHistory(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.ConHistory);
        }

        inline uint8_t& invSlots(UObject* o, int index)
        {
            return reinterpret_cast<uint8_t*>(o->data + offsets_deusexplayer.invSlots)[index];
        }

        inline int32_t& maxInvRows(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexplayer.maxInvRows);
        }

        inline int32_t& maxInvCols(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexplayer.maxInvCols);
        }

        inline UObject*& inHand(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.inHand);
        }

        inline UObject*& inHandPending(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.inHandPending);
        }

        inline UObject*& ClientinHandPending(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.ClientinHandPending);
        }

        inline UObject*& LastinHand(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.LastinHand);
        }

        inline uint32_t& bInHandTransition(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bInHandTransition);
        }

        inline uint32_t& bBeltIsMPInventory(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bBeltIsMPInventory);
        }

        inline UObject*& FirstGoal(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.FirstGoal);
        }

        inline UObject*& LastGoal(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.LastGoal);
        }

        inline UObject*& FirstNote(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.FirstNote);
        }

        inline UObject*& LastNote(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.LastNote);
        }

        inline UObject*& FirstImage(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.FirstImage);
        }

        inline UObject*& FirstLog(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.FirstLog);
        }

        inline UObject*& LastLog(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.LastLog);
        }

        inline UObject*& ViewModelActor(UObject* o, int index)
        {
            return reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.ViewModelActor)[index];
        }

        inline uint32_t& bFirstOptionsSynced(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bFirstOptionsSynced);
        }

        inline uint32_t& bSecondOptionsSynced(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bSecondOptionsSynced);
        }

        inline uint32_t& bForceDuck(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bForceDuck);
        }

        inline uint32_t& bCrouchOn(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bCrouchOn);
        }

        inline uint32_t& bWasCrouchOn(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bWasCrouchOn);
        }

        inline uint8_t& lastbDuck(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_deusexplayer.lastbDuck);
        }

        inline uint32_t& bCanLean(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bCanLean);
        }

        inline float& curLeanDist(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.curLeanDist);
        }

        inline float& prevLeanDist(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.prevLeanDist);
        }

        inline uint32_t& bToggleWalk(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bToggleWalk);
        }

        inline float& RunSilentValue(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.RunSilentValue);
        }

        inline uint32_t& bWarrenEMPField(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bWarrenEMPField);
        }

        inline float& WarrenTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.WarrenTimer);
        }

        inline int32_t& WarrenSlot(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexplayer.WarrenSlot);
        }

        inline Name& FloorMaterial(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_deusexplayer.FloorMaterial);
        }

        inline Name& WallMaterial(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_deusexplayer.WallMaterial);
        }

        inline Vector& WallNormal(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_deusexplayer.WallNormal);
        }

        inline float& drugEffectTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.drugEffectTimer);
        }

        inline float& JoltMagnitude(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.JoltMagnitude);
        }

        inline float& poisonTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.poisonTimer);
        }

        inline int32_t& poisonCounter(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexplayer.poisonCounter);
        }

        inline int32_t& poisonDamage(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexplayer.poisonDamage);
        }

        inline float& BleedRate(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.BleedRate);
        }

        inline float& DropCounter(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.DropCounter);
        }

        inline float& ClotPeriod(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.ClotPeriod);
        }

        inline float& FlashTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.FlashTimer);
        }

        inline float& swimDuration(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.swimDuration);
        }

        inline float& swimTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.swimTimer);
        }

        inline float& swimBubbleTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.swimBubbleTimer);
        }

        inline UObject*& ConversationActor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.ConversationActor);
        }

        inline UObject*& lastThirdPersonConvoActor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.lastThirdPersonConvoActor);
        }

        inline float& lastThirdPersonConvoTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.lastThirdPersonConvoTime);
        }

        inline UObject*& lastFirstPersonConvoActor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.lastFirstPersonConvoActor);
        }

        inline float& lastFirstPersonConvoTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.lastFirstPersonConvoTime);
        }

        inline uint32_t& bStartingNewGame(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bStartingNewGame);
        }

        inline uint32_t& bSavingSkillsAugs(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bSavingSkillsAugs);
        }

        inline uint32_t& bSpyDroneActive(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bSpyDroneActive);
        }

        inline int32_t& spyDroneLevel(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexplayer.spyDroneLevel);
        }

        inline float& spyDroneLevelValue(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.spyDroneLevelValue);
        }

        inline UObject*& aDrone(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.aDrone);
        }

        inline uint32_t& bBuySkills(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bBuySkills);
        }

        inline uint32_t& bKillerProfile(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bKillerProfile);
        }

        inline int32_t& mpMsgFlags(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexplayer.mpMsgFlags);
        }

        inline int32_t& mpMsgServerFlags(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexplayer.mpMsgServerFlags);
        }

        inline int32_t& mpMsgCode(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexplayer.mpMsgCode);
        }

        inline float& mpMsgTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.mpMsgTime);
        }

        inline int32_t& mpMsgOptionalParam(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexplayer.mpMsgOptionalParam);
        }

        inline std::string& mpMsgOptionalString(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.mpMsgOptionalString);
        }

        inline std::string& strStartMap(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.strStartMap);
        }

        inline uint32_t& bStartNewGameAfterIntro(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bStartNewGameAfterIntro);
        }

        inline uint32_t& bIgnoreNextShowMenu(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bIgnoreNextShowMenu);
        }

        inline std::string& NextMap(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.NextMap);
        }

        inline uint32_t& bObjectNames(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bObjectNames);
        }

        inline uint32_t& bNPCHighlighting(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bNPCHighlighting);
        }

        inline uint32_t& bSubtitles(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bSubtitles);
        }

        inline uint32_t& bAlwaysRun(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bAlwaysRun);
        }

        inline uint32_t& bToggleCrouch(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bToggleCrouch);
        }

        inline float& logTimeout(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.logTimeout);
        }

        inline uint8_t& maxLogLines(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_deusexplayer.maxLogLines);
        }

        inline uint32_t& bHelpMessages(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bHelpMessages);
        }

        inline uint8_t& translucencyLevel(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_deusexplayer.translucencyLevel);
        }

        inline uint32_t& bObjectBeltVisible(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bObjectBeltVisible);
        }

        inline uint32_t& bHitDisplayVisible(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bHitDisplayVisible);
        }

        inline uint32_t& bAmmoDisplayVisible(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bAmmoDisplayVisible);
        }

        inline uint32_t& bAugDisplayVisible(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bAugDisplayVisible);
        }

        inline uint32_t& bDisplayAmmoByClip(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bDisplayAmmoByClip);
        }

        inline uint32_t& bCompassVisible(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bCompassVisible);
        }

        inline uint32_t& bCrosshairVisible(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bCrosshairVisible);
        }

        inline uint32_t& bAutoReload(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bAutoReload);
        }

        inline uint32_t& bDisplayAllGoals(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bDisplayAllGoals);
        }

        inline uint32_t& bHUDShowAllAugs(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bHUDShowAllAugs);
        }

        inline int32_t& UIBackground(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexplayer.UIBackground);
        }

        inline uint32_t& bDisplayCompletedGoals(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bDisplayCompletedGoals);
        }

        inline uint32_t& bShowAmmoDescriptions(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bShowAmmoDescriptions);
        }

        inline uint32_t& bConfirmSaveDeletes(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bConfirmSaveDeletes);
        }

        inline uint32_t& bConfirmNoteDeletes(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bConfirmNoteDeletes);
        }

        inline uint32_t& bAskedToTrain(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bAskedToTrain);
        }

        inline Name& AugPrefs(UObject* o, int index)
        {
            return reinterpret_cast<Name*>(o->data + offsets_deusexplayer.AugPrefs)[index];
        }

        inline UObject*& BarkManager(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.BarkManager);
        }

        inline UObject*& ThemeManager(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.ThemeManager);
        }

        inline std::string& MenuThemeName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.MenuThemeName);
        }

        inline std::string& HUDThemeName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.HUDThemeName);
        }

        inline uint32_t& bHUDBordersVisible(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bHUDBordersVisible);
        }

        inline uint32_t& bHUDBordersTranslucent(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bHUDBordersTranslucent);
        }

        inline uint32_t& bHUDBackgroundTranslucent(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bHUDBackgroundTranslucent);
        }

        inline uint32_t& bMenusTranslucent(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bMenusTranslucent);
        }

        inline std::string& InventoryFull(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.InventoryFull);
        }

        inline std::string& TooMuchAmmo(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.TooMuchAmmo);
        }

        inline std::string& TooHeavyToLift(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.TooHeavyToLift);
        }

        inline std::string& CannotLift(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.CannotLift);
        }

        inline std::string& NoRoomToLift(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.NoRoomToLift);
        }

        inline std::string& CanCarryOnlyOne(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.CanCarryOnlyOne);
        }

        inline std::string& CannotDropHere(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.CannotDropHere);
        }

        inline std::string& HandsFull(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.HandsFull);
        }

        inline std::string& NoteAdded(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.NoteAdded);
        }

        inline std::string& GoalAdded(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.GoalAdded);
        }

        inline std::string& PrimaryGoalCompleted(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.PrimaryGoalCompleted);
        }

        inline std::string& SecondaryGoalCompleted(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.SecondaryGoalCompleted);
        }

        inline std::string& EnergyDepleted(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.EnergyDepleted);
        }

        inline std::string& AddedNanoKey(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.AddedNanoKey);
        }

        inline std::string& HealedPointsLabel(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.HealedPointsLabel);
        }

        inline std::string& HealedPointLabel(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.HealedPointLabel);
        }

        inline std::string& SkillPointsAward(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.SkillPointsAward);
        }

        inline std::string& QuickSaveGameTitle(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.QuickSaveGameTitle);
        }

        inline std::string& WeaponUnCloak(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.WeaponUnCloak);
        }

        inline std::string& TakenOverString(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.TakenOverString);
        }

        inline std::string& HeadString(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.HeadString);
        }

        inline std::string& TorsoString(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.TorsoString);
        }

        inline std::string& LegsString(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.LegsString);
        }

        inline std::string& WithTheString(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.WithTheString);
        }

        inline std::string& WithString(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.WithString);
        }

        inline std::string& PoisonString(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.PoisonString);
        }

        inline std::string& BurnString(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.BurnString);
        }

        inline std::string& NoneString(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexplayer.NoneString);
        }

        inline UObject*& DamageShield(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.DamageShield);
        }

        inline float& ShieldTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.ShieldTimer);
        }

        inline uint8_t& ShieldStatus(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_deusexplayer.ShieldStatus);
        }

        inline UObject*& myBurner(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.myBurner);
        }

        inline UObject*& myPoisoner(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.myPoisoner);
        }

        inline UObject*& myProjKiller(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.myProjKiller);
        }

        inline UObject*& myTurretKiller(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.myTurretKiller);
        }

        inline UObject*& myKiller(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.myKiller);
        }

        inline UObject*& killProfile(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.killProfile);
        }

        inline UObject*& invulnSph(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.invulnSph);
        }

        inline uint8_t& musicMode(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_deusexplayer.musicMode);
        }

        inline uint8_t& savedSection(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_deusexplayer.savedSection);
        }

        inline float& musicCheckTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.musicCheckTimer);
        }

        inline float& musicChangeTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.musicChangeTimer);
        }

        inline int32_t& saveCount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexplayer.saveCount);
        }

        inline float& saveTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.saveTime);
        }

        inline UObject*& GlobalDebugObj(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.GlobalDebugObj);
        }

        inline uint32_t& bQuotesEnabled(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bQuotesEnabled);
        }

        inline UObject*& DXGame(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.DXGame);
        }

        inline float& ServerTimeDiff(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.ServerTimeDiff);
        }

        inline float& ServerTimeLastRefresh(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.ServerTimeLastRefresh);
        }

        inline float& MPDamageMult(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.MPDamageMult);
        }

        inline float& NintendoImmunityTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.NintendoImmunityTime);
        }

        inline float& NintendoImmunityTimeLeft(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexplayer.NintendoImmunityTimeLeft);
        }

        inline uint32_t& bNintendoImmunity(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexplayer.bNintendoImmunity);
        }

        inline UObject*& ActiveComputer(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexplayer.ActiveComputer);
        }

    }

    namespace scriptedpawn {
        struct Offsets {
            size_t lastPoints = 0;
            size_t Restlessness = 0;
            size_t Wanderlust = 0;
            size_t Cowardice = 0;
            size_t BaseAccuracy = 0;
            size_t maxRange = 0;
            size_t MinRange = 0;
            size_t MinHealth = 0;
            size_t RandomWandering = 0;
            size_t sleepTime = 0;
            size_t destPoint = 0;
            size_t destLoc = 0;
            size_t useLoc = 0;
            size_t useRot = 0;
            size_t seekDistance = 0;
            size_t SeekLevel = 0;
            size_t SeekType = 0;
            size_t SeekPawn = 0;
            size_t CarcassTimer = 0;
            size_t CarcassHateTimer = 0;
            size_t CarcassCheckTimer = 0;
            size_t PotentialEnemyAlliance = 0;
            size_t PotentialEnemyTimer = 0;
            size_t BeamCheckTimer = 0;
            size_t bSeekPostCombat = 0;
            size_t bSeekLocation = 0;
            size_t bInterruptSeek = 0;
            size_t bAlliancesChanged = 0;
            size_t bNoNegativeAlliances = 0;
            size_t bSitAnywhere = 0;
            size_t bSitInterpolation = 0;
            size_t bStandInterpolation = 0;
            size_t remainingSitTime = 0;
            size_t remainingStandTime = 0;
            size_t StandRate = 0;
            size_t ReloadTimer = 0;
            size_t bReadyToReload = 0;
            size_t CarcassType = 0;
            size_t Orders = 0;
            size_t OrderTag = 0;
            size_t HomeTag = 0;
            size_t HomeExtent = 0;
            size_t OrderActor = 0;
            size_t NextAnim = 0;
            size_t WalkingSpeed = 0;
            size_t ProjectileSpeed = 0;
            size_t LastPainAnim = 0;
            size_t LastPainTime = 0;
            size_t DesiredPrePivot = 0;
            size_t PrePivotTime = 0;
            size_t PrePivotOffset = 0;
            size_t bCanBleed = 0;
            size_t BleedRate = 0;
            size_t DropCounter = 0;
            size_t ClotPeriod = 0;
            size_t bAcceptBump = 0;
            size_t bCanFire = 0;
            size_t bKeepWeaponDrawn = 0;
            size_t bShowPain = 0;
            size_t bCanSit = 0;
            size_t bAlwaysPatrol = 0;
            size_t bPlayIdle = 0;
            size_t bLeaveAfterFleeing = 0;
            size_t bLikesNeutral = 0;
            size_t bUseFirstSeatOnly = 0;
            size_t bCower = 0;
            size_t HomeActor = 0;
            size_t HomeLoc = 0;
            size_t HomeRot = 0;
            size_t bUseHome = 0;
            size_t bInterruptState = 0;
            size_t bCanConverse = 0;
            size_t bImportant = 0;
            size_t bInvincible = 0;
            size_t bInitialized = 0;
            size_t bAvoidAim = 0;
            size_t bAimForHead = 0;
            size_t bDefendHome = 0;
            size_t bCanCrouch = 0;
            size_t bSeekCover = 0;
            size_t bSprint = 0;
            size_t bUseFallbackWeapons = 0;
            size_t AvoidAccuracy = 0;
            size_t bAvoidHarm = 0;
            size_t HarmAccuracy = 0;
            size_t CrouchRate = 0;
            size_t SprintRate = 0;
            size_t CloseCombatMult = 0;
            size_t bHateHacking = 0;
            size_t bHateWeapon = 0;
            size_t bHateShot = 0;
            size_t bHateInjury = 0;
            size_t bHateIndirectInjury = 0;
            size_t bHateCarcass = 0;
            size_t bHateDistress = 0;
            size_t bReactFutz = 0;
            size_t bReactPresence = 0;
            size_t bReactLoudNoise = 0;
            size_t bReactAlarm = 0;
            size_t bReactShot = 0;
            size_t bReactCarcass = 0;
            size_t bReactDistress = 0;
            size_t bReactProjectiles = 0;
            size_t bFearHacking = 0;
            size_t bFearWeapon = 0;
            size_t bFearShot = 0;
            size_t bFearInjury = 0;
            size_t bFearIndirectInjury = 0;
            size_t bFearCarcass = 0;
            size_t bFearDistress = 0;
            size_t bFearAlarm = 0;
            size_t bFearProjectiles = 0;
            size_t bEmitDistress = 0;
            size_t RaiseAlarm = 0;
            size_t bLookingForEnemy = 0;
            size_t bLookingForLoudNoise = 0;
            size_t bLookingForAlarm = 0;
            size_t bLookingForDistress = 0;
            size_t bLookingForProjectiles = 0;
            size_t bLookingForFutz = 0;
            size_t bLookingForHacking = 0;
            size_t bLookingForShot = 0;
            size_t bLookingForWeapon = 0;
            size_t bLookingForCarcass = 0;
            size_t bLookingForInjury = 0;
            size_t bLookingForIndirectInjury = 0;
            size_t bFacingTarget = 0;
            size_t bMustFaceTarget = 0;
            size_t FireAngle = 0;
            size_t FireElevation = 0;
            size_t MaxProvocations = 0;
            size_t AgitationSustainTime = 0;
            size_t AgitationDecayRate = 0;
            size_t AgitationTimer = 0;
            size_t AgitationCheckTimer = 0;
            size_t PlayerAgitationTimer = 0;
            size_t FearSustainTime = 0;
            size_t FearDecayRate = 0;
            size_t FearTimer = 0;
            size_t FearLevel = 0;
            size_t EnemyReadiness = 0;
            size_t ReactionLevel = 0;
            size_t SurprisePeriod = 0;
            size_t SightPercentage = 0;
            size_t CycleTimer = 0;
            size_t CyclePeriod = 0;
            size_t CycleCumulative = 0;
            size_t CycleCandidate = 0;
            size_t CycleDistance = 0;
            size_t AlarmActor = 0;
            size_t AlarmTimer = 0;
            size_t WeaponTimer = 0;
            size_t FireTimer = 0;
            size_t SpecialTimer = 0;
            size_t CrouchTimer = 0;
            size_t BackpedalTimer = 0;
            size_t bHasShadow = 0;
            size_t ShadowScale = 0;
            size_t bDisappear = 0;
            size_t bInTransientState = 0;
            size_t InitialAlliances = 0;
            size_t AlliancesEx = 0;
            size_t bReverseAlliances = 0;
            size_t BaseAssHeight = 0;
            size_t EnemyTimeout = 0;
            size_t CheckPeriod = 0;
            size_t EnemyLastSeen = 0;
            size_t SeatSlot = 0;
            size_t SeatActor = 0;
            size_t cycleIndex = 0;
            size_t BodyIndex = 0;
            size_t bRunningStealthy = 0;
            size_t bPausing = 0;
            size_t bStaring = 0;
            size_t bAttacking = 0;
            size_t bDistressed = 0;
            size_t bStunned = 0;
            size_t bSitting = 0;
            size_t bDancing = 0;
            size_t bCrouching = 0;
            size_t bCanTurnHead = 0;
            size_t bTickVisibleOnly = 0;
            size_t bInWorld = 0;
            size_t WorldPosition = 0;
            size_t bWorldCollideActors = 0;
            size_t bWorldBlockActors = 0;
            size_t bWorldBlockPlayers = 0;
            size_t bHighlight = 0;
            size_t bHokeyPokey = 0;
            size_t bConvEndState = 0;
            size_t InitialInventory = 0;
            size_t bConversationEndedNormally = 0;
            size_t bInConversation = 0;
            size_t ConversationActor = 0;
            size_t WalkSound = 0;
            size_t swimBubbleTimer = 0;
            size_t bSpawnBubbles = 0;
            size_t bUseSecondaryAttack = 0;
            size_t bWalkAround = 0;
            size_t bClearedObstacle = 0;
            size_t bEnableCheckDest = 0;
            size_t TurnDirection = 0;
            size_t NextDirection = 0;
            size_t ActorAvoiding = 0;
            size_t AvoidWallTimer = 0;
            size_t AvoidBumpTimer = 0;
            size_t ObstacleTimer = 0;
            size_t LastDestLoc = 0;
            size_t LastDestPoint = 0;
            size_t DestAttempts = 0;
            size_t DeathTimer = 0;
            size_t EnemyTimer = 0;
            size_t TakeHitTimer = 0;
            size_t ConvOrders = 0;
            size_t ConvOrderTag = 0;
            size_t BurnPeriod = 0;
            size_t FutzTimer = 0;
            size_t DistressTimer = 0;
            size_t SeatLocation = 0;
            size_t SeatHack = 0;
            size_t bSeatLocationValid = 0;
            size_t bSeatHackUsed = 0;
            size_t bBurnedToDeath = 0;
            size_t bHasCloak = 0;
            size_t bCloakOn = 0;
            size_t CloakThreshold = 0;
            size_t CloakEMPTimer = 0;
            size_t poisonTimer = 0;
            size_t poisonCounter = 0;
            size_t poisonDamage = 0;
            size_t poisoner = 0;
            size_t Carcasses = 0;
            size_t NumCarcasses = 0;
            size_t walkAnimMult = 0;
            size_t runAnimMult = 0;
        };
    }

    extern scriptedpawn::Offsets offsets_scriptedpawn;

    namespace scriptedpawn {
        inline UObject*& lastPoints(UObject* o, int index)
        {
            return reinterpret_cast<UObject**>(o->data + offsets_scriptedpawn.lastPoints)[index];
        }

        inline float& Restlessness(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.Restlessness);
        }

        inline float& Wanderlust(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.Wanderlust);
        }

        inline float& Cowardice(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.Cowardice);
        }

        inline float& BaseAccuracy(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.BaseAccuracy);
        }

        inline float& maxRange(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.maxRange);
        }

        inline float& MinRange(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.MinRange);
        }

        inline float& MinHealth(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.MinHealth);
        }

        inline float& RandomWandering(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.RandomWandering);
        }

        inline float& sleepTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.sleepTime);
        }

        inline UObject*& destPoint(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scriptedpawn.destPoint);
        }

        inline Vector& destLoc(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_scriptedpawn.destLoc);
        }

        inline Vector& useLoc(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_scriptedpawn.useLoc);
        }

        inline Rotator& useRot(UObject* o)
        {
            return *reinterpret_cast<Rotator*>(o->data + offsets_scriptedpawn.useRot);
        }

        inline float& seekDistance(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.seekDistance);
        }

        inline int32_t& SeekLevel(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_scriptedpawn.SeekLevel);
        }

        inline uint8_t& SeekType(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_scriptedpawn.SeekType);
        }

        inline UObject*& SeekPawn(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scriptedpawn.SeekPawn);
        }

        inline float& CarcassTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.CarcassTimer);
        }

        inline float& CarcassHateTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.CarcassHateTimer);
        }

        inline float& CarcassCheckTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.CarcassCheckTimer);
        }

        inline Name& PotentialEnemyAlliance(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_scriptedpawn.PotentialEnemyAlliance);
        }

        inline float& PotentialEnemyTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.PotentialEnemyTimer);
        }

        inline float& BeamCheckTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.BeamCheckTimer);
        }

        inline uint32_t& bSeekPostCombat(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bSeekPostCombat);
        }

        inline uint32_t& bSeekLocation(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bSeekLocation);
        }

        inline uint32_t& bInterruptSeek(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bInterruptSeek);
        }

        inline uint32_t& bAlliancesChanged(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bAlliancesChanged);
        }

        inline uint32_t& bNoNegativeAlliances(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bNoNegativeAlliances);
        }

        inline uint32_t& bSitAnywhere(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bSitAnywhere);
        }

        inline uint32_t& bSitInterpolation(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bSitInterpolation);
        }

        inline uint32_t& bStandInterpolation(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bStandInterpolation);
        }

        inline float& remainingSitTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.remainingSitTime);
        }

        inline float& remainingStandTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.remainingStandTime);
        }

        inline Vector& StandRate(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_scriptedpawn.StandRate);
        }

        inline float& ReloadTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.ReloadTimer);
        }

        inline uint32_t& bReadyToReload(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bReadyToReload);
        }

        inline UObject*& CarcassType(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scriptedpawn.CarcassType);
        }

        inline Name& Orders(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_scriptedpawn.Orders);
        }

        inline Name& OrderTag(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_scriptedpawn.OrderTag);
        }

        inline Name& HomeTag(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_scriptedpawn.HomeTag);
        }

        inline float& HomeExtent(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.HomeExtent);
        }

        inline UObject*& OrderActor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scriptedpawn.OrderActor);
        }

        inline Name& NextAnim(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_scriptedpawn.NextAnim);
        }

        inline float& WalkingSpeed(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.WalkingSpeed);
        }

        inline float& ProjectileSpeed(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.ProjectileSpeed);
        }

        inline Name& LastPainAnim(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_scriptedpawn.LastPainAnim);
        }

        inline float& LastPainTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.LastPainTime);
        }

        inline Vector& DesiredPrePivot(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_scriptedpawn.DesiredPrePivot);
        }

        inline float& PrePivotTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.PrePivotTime);
        }

        inline Vector& PrePivotOffset(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_scriptedpawn.PrePivotOffset);
        }

        inline uint32_t& bCanBleed(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bCanBleed);
        }

        inline float& BleedRate(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.BleedRate);
        }

        inline float& DropCounter(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.DropCounter);
        }

        inline float& ClotPeriod(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.ClotPeriod);
        }

        inline uint32_t& bAcceptBump(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bAcceptBump);
        }

        inline uint32_t& bCanFire(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bCanFire);
        }

        inline uint32_t& bKeepWeaponDrawn(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bKeepWeaponDrawn);
        }

        inline uint32_t& bShowPain(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bShowPain);
        }

        inline uint32_t& bCanSit(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bCanSit);
        }

        inline uint32_t& bAlwaysPatrol(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bAlwaysPatrol);
        }

        inline uint32_t& bPlayIdle(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bPlayIdle);
        }

        inline uint32_t& bLeaveAfterFleeing(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bLeaveAfterFleeing);
        }

        inline uint32_t& bLikesNeutral(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bLikesNeutral);
        }

        inline uint32_t& bUseFirstSeatOnly(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bUseFirstSeatOnly);
        }

        inline uint32_t& bCower(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bCower);
        }

        inline UObject*& HomeActor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scriptedpawn.HomeActor);
        }

        inline Vector& HomeLoc(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_scriptedpawn.HomeLoc);
        }

        inline Vector& HomeRot(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_scriptedpawn.HomeRot);
        }

        inline uint32_t& bUseHome(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bUseHome);
        }

        inline uint32_t& bInterruptState(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bInterruptState);
        }

        inline uint32_t& bCanConverse(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bCanConverse);
        }

        inline uint32_t& bImportant(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bImportant);
        }

        inline uint32_t& bInvincible(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bInvincible);
        }

        inline uint32_t& bInitialized(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bInitialized);
        }

        inline uint32_t& bAvoidAim(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bAvoidAim);
        }

        inline uint32_t& bAimForHead(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bAimForHead);
        }

        inline uint32_t& bDefendHome(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bDefendHome);
        }

        inline uint32_t& bCanCrouch(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bCanCrouch);
        }

        inline uint32_t& bSeekCover(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bSeekCover);
        }

        inline uint32_t& bSprint(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bSprint);
        }

        inline uint32_t& bUseFallbackWeapons(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bUseFallbackWeapons);
        }

        inline float& AvoidAccuracy(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.AvoidAccuracy);
        }

        inline uint32_t& bAvoidHarm(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bAvoidHarm);
        }

        inline float& HarmAccuracy(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.HarmAccuracy);
        }

        inline float& CrouchRate(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.CrouchRate);
        }

        inline float& SprintRate(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.SprintRate);
        }

        inline float& CloseCombatMult(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.CloseCombatMult);
        }

        inline uint32_t& bHateHacking(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bHateHacking);
        }

        inline uint32_t& bHateWeapon(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bHateWeapon);
        }

        inline uint32_t& bHateShot(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bHateShot);
        }

        inline uint32_t& bHateInjury(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bHateInjury);
        }

        inline uint32_t& bHateIndirectInjury(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bHateIndirectInjury);
        }

        inline uint32_t& bHateCarcass(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bHateCarcass);
        }

        inline uint32_t& bHateDistress(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bHateDistress);
        }

        inline uint32_t& bReactFutz(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bReactFutz);
        }

        inline uint32_t& bReactPresence(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bReactPresence);
        }

        inline uint32_t& bReactLoudNoise(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bReactLoudNoise);
        }

        inline uint32_t& bReactAlarm(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bReactAlarm);
        }

        inline uint32_t& bReactShot(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bReactShot);
        }

        inline uint32_t& bReactCarcass(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bReactCarcass);
        }

        inline uint32_t& bReactDistress(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bReactDistress);
        }

        inline uint32_t& bReactProjectiles(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bReactProjectiles);
        }

        inline uint32_t& bFearHacking(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bFearHacking);
        }

        inline uint32_t& bFearWeapon(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bFearWeapon);
        }

        inline uint32_t& bFearShot(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bFearShot);
        }

        inline uint32_t& bFearInjury(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bFearInjury);
        }

        inline uint32_t& bFearIndirectInjury(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bFearIndirectInjury);
        }

        inline uint32_t& bFearCarcass(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bFearCarcass);
        }

        inline uint32_t& bFearDistress(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bFearDistress);
        }

        inline uint32_t& bFearAlarm(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bFearAlarm);
        }

        inline uint32_t& bFearProjectiles(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bFearProjectiles);
        }

        inline uint32_t& bEmitDistress(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bEmitDistress);
        }

        inline uint8_t& RaiseAlarm(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_scriptedpawn.RaiseAlarm);
        }

        inline uint32_t& bLookingForEnemy(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bLookingForEnemy);
        }

        inline uint32_t& bLookingForLoudNoise(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bLookingForLoudNoise);
        }

        inline uint32_t& bLookingForAlarm(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bLookingForAlarm);
        }

        inline uint32_t& bLookingForDistress(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bLookingForDistress);
        }

        inline uint32_t& bLookingForProjectiles(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bLookingForProjectiles);
        }

        inline uint32_t& bLookingForFutz(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bLookingForFutz);
        }

        inline uint32_t& bLookingForHacking(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bLookingForHacking);
        }

        inline uint32_t& bLookingForShot(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bLookingForShot);
        }

        inline uint32_t& bLookingForWeapon(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bLookingForWeapon);
        }

        inline uint32_t& bLookingForCarcass(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bLookingForCarcass);
        }

        inline uint32_t& bLookingForInjury(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bLookingForInjury);
        }

        inline uint32_t& bLookingForIndirectInjury(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bLookingForIndirectInjury);
        }

        inline uint32_t& bFacingTarget(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bFacingTarget);
        }

        inline uint32_t& bMustFaceTarget(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bMustFaceTarget);
        }

        inline float& FireAngle(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.FireAngle);
        }

        inline float& FireElevation(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.FireElevation);
        }

        inline int32_t& MaxProvocations(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_scriptedpawn.MaxProvocations);
        }

        inline float& AgitationSustainTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.AgitationSustainTime);
        }

        inline float& AgitationDecayRate(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.AgitationDecayRate);
        }

        inline float& AgitationTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.AgitationTimer);
        }

        inline float& AgitationCheckTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.AgitationCheckTimer);
        }

        inline float& PlayerAgitationTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.PlayerAgitationTimer);
        }

        inline float& FearSustainTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.FearSustainTime);
        }

        inline float& FearDecayRate(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.FearDecayRate);
        }

        inline float& FearTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.FearTimer);
        }

        inline float& FearLevel(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.FearLevel);
        }

        inline float& EnemyReadiness(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.EnemyReadiness);
        }

        inline float& ReactionLevel(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.ReactionLevel);
        }

        inline float& SurprisePeriod(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.SurprisePeriod);
        }

        inline float& SightPercentage(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.SightPercentage);
        }

        inline float& CycleTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.CycleTimer);
        }

        inline float& CyclePeriod(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.CyclePeriod);
        }

        inline float& CycleCumulative(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.CycleCumulative);
        }

        inline UObject*& CycleCandidate(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scriptedpawn.CycleCandidate);
        }

        inline float& CycleDistance(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.CycleDistance);
        }

        inline UObject*& AlarmActor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scriptedpawn.AlarmActor);
        }

        inline float& AlarmTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.AlarmTimer);
        }

        inline float& WeaponTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.WeaponTimer);
        }

        inline float& FireTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.FireTimer);
        }

        inline float& SpecialTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.SpecialTimer);
        }

        inline float& CrouchTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.CrouchTimer);
        }

        inline float& BackpedalTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.BackpedalTimer);
        }

        inline uint32_t& bHasShadow(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bHasShadow);
        }

        inline float& ShadowScale(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.ShadowScale);
        }

        inline uint32_t& bDisappear(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bDisappear);
        }

        inline uint32_t& bInTransientState(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bInTransientState);
        }

        inline uint8_t* InitialAlliances(UObject* o)
        {
            return o->data + offsets_scriptedpawn.InitialAlliances;
        }

        inline uint8_t* AlliancesEx(UObject* o)
        {
            return o->data + offsets_scriptedpawn.AlliancesEx;
        }

        inline uint32_t& bReverseAlliances(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bReverseAlliances);
        }

        inline float& BaseAssHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.BaseAssHeight);
        }

        inline float& EnemyTimeout(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.EnemyTimeout);
        }

        inline float& CheckPeriod(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.CheckPeriod);
        }

        inline float& EnemyLastSeen(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.EnemyLastSeen);
        }

        inline int32_t& SeatSlot(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_scriptedpawn.SeatSlot);
        }

        inline UObject*& SeatActor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scriptedpawn.SeatActor);
        }

        inline int32_t& cycleIndex(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_scriptedpawn.cycleIndex);
        }

        inline int32_t& BodyIndex(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_scriptedpawn.BodyIndex);
        }

        inline uint32_t& bRunningStealthy(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bRunningStealthy);
        }

        inline uint32_t& bPausing(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bPausing);
        }

        inline uint32_t& bStaring(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bStaring);
        }

        inline uint32_t& bAttacking(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bAttacking);
        }

        inline uint32_t& bDistressed(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bDistressed);
        }

        inline uint32_t& bStunned(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bStunned);
        }

        inline uint32_t& bSitting(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bSitting);
        }

        inline uint32_t& bDancing(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bDancing);
        }

        inline uint32_t& bCrouching(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bCrouching);
        }

        inline uint32_t& bCanTurnHead(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bCanTurnHead);
        }

        inline uint32_t& bTickVisibleOnly(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bTickVisibleOnly);
        }

        inline uint32_t& bInWorld(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bInWorld);
        }

        inline Vector& WorldPosition(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_scriptedpawn.WorldPosition);
        }

        inline uint32_t& bWorldCollideActors(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bWorldCollideActors);
        }

        inline uint32_t& bWorldBlockActors(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bWorldBlockActors);
        }

        inline uint32_t& bWorldBlockPlayers(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bWorldBlockPlayers);
        }

        inline uint32_t& bHighlight(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bHighlight);
        }

        inline uint32_t& bHokeyPokey(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bHokeyPokey);
        }

        inline uint32_t& bConvEndState(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bConvEndState);
        }

        inline uint8_t* InitialInventory(UObject* o)
        {
            return o->data + offsets_scriptedpawn.InitialInventory;
        }

        inline uint32_t& bConversationEndedNormally(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bConversationEndedNormally);
        }

        inline uint32_t& bInConversation(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bInConversation);
        }

        inline UObject*& ConversationActor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scriptedpawn.ConversationActor);
        }

        inline UObject*& WalkSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scriptedpawn.WalkSound);
        }

        inline float& swimBubbleTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.swimBubbleTimer);
        }

        inline uint32_t& bSpawnBubbles(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bSpawnBubbles);
        }

        inline uint32_t& bUseSecondaryAttack(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bUseSecondaryAttack);
        }

        inline uint32_t& bWalkAround(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bWalkAround);
        }

        inline uint32_t& bClearedObstacle(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bClearedObstacle);
        }

        inline uint32_t& bEnableCheckDest(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bEnableCheckDest);
        }

        inline uint8_t& TurnDirection(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_scriptedpawn.TurnDirection);
        }

        inline uint8_t& NextDirection(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_scriptedpawn.NextDirection);
        }

        inline UObject*& ActorAvoiding(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scriptedpawn.ActorAvoiding);
        }

        inline float& AvoidWallTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.AvoidWallTimer);
        }

        inline float& AvoidBumpTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.AvoidBumpTimer);
        }

        inline float& ObstacleTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.ObstacleTimer);
        }

        inline Vector& LastDestLoc(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_scriptedpawn.LastDestLoc);
        }

        inline Vector& LastDestPoint(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_scriptedpawn.LastDestPoint);
        }

        inline int32_t& DestAttempts(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_scriptedpawn.DestAttempts);
        }

        inline float& DeathTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.DeathTimer);
        }

        inline float& EnemyTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.EnemyTimer);
        }

        inline float& TakeHitTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.TakeHitTimer);
        }

        inline Name& ConvOrders(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_scriptedpawn.ConvOrders);
        }

        inline Name& ConvOrderTag(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_scriptedpawn.ConvOrderTag);
        }

        inline float& BurnPeriod(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.BurnPeriod);
        }

        inline float& FutzTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.FutzTimer);
        }

        inline float& DistressTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.DistressTimer);
        }

        inline Vector& SeatLocation(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_scriptedpawn.SeatLocation);
        }

        inline UObject*& SeatHack(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scriptedpawn.SeatHack);
        }

        inline uint32_t& bSeatLocationValid(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bSeatLocationValid);
        }

        inline uint32_t& bSeatHackUsed(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bSeatHackUsed);
        }

        inline uint32_t& bBurnedToDeath(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bBurnedToDeath);
        }

        inline uint32_t& bHasCloak(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bHasCloak);
        }

        inline uint32_t& bCloakOn(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scriptedpawn.bCloakOn);
        }

        inline int32_t& CloakThreshold(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_scriptedpawn.CloakThreshold);
        }

        inline float& CloakEMPTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.CloakEMPTimer);
        }

        inline float& poisonTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.poisonTimer);
        }

        inline int32_t& poisonCounter(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_scriptedpawn.poisonCounter);
        }

        inline int32_t& poisonDamage(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_scriptedpawn.poisonDamage);
        }

        inline UObject*& poisoner(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scriptedpawn.poisoner);
        }

        inline Name& Carcasses(UObject* o, int index)
        {
            return reinterpret_cast<Name*>(o->data + offsets_scriptedpawn.Carcasses)[index];
        }

        inline int32_t& NumCarcasses(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_scriptedpawn.NumCarcasses);
        }

        inline float& walkAnimMult(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.walkAnimMult);
        }

        inline float& runAnimMult(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scriptedpawn.runAnimMult);
        }

    }

    namespace deusexdecoration {
        struct Offsets {
            size_t HitPoints = 0;
            size_t minDamageThreshold = 0;
            size_t bInvincible = 0;
            size_t FragType = 0;
            size_t bFloating = 0;
            size_t origRot = 0;
            size_t moverTag = 0;
            size_t bFlammable = 0;
            size_t Flammability = 0;
            size_t bExplosive = 0;
            size_t explosionDamage = 0;
            size_t explosionRadius = 0;
            size_t bHighlight = 0;
            size_t bCanBeBase = 0;
            size_t bGenerateFlies = 0;
            size_t pushSoundId = 0;
            size_t gradualHurtSteps = 0;
            size_t gradualHurtCounter = 0;
            size_t NextState = 0;
            size_t NextLabel = 0;
            size_t flyGen = 0;
            size_t ItemArticle = 0;
            size_t ItemName = 0;
        };
    }

    extern deusexdecoration::Offsets offsets_deusexdecoration;

    namespace deusexdecoration {
        inline int32_t& HitPoints(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexdecoration.HitPoints);
        }

        inline int32_t& minDamageThreshold(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexdecoration.minDamageThreshold);
        }

        inline uint32_t& bInvincible(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexdecoration.bInvincible);
        }

        inline UObject*& FragType(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexdecoration.FragType);
        }

        inline uint32_t& bFloating(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexdecoration.bFloating);
        }

        inline Rotator& origRot(UObject* o)
        {
            return *reinterpret_cast<Rotator*>(o->data + offsets_deusexdecoration.origRot);
        }

        inline Name& moverTag(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_deusexdecoration.moverTag);
        }

        inline uint32_t& bFlammable(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexdecoration.bFlammable);
        }

        inline float& Flammability(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexdecoration.Flammability);
        }

        inline uint32_t& bExplosive(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexdecoration.bExplosive);
        }

        inline int32_t& explosionDamage(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexdecoration.explosionDamage);
        }

        inline float& explosionRadius(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexdecoration.explosionRadius);
        }

        inline uint32_t& bHighlight(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexdecoration.bHighlight);
        }

        inline uint32_t& bCanBeBase(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexdecoration.bCanBeBase);
        }

        inline uint32_t& bGenerateFlies(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexdecoration.bGenerateFlies);
        }

        inline int32_t& pushSoundId(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexdecoration.pushSoundId);
        }

        inline int32_t& gradualHurtSteps(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexdecoration.gradualHurtSteps);
        }

        inline int32_t& gradualHurtCounter(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexdecoration.gradualHurtCounter);
        }

        inline Name& NextState(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_deusexdecoration.NextState);
        }

        inline Name& NextLabel(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_deusexdecoration.NextLabel);
        }

        inline UObject*& flyGen(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexdecoration.flyGen);
        }

        inline std::string& ItemArticle(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexdecoration.ItemArticle);
        }

        inline std::string& ItemName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexdecoration.ItemName);
        }

    }

    namespace deusexsaveinfo {
        struct Offsets {
            size_t Year = 0;
            size_t Month = 0;
            size_t Day = 0;
            size_t Hour = 0;
            size_t Minute = 0;
            size_t Second = 0;
            size_t DirectoryIndex = 0;
            size_t Description = 0;
            size_t MissionLocation = 0;
            size_t MapName = 0;
            size_t Snapshot = 0;
            size_t saveCount = 0;
            size_t saveTime = 0;
            size_t bCheatsEnabled = 0;
        };
    }

    extern deusexsaveinfo::Offsets offsets_deusexsaveinfo;

    namespace deusexsaveinfo {
        inline int32_t& Year(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexsaveinfo.Year);
        }

        inline int32_t& Month(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexsaveinfo.Month);
        }

        inline int32_t& Day(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexsaveinfo.Day);
        }

        inline int32_t& Hour(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexsaveinfo.Hour);
        }

        inline int32_t& Minute(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexsaveinfo.Minute);
        }

        inline int32_t& Second(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexsaveinfo.Second);
        }

        inline int32_t& DirectoryIndex(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexsaveinfo.DirectoryIndex);
        }

        inline std::string& Description(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexsaveinfo.Description);
        }

        inline std::string& MissionLocation(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexsaveinfo.MissionLocation);
        }

        inline std::string& MapName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexsaveinfo.MapName);
        }

        inline UObject*& Snapshot(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexsaveinfo.Snapshot);
        }

        inline int32_t& saveCount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexsaveinfo.saveCount);
        }

        inline int32_t& saveTime(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexsaveinfo.saveTime);
        }

        inline uint32_t& bCheatsEnabled(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexsaveinfo.bCheatsEnabled);
        }

    }

    namespace gamedirectory {
        struct Offsets {
            size_t gameDirectoryType = 0;
            size_t currentFilter = 0;
            size_t directoryList = 0;
            size_t loadedSaveInfoPointers = 0;
            size_t tempSaveInfo = 0;
        };
    }

    extern gamedirectory::Offsets offsets_gamedirectory;

    namespace gamedirectory {
        inline uint8_t& gameDirectoryType(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_gamedirectory.gameDirectoryType);
        }

        inline std::string& currentFilter(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_gamedirectory.currentFilter);
        }

        inline uint8_t* directoryList(UObject* o)
        {
            return o->data + offsets_gamedirectory.directoryList;
        }

        inline uint8_t* loadedSaveInfoPointers(UObject* o)
        {
            return o->data + offsets_gamedirectory.loadedSaveInfoPointers;
        }

        inline UObject*& tempSaveInfo(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_gamedirectory.tempSaveInfo);
        }

    }

    namespace dumplocation {
        struct Offsets {
            size_t currentDumpFileLocation = 0;
            size_t currentDumpLocation = 0;
            size_t dumpFileDirectory = 0;
            size_t currentDumpFileIndex = 0;
            size_t currentDumpLocationIndex = 0;
            size_t currentUser = 0;
            size_t dumpFile = 0;
            size_t dumpLocationCount = 0;
            size_t Player = 0;
        };
    }

    extern dumplocation::Offsets offsets_dumplocation;

    namespace dumplocation {
        inline int32_t& currentDumpFileLocation(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_dumplocation.currentDumpFileLocation);
        }

        inline uint8_t* currentDumpLocation(UObject* o)
        {
            return o->data + offsets_dumplocation.currentDumpLocation;
        }

        inline uint8_t* dumpFileDirectory(UObject* o)
        {
            return o->data + offsets_dumplocation.dumpFileDirectory;
        }

        inline int32_t& currentDumpFileIndex(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_dumplocation.currentDumpFileIndex);
        }

        inline int32_t& currentDumpLocationIndex(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_dumplocation.currentDumpLocationIndex);
        }

        inline std::string& currentUser(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_dumplocation.currentUser);
        }

        inline int32_t& dumpFile(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_dumplocation.dumpFile);
        }

        inline int32_t& dumpLocationCount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_dumplocation.dumpLocationCount);
        }

        inline UObject*& Player(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_dumplocation.Player);
        }

    }

    namespace particleiterator {
        struct Offsets {
            size_t Particles = 0;
            size_t nextFreeParticle = 0;
            size_t proxy = 0;
            size_t bOwnerUsesGravity = 0;
            size_t bOwnerScales = 0;
            size_t bOwnerFades = 0;
            size_t OwnerZoneGravity = 0;
            size_t OwnerRiseRate = 0;
            size_t OwnerLifeSpan = 0;
            size_t OwnerDrawScale = 0;
        };
    }

    extern particleiterator::Offsets offsets_particleiterator;

    namespace particleiterator {
        inline uint8_t* Particles(UObject* o)
        {
            return o->data + offsets_particleiterator.Particles;
        }

        inline int32_t& nextFreeParticle(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_particleiterator.nextFreeParticle);
        }

        inline UObject*& proxy(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_particleiterator.proxy);
        }

        inline uint32_t& bOwnerUsesGravity(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_particleiterator.bOwnerUsesGravity);
        }

        inline uint32_t& bOwnerScales(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_particleiterator.bOwnerScales);
        }

        inline uint32_t& bOwnerFades(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_particleiterator.bOwnerFades);
        }

        inline float& OwnerZoneGravity(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_particleiterator.OwnerZoneGravity);
        }

        inline float& OwnerRiseRate(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_particleiterator.OwnerRiseRate);
        }

        inline float& OwnerLifeSpan(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_particleiterator.OwnerLifeSpan);
        }

        inline float& OwnerDrawScale(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_particleiterator.OwnerDrawScale);
        }

    }

    namespace deusexlevelinfo {
        struct Offsets {
            size_t MapName = 0;
            size_t MapAuthor = 0;
            size_t MissionLocation = 0;
            size_t missionNumber = 0;
            size_t bMultiPlayerMap = 0;
            size_t Script = 0;
            size_t TrueNorth = 0;
            size_t startupMessage = 0;
            size_t ConversationPackage = 0;
        };
    }

    extern deusexlevelinfo::Offsets offsets_deusexlevelinfo;

    namespace deusexlevelinfo {
        inline std::string& MapName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexlevelinfo.MapName);
        }

        inline std::string& MapAuthor(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexlevelinfo.MapAuthor);
        }

        inline std::string& MissionLocation(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexlevelinfo.MissionLocation);
        }

        inline int32_t& missionNumber(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexlevelinfo.missionNumber);
        }

        inline uint32_t& bMultiPlayerMap(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexlevelinfo.bMultiPlayerMap);
        }

        inline UObject*& Script(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexlevelinfo.Script);
        }

        inline int32_t& TrueNorth(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexlevelinfo.TrueNorth);
        }

        inline std::string& startupMessage(UObject* o, int index)
        {
            return reinterpret_cast<std::string*>(o->data + offsets_deusexlevelinfo.startupMessage)[index];
        }

        inline std::string& ConversationPackage(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexlevelinfo.ConversationPackage);
        }

    }

    namespace deusexrootwindow {
        struct Offsets {
            size_t HUD = 0;
            size_t actorDisplay = 0;
            size_t scopeView = 0;
            size_t MaxWinStack = 0;
            size_t winStack = 0;
            size_t winCount = 0;
            size_t bIgnoreHotkeys = 0;
            size_t defaultMenuColorSchemes = 0;
            size_t defaultHUDColorSchemes = 0;
            size_t menuColorScheme = 0;
            size_t bMenuTranslucency = 0;
            size_t HUDColorScheme = 0;
            size_t bHUDTranslucency = 0;
            size_t colSnapshot = 0;
            size_t Snapshot = 0;
            size_t snapshotWidth = 0;
            size_t snapshotHeight = 0;
            size_t bUIPaused = 0;
            size_t DataVaultFunctions = 0;
            size_t QuickLoadTitle = 0;
            size_t QuickLoadMessage = 0;
        };
    }

    extern deusexrootwindow::Offsets offsets_deusexrootwindow;

    namespace deusexrootwindow {
        inline UObject*& HUD(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexrootwindow.HUD);
        }

        inline UObject*& actorDisplay(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexrootwindow.actorDisplay);
        }

        inline UObject*& scopeView(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexrootwindow.scopeView);
        }

        inline int32_t& MaxWinStack(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexrootwindow.MaxWinStack);
        }

        inline UObject*& winStack(UObject* o, int index)
        {
            return reinterpret_cast<UObject**>(o->data + offsets_deusexrootwindow.winStack)[index];
        }

        inline int32_t& winCount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexrootwindow.winCount);
        }

        inline uint32_t& bIgnoreHotkeys(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexrootwindow.bIgnoreHotkeys);
        }

        inline uint8_t* defaultMenuColorSchemes(UObject* o)
        {
            return o->data + offsets_deusexrootwindow.defaultMenuColorSchemes;
        }

        inline uint8_t* defaultHUDColorSchemes(UObject* o)
        {
            return o->data + offsets_deusexrootwindow.defaultHUDColorSchemes;
        }

        inline uint8_t* menuColorScheme(UObject* o)
        {
            return o->data + offsets_deusexrootwindow.menuColorScheme;
        }

        inline uint32_t& bMenuTranslucency(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexrootwindow.bMenuTranslucency);
        }

        inline uint8_t* HUDColorScheme(UObject* o)
        {
            return o->data + offsets_deusexrootwindow.HUDColorScheme;
        }

        inline uint32_t& bHUDTranslucency(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexrootwindow.bHUDTranslucency);
        }

        inline Color& colSnapshot(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_deusexrootwindow.colSnapshot);
        }

        inline UObject*& Snapshot(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexrootwindow.Snapshot);
        }

        inline float& snapshotWidth(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexrootwindow.snapshotWidth);
        }

        inline float& snapshotHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexrootwindow.snapshotHeight);
        }

        inline uint32_t& bUIPaused(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexrootwindow.bUIPaused);
        }

        inline uint8_t* DataVaultFunctions(UObject* o)
        {
            return o->data + offsets_deusexrootwindow.DataVaultFunctions;
        }

        inline std::string& QuickLoadTitle(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexrootwindow.QuickLoadTitle);
        }

        inline std::string& QuickLoadMessage(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexrootwindow.QuickLoadMessage);
        }

    }

    namespace deusexgameinfo {
        struct Offsets {
        };
    }

    extern deusexgameinfo::Offsets offsets_deusexgameinfo;

    namespace deusexgameinfo {

    }

    namespace deusexweapon {
        struct Offsets {
            size_t bReadyToFire = 0;
            size_t LowAmmoWaterMark = 0;
            size_t ClipCount = 0;
            size_t GoverningSkill = 0;
            size_t NoiseLevel = 0;
            size_t EnemyEffective = 0;
            size_t EnviroEffective = 0;
            size_t Concealability = 0;
            size_t bAutomatic = 0;
            size_t ShotTime = 0;
            size_t reloadTime = 0;
            size_t HitDamage = 0;
            size_t maxRange = 0;
            size_t AccurateRange = 0;
            size_t BaseAccuracy = 0;
            size_t bCanHaveScope = 0;
            size_t bHasScope = 0;
            size_t ScopeFOV = 0;
            size_t bZoomed = 0;
            size_t bWasZoomed = 0;
            size_t bCanHaveLaser = 0;
            size_t bHasLaser = 0;
            size_t bLasing = 0;
            size_t emitter = 0;
            size_t bCanHaveSilencer = 0;
            size_t bHasSilencer = 0;
            size_t bCanTrack = 0;
            size_t LockTime = 0;
            size_t LockTimer = 0;
            size_t MaintainLockTimer = 0;
            size_t LockTarget = 0;
            size_t Target = 0;
            size_t LockMode = 0;
            size_t TargetMessage = 0;
            size_t TargetRange = 0;
            size_t LockedSound = 0;
            size_t TrackingSound = 0;
            size_t SoundTimer = 0;
            size_t AmmoNames = 0;
            size_t ProjectileNames = 0;
            size_t AreaOfEffect = 0;
            size_t bPenetrating = 0;
            size_t StunDuration = 0;
            size_t bHasMuzzleFlash = 0;
            size_t bHandToHand = 0;
            size_t SwingOffset = 0;
            size_t recoilStrength = 0;
            size_t bFiring = 0;
            size_t bOwnerWillNotify = 0;
            size_t bFallbackWeapon = 0;
            size_t bNativeAttack = 0;
            size_t bEmitWeaponDrawn = 0;
            size_t bUseWhileCrouched = 0;
            size_t bUseAsDrawnWeapon = 0;
            size_t bWasInFiring = 0;
            size_t bNearWall = 0;
            size_t placeLocation = 0;
            size_t placeNormal = 0;
            size_t placeMover = 0;
            size_t shaketimer = 0;
            size_t ShakeYaw = 0;
            size_t ShakePitch = 0;
            size_t AIMinRange = 0;
            size_t AIMaxRange = 0;
            size_t AITimeLimit = 0;
            size_t AIFireDelay = 0;
            size_t standingTimer = 0;
            size_t currentAccuracy = 0;
            size_t flash = 0;
            size_t MinSpreadAcc = 0;
            size_t MinProjSpreadAcc = 0;
            size_t MinWeaponAcc = 0;
            size_t bNeedToSetMPPickupAmmo = 0;
            size_t bDestroyOnFinish = 0;
            size_t mpReloadTime = 0;
            size_t mpHitDamage = 0;
            size_t mpBaseAccuracy = 0;
            size_t mpAccurateRange = 0;
            size_t mpMaxRange = 0;
            size_t mpReloadCount = 0;
            size_t mpPickupAmmoCount = 0;
            size_t bCanHaveModBaseAccuracy = 0;
            size_t bCanHaveModReloadCount = 0;
            size_t bCanHaveModAccurateRange = 0;
            size_t bCanHaveModReloadTime = 0;
            size_t bCanHaveModRecoilStrength = 0;
            size_t ModBaseAccuracy = 0;
            size_t ModReloadCount = 0;
            size_t ModAccurateRange = 0;
            size_t ModReloadTime = 0;
            size_t ModRecoilStrength = 0;
            size_t msgCannotBeReloaded = 0;
            size_t msgOutOf = 0;
            size_t msgNowHas = 0;
            size_t msgAlreadyHas = 0;
            size_t msgNone = 0;
            size_t msgLockInvalid = 0;
            size_t msgLockRange = 0;
            size_t msgLockAcquire = 0;
            size_t msgLockLocked = 0;
            size_t msgRangeUnit = 0;
            size_t msgTimeUnit = 0;
            size_t msgMassUnit = 0;
            size_t msgNotWorking = 0;
            size_t msgInfoAmmoLoaded = 0;
            size_t msgInfoAmmo = 0;
            size_t msgInfoDamage = 0;
            size_t msgInfoClip = 0;
            size_t msgInfoROF = 0;
            size_t msgInfoReload = 0;
            size_t msgInfoRecoil = 0;
            size_t msgInfoAccuracy = 0;
            size_t msgInfoAccRange = 0;
            size_t msgInfoMaxRange = 0;
            size_t msgInfoMass = 0;
            size_t msgInfoLaser = 0;
            size_t msgInfoScope = 0;
            size_t msgInfoSilencer = 0;
            size_t msgInfoNA = 0;
            size_t msgInfoYes = 0;
            size_t msgInfoNo = 0;
            size_t msgInfoAuto = 0;
            size_t msgInfoSingle = 0;
            size_t msgInfoRounds = 0;
            size_t msgInfoRoundsPerSec = 0;
            size_t msgInfoSkill = 0;
            size_t msgInfoWeaponStats = 0;
            size_t bClientReadyToFire = 0;
            size_t bClientReady = 0;
            size_t bInProcess = 0;
            size_t bFlameOn = 0;
            size_t bLooping = 0;
            size_t SimClipCount = 0;
            size_t flameShotCount = 0;
            size_t SimAmmoAmount = 0;
            size_t TimeLockSet = 0;
        };
    }

    extern deusexweapon::Offsets offsets_deusexweapon;

    namespace deusexweapon {
        inline uint32_t& bReadyToFire(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bReadyToFire);
        }

        inline int32_t& LowAmmoWaterMark(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexweapon.LowAmmoWaterMark);
        }

        inline int32_t& ClipCount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexweapon.ClipCount);
        }

        inline UObject*& GoverningSkill(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexweapon.GoverningSkill);
        }

        inline float& NoiseLevel(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.NoiseLevel);
        }

        inline uint8_t& EnemyEffective(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_deusexweapon.EnemyEffective);
        }

        inline uint8_t& EnviroEffective(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_deusexweapon.EnviroEffective);
        }

        inline uint8_t& Concealability(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_deusexweapon.Concealability);
        }

        inline uint32_t& bAutomatic(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bAutomatic);
        }

        inline float& ShotTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.ShotTime);
        }

        inline float& reloadTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.reloadTime);
        }

        inline int32_t& HitDamage(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexweapon.HitDamage);
        }

        inline int32_t& maxRange(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexweapon.maxRange);
        }

        inline int32_t& AccurateRange(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexweapon.AccurateRange);
        }

        inline float& BaseAccuracy(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.BaseAccuracy);
        }

        inline uint32_t& bCanHaveScope(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bCanHaveScope);
        }

        inline uint32_t& bHasScope(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bHasScope);
        }

        inline int32_t& ScopeFOV(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexweapon.ScopeFOV);
        }

        inline uint32_t& bZoomed(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bZoomed);
        }

        inline uint32_t& bWasZoomed(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bWasZoomed);
        }

        inline uint32_t& bCanHaveLaser(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bCanHaveLaser);
        }

        inline uint32_t& bHasLaser(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bHasLaser);
        }

        inline uint32_t& bLasing(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bLasing);
        }

        inline UObject*& emitter(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexweapon.emitter);
        }

        inline uint32_t& bCanHaveSilencer(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bCanHaveSilencer);
        }

        inline uint32_t& bHasSilencer(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bHasSilencer);
        }

        inline uint32_t& bCanTrack(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bCanTrack);
        }

        inline float& LockTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.LockTime);
        }

        inline float& LockTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.LockTimer);
        }

        inline float& MaintainLockTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.MaintainLockTimer);
        }

        inline UObject*& LockTarget(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexweapon.LockTarget);
        }

        inline UObject*& Target(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexweapon.Target);
        }

        inline uint8_t& LockMode(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_deusexweapon.LockMode);
        }

        inline std::string& TargetMessage(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.TargetMessage);
        }

        inline float& TargetRange(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.TargetRange);
        }

        inline UObject*& LockedSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexweapon.LockedSound);
        }

        inline UObject*& TrackingSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexweapon.TrackingSound);
        }

        inline float& SoundTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.SoundTimer);
        }

        inline UObject*& AmmoNames(UObject* o, int index)
        {
            return reinterpret_cast<UObject**>(o->data + offsets_deusexweapon.AmmoNames)[index];
        }

        inline UObject*& ProjectileNames(UObject* o, int index)
        {
            return reinterpret_cast<UObject**>(o->data + offsets_deusexweapon.ProjectileNames)[index];
        }

        inline uint8_t& AreaOfEffect(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_deusexweapon.AreaOfEffect);
        }

        inline uint32_t& bPenetrating(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bPenetrating);
        }

        inline float& StunDuration(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.StunDuration);
        }

        inline uint32_t& bHasMuzzleFlash(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bHasMuzzleFlash);
        }

        inline uint32_t& bHandToHand(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bHandToHand);
        }

        inline Vector& SwingOffset(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_deusexweapon.SwingOffset);
        }

        inline float& recoilStrength(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.recoilStrength);
        }

        inline uint32_t& bFiring(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bFiring);
        }

        inline uint32_t& bOwnerWillNotify(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bOwnerWillNotify);
        }

        inline uint32_t& bFallbackWeapon(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bFallbackWeapon);
        }

        inline uint32_t& bNativeAttack(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bNativeAttack);
        }

        inline uint32_t& bEmitWeaponDrawn(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bEmitWeaponDrawn);
        }

        inline uint32_t& bUseWhileCrouched(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bUseWhileCrouched);
        }

        inline uint32_t& bUseAsDrawnWeapon(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bUseAsDrawnWeapon);
        }

        inline uint32_t& bWasInFiring(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bWasInFiring);
        }

        inline uint32_t& bNearWall(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bNearWall);
        }

        inline Vector& placeLocation(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_deusexweapon.placeLocation);
        }

        inline Vector& placeNormal(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_deusexweapon.placeNormal);
        }

        inline UObject*& placeMover(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexweapon.placeMover);
        }

        inline float& shaketimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.shaketimer);
        }

        inline float& ShakeYaw(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.ShakeYaw);
        }

        inline float& ShakePitch(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.ShakePitch);
        }

        inline float& AIMinRange(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.AIMinRange);
        }

        inline float& AIMaxRange(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.AIMaxRange);
        }

        inline float& AITimeLimit(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.AITimeLimit);
        }

        inline float& AIFireDelay(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.AIFireDelay);
        }

        inline float& standingTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.standingTimer);
        }

        inline float& currentAccuracy(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.currentAccuracy);
        }

        inline UObject*& flash(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexweapon.flash);
        }

        inline float& MinSpreadAcc(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.MinSpreadAcc);
        }

        inline float& MinProjSpreadAcc(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.MinProjSpreadAcc);
        }

        inline float& MinWeaponAcc(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.MinWeaponAcc);
        }

        inline uint32_t& bNeedToSetMPPickupAmmo(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bNeedToSetMPPickupAmmo);
        }

        inline uint32_t& bDestroyOnFinish(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bDestroyOnFinish);
        }

        inline float& mpReloadTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.mpReloadTime);
        }

        inline int32_t& mpHitDamage(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexweapon.mpHitDamage);
        }

        inline float& mpBaseAccuracy(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.mpBaseAccuracy);
        }

        inline int32_t& mpAccurateRange(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexweapon.mpAccurateRange);
        }

        inline int32_t& mpMaxRange(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexweapon.mpMaxRange);
        }

        inline int32_t& mpReloadCount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexweapon.mpReloadCount);
        }

        inline int32_t& mpPickupAmmoCount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexweapon.mpPickupAmmoCount);
        }

        inline uint32_t& bCanHaveModBaseAccuracy(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bCanHaveModBaseAccuracy);
        }

        inline uint32_t& bCanHaveModReloadCount(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bCanHaveModReloadCount);
        }

        inline uint32_t& bCanHaveModAccurateRange(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bCanHaveModAccurateRange);
        }

        inline uint32_t& bCanHaveModReloadTime(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bCanHaveModReloadTime);
        }

        inline uint32_t& bCanHaveModRecoilStrength(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bCanHaveModRecoilStrength);
        }

        inline float& ModBaseAccuracy(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.ModBaseAccuracy);
        }

        inline float& ModReloadCount(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.ModReloadCount);
        }

        inline float& ModAccurateRange(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.ModAccurateRange);
        }

        inline float& ModReloadTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.ModReloadTime);
        }

        inline float& ModRecoilStrength(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.ModRecoilStrength);
        }

        inline std::string& msgCannotBeReloaded(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgCannotBeReloaded);
        }

        inline std::string& msgOutOf(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgOutOf);
        }

        inline std::string& msgNowHas(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgNowHas);
        }

        inline std::string& msgAlreadyHas(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgAlreadyHas);
        }

        inline std::string& msgNone(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgNone);
        }

        inline std::string& msgLockInvalid(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgLockInvalid);
        }

        inline std::string& msgLockRange(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgLockRange);
        }

        inline std::string& msgLockAcquire(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgLockAcquire);
        }

        inline std::string& msgLockLocked(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgLockLocked);
        }

        inline std::string& msgRangeUnit(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgRangeUnit);
        }

        inline std::string& msgTimeUnit(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgTimeUnit);
        }

        inline std::string& msgMassUnit(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgMassUnit);
        }

        inline std::string& msgNotWorking(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgNotWorking);
        }

        inline std::string& msgInfoAmmoLoaded(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgInfoAmmoLoaded);
        }

        inline std::string& msgInfoAmmo(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgInfoAmmo);
        }

        inline std::string& msgInfoDamage(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgInfoDamage);
        }

        inline std::string& msgInfoClip(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgInfoClip);
        }

        inline std::string& msgInfoROF(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgInfoROF);
        }

        inline std::string& msgInfoReload(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgInfoReload);
        }

        inline std::string& msgInfoRecoil(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgInfoRecoil);
        }

        inline std::string& msgInfoAccuracy(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgInfoAccuracy);
        }

        inline std::string& msgInfoAccRange(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgInfoAccRange);
        }

        inline std::string& msgInfoMaxRange(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgInfoMaxRange);
        }

        inline std::string& msgInfoMass(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgInfoMass);
        }

        inline std::string& msgInfoLaser(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgInfoLaser);
        }

        inline std::string& msgInfoScope(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgInfoScope);
        }

        inline std::string& msgInfoSilencer(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgInfoSilencer);
        }

        inline std::string& msgInfoNA(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgInfoNA);
        }

        inline std::string& msgInfoYes(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgInfoYes);
        }

        inline std::string& msgInfoNo(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgInfoNo);
        }

        inline std::string& msgInfoAuto(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgInfoAuto);
        }

        inline std::string& msgInfoSingle(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgInfoSingle);
        }

        inline std::string& msgInfoRounds(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgInfoRounds);
        }

        inline std::string& msgInfoRoundsPerSec(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgInfoRoundsPerSec);
        }

        inline std::string& msgInfoSkill(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgInfoSkill);
        }

        inline std::string& msgInfoWeaponStats(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexweapon.msgInfoWeaponStats);
        }

        inline uint32_t& bClientReadyToFire(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bClientReadyToFire);
        }

        inline uint32_t& bClientReady(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bClientReady);
        }

        inline uint32_t& bInProcess(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bInProcess);
        }

        inline uint32_t& bFlameOn(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bFlameOn);
        }

        inline uint32_t& bLooping(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexweapon.bLooping);
        }

        inline int32_t& SimClipCount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexweapon.SimClipCount);
        }

        inline int32_t& flameShotCount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexweapon.flameShotCount);
        }

        inline int32_t& SimAmmoAmount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexweapon.SimAmmoAmount);
        }

        inline float& TimeLockSet(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexweapon.TimeLockSet);
        }

    }

    namespace deusexcarcass {
        struct Offsets {
            size_t Mesh2 = 0;
            size_t Mesh3 = 0;
            size_t InitialInventory = 0;
            size_t bHighlight = 0;
            size_t KillerBindName = 0;
            size_t KillerAlliance = 0;
            size_t bGenerateFlies = 0;
            size_t flyGen = 0;
            size_t Alliance = 0;
            size_t CarcassName = 0;
            size_t MaxDamage = 0;
            size_t bNotDead = 0;
            size_t bEmitCarcass = 0;
            size_t bQueuedDestroy = 0;
            size_t bInit = 0;
            size_t bSearchMsgPrinted = 0;
            size_t msgSearching = 0;
            size_t msgEmpty = 0;
            size_t msgNotDead = 0;
            size_t msgAnimalCarcass = 0;
            size_t msgCannotPickup = 0;
            size_t msgRecharged = 0;
            size_t ItemName = 0;
            size_t bInvincible = 0;
            size_t bAnimalCarcass = 0;
        };
    }

    extern deusexcarcass::Offsets offsets_deusexcarcass;

    namespace deusexcarcass {
        inline UObject*& Mesh2(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexcarcass.Mesh2);
        }

        inline UObject*& Mesh3(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexcarcass.Mesh3);
        }

        inline uint8_t* InitialInventory(UObject* o)
        {
            return o->data + offsets_deusexcarcass.InitialInventory;
        }

        inline uint32_t& bHighlight(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexcarcass.bHighlight);
        }

        inline std::string& KillerBindName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexcarcass.KillerBindName);
        }

        inline Name& KillerAlliance(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_deusexcarcass.KillerAlliance);
        }

        inline uint32_t& bGenerateFlies(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexcarcass.bGenerateFlies);
        }

        inline UObject*& flyGen(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexcarcass.flyGen);
        }

        inline Name& Alliance(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_deusexcarcass.Alliance);
        }

        inline Name& CarcassName(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_deusexcarcass.CarcassName);
        }

        inline int32_t& MaxDamage(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexcarcass.MaxDamage);
        }

        inline uint32_t& bNotDead(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexcarcass.bNotDead);
        }

        inline uint32_t& bEmitCarcass(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexcarcass.bEmitCarcass);
        }

        inline uint32_t& bQueuedDestroy(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexcarcass.bQueuedDestroy);
        }

        inline uint32_t& bInit(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexcarcass.bInit);
        }

        inline uint32_t& bSearchMsgPrinted(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexcarcass.bSearchMsgPrinted);
        }

        inline std::string& msgSearching(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexcarcass.msgSearching);
        }

        inline std::string& msgEmpty(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexcarcass.msgEmpty);
        }

        inline std::string& msgNotDead(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexcarcass.msgNotDead);
        }

        inline std::string& msgAnimalCarcass(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexcarcass.msgAnimalCarcass);
        }

        inline std::string& msgCannotPickup(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexcarcass.msgCannotPickup);
        }

        inline std::string& msgRecharged(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexcarcass.msgRecharged);
        }

        inline std::string& ItemName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexcarcass.ItemName);
        }

        inline uint32_t& bInvincible(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexcarcass.bInvincible);
        }

        inline uint32_t& bAnimalCarcass(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexcarcass.bAnimalCarcass);
        }

    }

    namespace deusexpickup {
        struct Offsets {
            size_t bBreakable = 0;
            size_t FragType = 0;
            size_t maxCopies = 0;
            size_t CountLabel = 0;
            size_t msgTooMany = 0;
        };
    }

    extern deusexpickup::Offsets offsets_deusexpickup;

    namespace deusexpickup {
        inline uint32_t& bBreakable(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexpickup.bBreakable);
        }

        inline UObject*& FragType(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexpickup.FragType);
        }

        inline int32_t& maxCopies(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexpickup.maxCopies);
        }

        inline std::string& CountLabel(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexpickup.CountLabel);
        }

        inline std::string& msgTooMany(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexpickup.msgTooMany);
        }

    }

    namespace deusexprojectile {
        struct Offsets {
            size_t bExplodes = 0;
            size_t bBlood = 0;
            size_t bDebris = 0;
            size_t bStickToWall = 0;
            size_t bStuck = 0;
            size_t initDir = 0;
            size_t blastRadius = 0;
            size_t damagee = 0;
            size_t DamageType = 0;
            size_t AccurateRange = 0;
            size_t maxRange = 0;
            size_t initLoc = 0;
            size_t bTracking = 0;
            size_t Target = 0;
            size_t Time = 0;
            size_t MinDrawScale = 0;
            size_t maxDrawScale = 0;
            size_t LastSeenLoc = 0;
            size_t NetworkTargetLoc = 0;
            size_t bHasNetworkTarget = 0;
            size_t bHadLocalTarget = 0;
            size_t gradualHurtSteps = 0;
            size_t gradualHurtCounter = 0;
            size_t bEmitDanger = 0;
            size_t spawnWeaponClass = 0;
            size_t spawnAmmoClass = 0;
            size_t bIgnoresNanoDefense = 0;
            size_t bAggressiveExploded = 0;
            size_t ItemName = 0;
            size_t ItemArticle = 0;
        };
    }

    extern deusexprojectile::Offsets offsets_deusexprojectile;

    namespace deusexprojectile {
        inline uint32_t& bExplodes(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexprojectile.bExplodes);
        }

        inline uint32_t& bBlood(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexprojectile.bBlood);
        }

        inline uint32_t& bDebris(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexprojectile.bDebris);
        }

        inline uint32_t& bStickToWall(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexprojectile.bStickToWall);
        }

        inline uint32_t& bStuck(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexprojectile.bStuck);
        }

        inline Vector& initDir(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_deusexprojectile.initDir);
        }

        inline float& blastRadius(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexprojectile.blastRadius);
        }

        inline UObject*& damagee(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexprojectile.damagee);
        }

        inline Name& DamageType(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_deusexprojectile.DamageType);
        }

        inline int32_t& AccurateRange(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexprojectile.AccurateRange);
        }

        inline int32_t& maxRange(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexprojectile.maxRange);
        }

        inline Vector& initLoc(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_deusexprojectile.initLoc);
        }

        inline uint32_t& bTracking(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexprojectile.bTracking);
        }

        inline UObject*& Target(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexprojectile.Target);
        }

        inline float& Time(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexprojectile.Time);
        }

        inline float& MinDrawScale(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexprojectile.MinDrawScale);
        }

        inline float& maxDrawScale(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexprojectile.maxDrawScale);
        }

        inline Vector& LastSeenLoc(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_deusexprojectile.LastSeenLoc);
        }

        inline Vector& NetworkTargetLoc(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_deusexprojectile.NetworkTargetLoc);
        }

        inline uint32_t& bHasNetworkTarget(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexprojectile.bHasNetworkTarget);
        }

        inline uint32_t& bHadLocalTarget(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexprojectile.bHadLocalTarget);
        }

        inline int32_t& gradualHurtSteps(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexprojectile.gradualHurtSteps);
        }

        inline int32_t& gradualHurtCounter(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexprojectile.gradualHurtCounter);
        }

        inline uint32_t& bEmitDanger(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexprojectile.bEmitDanger);
        }

        inline UObject*& spawnWeaponClass(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexprojectile.spawnWeaponClass);
        }

        inline UObject*& spawnAmmoClass(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexprojectile.spawnAmmoClass);
        }

        inline uint32_t& bIgnoresNanoDefense(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexprojectile.bIgnoresNanoDefense);
        }

        inline uint32_t& bAggressiveExploded(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexprojectile.bAggressiveExploded);
        }

        inline std::string& ItemName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexprojectile.ItemName);
        }

        inline std::string& ItemArticle(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexprojectile.ItemArticle);
        }

    }

    namespace conplay {
        struct Offsets {
            size_t currentEvent = 0;
            size_t conWinFirst = 0;
            size_t currentSpeaker = 0;
            size_t currentSpeakingTo = 0;
            size_t SetupInitialCamera = 0;
            size_t speakingActor = 0;
            size_t cameraInfo = 0;
            size_t randomCamera = 0;
            size_t interactiveCamera = 0;
            size_t pendingCameraEvent = 0;
            size_t bSetupInitialCamera = 0;
            size_t bEndingConversation = 0;
            size_t bWaitingForConWin = 0;
            size_t bActorsTurned = 0;
            size_t bSaveContinueSpeech = 0;
            size_t bSaveNoPlayedFlag = 0;
            size_t playerInHand = 0;
            size_t perCharDelay = 0;
            size_t minimumTextPause = 0;
            size_t ConversationSpeechFonts = 0;
            size_t ConversationNameFonts = 0;
        };
    }

    extern conplay::Offsets offsets_conplay;

    namespace conplay {
        inline UObject*& currentEvent(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_conplay.currentEvent);
        }

        inline UObject*& conWinFirst(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_conplay.conWinFirst);
        }

        inline UObject*& currentSpeaker(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_conplay.currentSpeaker);
        }

        inline UObject*& currentSpeakingTo(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_conplay.currentSpeakingTo);
        }

        inline uint32_t& SetupInitialCamera(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_conplay.SetupInitialCamera);
        }

        inline UObject*& speakingActor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_conplay.speakingActor);
        }

        inline UObject*& cameraInfo(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_conplay.cameraInfo);
        }

        inline uint32_t& randomCamera(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_conplay.randomCamera);
        }

        inline uint32_t& interactiveCamera(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_conplay.interactiveCamera);
        }

        inline UObject*& pendingCameraEvent(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_conplay.pendingCameraEvent);
        }

        inline uint32_t& bSetupInitialCamera(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_conplay.bSetupInitialCamera);
        }

        inline uint32_t& bEndingConversation(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_conplay.bEndingConversation);
        }

        inline uint32_t& bWaitingForConWin(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_conplay.bWaitingForConWin);
        }

        inline uint32_t& bActorsTurned(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_conplay.bActorsTurned);
        }

        inline uint32_t& bSaveContinueSpeech(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_conplay.bSaveContinueSpeech);
        }

        inline uint32_t& bSaveNoPlayedFlag(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_conplay.bSaveNoPlayedFlag);
        }

        inline UObject*& playerInHand(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_conplay.playerInHand);
        }

        inline float& perCharDelay(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_conplay.perCharDelay);
        }

        inline float& minimumTextPause(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_conplay.minimumTextPause);
        }

        inline UObject*& ConversationSpeechFonts(UObject* o, int index)
        {
            return reinterpret_cast<UObject**>(o->data + offsets_conplay.ConversationSpeechFonts)[index];
        }

        inline UObject*& ConversationNameFonts(UObject* o, int index)
        {
            return reinterpret_cast<UObject**>(o->data + offsets_conplay.ConversationNameFonts)[index];
        }

    }

    namespace deusexmover {
        struct Offsets {
            size_t bOneWay = 0;
            size_t bLocked = 0;
            size_t bPickable = 0;
            size_t lockStrength = 0;
            size_t initiallockStrength = 0;
            size_t bInitialLocked = 0;
            size_t bBreakable = 0;
            size_t doorStrength = 0;
            size_t KeyIDNeeded = 0;
            size_t bHighlight = 0;
            size_t bFrobbable = 0;
            size_t bPicking = 0;
            size_t pickValue = 0;
            size_t pickTime = 0;
            size_t numPicks = 0;
            size_t TicksSinceLastPick = 0;
            size_t TicksPerPick = 0;
            size_t LastTickTime = 0;
            size_t pickPlayer = 0;
            size_t curPick = 0;
            size_t minDamageThreshold = 0;
            size_t bDestroyed = 0;
            size_t NumFragments = 0;
            size_t FragmentScale = 0;
            size_t FragmentSpread = 0;
            size_t FragmentClass = 0;
            size_t FragmentTexture = 0;
            size_t bFragmentTranslucent = 0;
            size_t bFragmentUnlit = 0;
            size_t ExplodeSound1 = 0;
            size_t ExplodeSound2 = 0;
            size_t bDrawExplosion = 0;
            size_t bIsDoor = 0;
            size_t TimeSinceReset = 0;
            size_t TimeToReset = 0;
            size_t msgKeyLocked = 0;
            size_t msgKeyUnlocked = 0;
            size_t msgLockpickSuccess = 0;
            size_t msgOneWayFail = 0;
            size_t msgLocked = 0;
            size_t msgPicking = 0;
            size_t msgAlreadyUnlocked = 0;
            size_t msgNoNanoKey = 0;
        };
    }

    extern deusexmover::Offsets offsets_deusexmover;

    namespace deusexmover {
        inline uint32_t& bOneWay(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexmover.bOneWay);
        }

        inline uint32_t& bLocked(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexmover.bLocked);
        }

        inline uint32_t& bPickable(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexmover.bPickable);
        }

        inline float& lockStrength(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexmover.lockStrength);
        }

        inline float& initiallockStrength(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexmover.initiallockStrength);
        }

        inline uint32_t& bInitialLocked(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexmover.bInitialLocked);
        }

        inline uint32_t& bBreakable(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexmover.bBreakable);
        }

        inline float& doorStrength(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexmover.doorStrength);
        }

        inline Name& KeyIDNeeded(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_deusexmover.KeyIDNeeded);
        }

        inline uint32_t& bHighlight(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexmover.bHighlight);
        }

        inline uint32_t& bFrobbable(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexmover.bFrobbable);
        }

        inline uint32_t& bPicking(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexmover.bPicking);
        }

        inline float& pickValue(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexmover.pickValue);
        }

        inline float& pickTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexmover.pickTime);
        }

        inline int32_t& numPicks(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexmover.numPicks);
        }

        inline float& TicksSinceLastPick(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexmover.TicksSinceLastPick);
        }

        inline float& TicksPerPick(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexmover.TicksPerPick);
        }

        inline float& LastTickTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexmover.LastTickTime);
        }

        inline UObject*& pickPlayer(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexmover.pickPlayer);
        }

        inline UObject*& curPick(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexmover.curPick);
        }

        inline int32_t& minDamageThreshold(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexmover.minDamageThreshold);
        }

        inline uint32_t& bDestroyed(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexmover.bDestroyed);
        }

        inline int32_t& NumFragments(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexmover.NumFragments);
        }

        inline float& FragmentScale(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexmover.FragmentScale);
        }

        inline int32_t& FragmentSpread(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusexmover.FragmentSpread);
        }

        inline UObject*& FragmentClass(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexmover.FragmentClass);
        }

        inline UObject*& FragmentTexture(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexmover.FragmentTexture);
        }

        inline uint32_t& bFragmentTranslucent(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexmover.bFragmentTranslucent);
        }

        inline uint32_t& bFragmentUnlit(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexmover.bFragmentUnlit);
        }

        inline UObject*& ExplodeSound1(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexmover.ExplodeSound1);
        }

        inline UObject*& ExplodeSound2(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_deusexmover.ExplodeSound2);
        }

        inline uint32_t& bDrawExplosion(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexmover.bDrawExplosion);
        }

        inline uint32_t& bIsDoor(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusexmover.bIsDoor);
        }

        inline float& TimeSinceReset(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexmover.TimeSinceReset);
        }

        inline float& TimeToReset(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_deusexmover.TimeToReset);
        }

        inline std::string& msgKeyLocked(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexmover.msgKeyLocked);
        }

        inline std::string& msgKeyUnlocked(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexmover.msgKeyUnlocked);
        }

        inline std::string& msgLockpickSuccess(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexmover.msgLockpickSuccess);
        }

        inline std::string& msgOneWayFail(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexmover.msgOneWayFail);
        }

        inline std::string& msgLocked(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexmover.msgLocked);
        }

        inline std::string& msgPicking(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexmover.msgPicking);
        }

        inline std::string& msgAlreadyUnlocked(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexmover.msgAlreadyUnlocked);
        }

        inline std::string& msgNoNanoKey(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusexmover.msgNoNanoKey);
        }

    }

    namespace deusextextparser {
        struct Offsets {
            size_t Text = 0;
            size_t textPos = 0;
            size_t tagEndPos = 0;
            size_t lastText = 0;
            size_t lastTag = 0;
            size_t lastName = 0;
            size_t lastColor = 0;
            size_t DefaultColor = 0;
            size_t bParagraphStarted = 0;
            size_t PlayerName = 0;
            size_t playerFirstName = 0;
            size_t lastEmailName = 0;
            size_t lastEmailSubject = 0;
            size_t lastEmailFrom = 0;
            size_t lastEmailTo = 0;
            size_t lastEmailCC = 0;
            size_t lastFileName = 0;
            size_t lastFileDescription = 0;
        };
    }

    extern deusextextparser::Offsets offsets_deusextextparser;

    namespace deusextextparser {
        inline int32_t& Text(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusextextparser.Text);
        }

        inline int32_t& textPos(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusextextparser.textPos);
        }

        inline int32_t& tagEndPos(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_deusextextparser.tagEndPos);
        }

        inline std::string& lastText(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusextextparser.lastText);
        }

        inline uint8_t& lastTag(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_deusextextparser.lastTag);
        }

        inline Name& lastName(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_deusextextparser.lastName);
        }

        inline Color& lastColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_deusextextparser.lastColor);
        }

        inline Color& DefaultColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_deusextextparser.DefaultColor);
        }

        inline uint32_t& bParagraphStarted(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_deusextextparser.bParagraphStarted);
        }

        inline std::string& PlayerName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusextextparser.PlayerName);
        }

        inline std::string& playerFirstName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusextextparser.playerFirstName);
        }

        inline std::string& lastEmailName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusextextparser.lastEmailName);
        }

        inline std::string& lastEmailSubject(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusextextparser.lastEmailSubject);
        }

        inline std::string& lastEmailFrom(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusextextparser.lastEmailFrom);
        }

        inline std::string& lastEmailTo(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusextextparser.lastEmailTo);
        }

        inline std::string& lastEmailCC(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusextextparser.lastEmailCC);
        }

        inline std::string& lastFileName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusextextparser.lastFileName);
        }

        inline std::string& lastFileDescription(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_deusextextparser.lastFileDescription);
        }

    }

    void bindProps_deusex();
}
