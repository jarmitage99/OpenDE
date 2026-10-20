#include "engine/props_deusex.h"

#include "core/objects.h"

#include <cstdio>

namespace dx {
    deusexplayer::Offsets offsets_deusexplayer;
    scriptedpawn::Offsets offsets_scriptedpawn;
    deusexdecoration::Offsets offsets_deusexdecoration;
    deusexsaveinfo::Offsets offsets_deusexsaveinfo;
    gamedirectory::Offsets offsets_gamedirectory;
    dumplocation::Offsets offsets_dumplocation;
    particleiterator::Offsets offsets_particleiterator;
    deusexlevelinfo::Offsets offsets_deusexlevelinfo;
    deusexrootwindow::Offsets offsets_deusexrootwindow;
    deusexgameinfo::Offsets offsets_deusexgameinfo;
    deusexweapon::Offsets offsets_deusexweapon;
    deusexcarcass::Offsets offsets_deusexcarcass;
    deusexpickup::Offsets offsets_deusexpickup;
    deusexprojectile::Offsets offsets_deusexprojectile;
    conplay::Offsets offsets_conplay;
    deusexmover::Offsets offsets_deusexmover;
    deusextextparser::Offsets offsets_deusextextparser;

    void bindProps_deusex()
    {
        {
            UClass* cls = objects::findClass("DeusEx.DeusExPlayer");
            if (!cls) {
                std::fprintf(stderr, "missing class DeusEx.DeusExPlayer\n");
            } else {
                offsets_deusexplayer.TruePlayerName = cls->findProperty(Name("TruePlayerName"))->offset;
                offsets_deusexplayer.PlayerSkin = cls->findProperty(Name("PlayerSkin"))->offset;
                offsets_deusexplayer.CombatDifficulty = cls->findProperty(Name("CombatDifficulty"))->offset;
                offsets_deusexplayer.AugmentationSystem = cls->findProperty(Name("AugmentationSystem"))->offset;
                offsets_deusexplayer.SkillSystem = cls->findProperty(Name("SkillSystem"))->offset;
                offsets_deusexplayer.SkillPointsTotal = cls->findProperty(Name("SkillPointsTotal"))->offset;
                offsets_deusexplayer.SkillPointsAvail = cls->findProperty(Name("SkillPointsAvail"))->offset;
                offsets_deusexplayer.Credits = cls->findProperty(Name("Credits"))->offset;
                offsets_deusexplayer.Energy = cls->findProperty(Name("Energy"))->offset;
                offsets_deusexplayer.EnergyMax = cls->findProperty(Name("EnergyMax"))->offset;
                offsets_deusexplayer.EnergyDrain = cls->findProperty(Name("EnergyDrain"))->offset;
                offsets_deusexplayer.EnergyDrainTotal = cls->findProperty(Name("EnergyDrainTotal"))->offset;
                if (UProperty* property = cls->findProperty(Name("MaxRegenPoint"))) {
                    offsets_deusexplayer.MaxRegenPoint = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("RegenRate"))) {
                    offsets_deusexplayer.RegenRate = property->offset;
                }
                offsets_deusexplayer.KeyRing = cls->findProperty(Name("KeyRing"))->offset;
                offsets_deusexplayer.KeyList = cls->findProperty(Name("KeyList"))->offset;
                offsets_deusexplayer.MaxFrobDistance = cls->findProperty(Name("MaxFrobDistance"))->offset;
                offsets_deusexplayer.FrobTarget = cls->findProperty(Name("FrobTarget"))->offset;
                offsets_deusexplayer.FrobTime = cls->findProperty(Name("FrobTime"))->offset;
                if (UProperty* property = cls->findProperty(Name("LastRefreshTime"))) {
                    offsets_deusexplayer.LastRefreshTime = property->offset;
                }
                offsets_deusexplayer.ConPlay = cls->findProperty(Name("ConPlay"))->offset;
                offsets_deusexplayer.DataLinkPlay = cls->findProperty(Name("DataLinkPlay"))->offset;
                offsets_deusexplayer.ConHistory = cls->findProperty(Name("ConHistory"))->offset;
                offsets_deusexplayer.invSlots = cls->findProperty(Name("invSlots"))->offset;
                offsets_deusexplayer.maxInvRows = cls->findProperty(Name("maxInvRows"))->offset;
                offsets_deusexplayer.maxInvCols = cls->findProperty(Name("maxInvCols"))->offset;
                offsets_deusexplayer.inHand = cls->findProperty(Name("inHand"))->offset;
                offsets_deusexplayer.inHandPending = cls->findProperty(Name("inHandPending"))->offset;
                if (UProperty* property = cls->findProperty(Name("ClientinHandPending"))) {
                    offsets_deusexplayer.ClientinHandPending = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("LastinHand"))) {
                    offsets_deusexplayer.LastinHand = property->offset;
                }
                offsets_deusexplayer.bInHandTransition = cls->findProperty(Name("bInHandTransition"))->offset;
                if (UProperty* property = cls->findProperty(Name("bBeltIsMPInventory"))) {
                    offsets_deusexplayer.bBeltIsMPInventory = property->offset;
                }
                offsets_deusexplayer.FirstGoal = cls->findProperty(Name("FirstGoal"))->offset;
                offsets_deusexplayer.LastGoal = cls->findProperty(Name("LastGoal"))->offset;
                offsets_deusexplayer.FirstNote = cls->findProperty(Name("FirstNote"))->offset;
                offsets_deusexplayer.LastNote = cls->findProperty(Name("LastNote"))->offset;
                offsets_deusexplayer.FirstImage = cls->findProperty(Name("FirstImage"))->offset;
                offsets_deusexplayer.FirstLog = cls->findProperty(Name("FirstLog"))->offset;
                offsets_deusexplayer.LastLog = cls->findProperty(Name("LastLog"))->offset;
                offsets_deusexplayer.ViewModelActor = cls->findProperty(Name("ViewModelActor"))->offset;
                if (UProperty* property = cls->findProperty(Name("bFirstOptionsSynced"))) {
                    offsets_deusexplayer.bFirstOptionsSynced = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("bSecondOptionsSynced"))) {
                    offsets_deusexplayer.bSecondOptionsSynced = property->offset;
                }
                offsets_deusexplayer.bForceDuck = cls->findProperty(Name("bForceDuck"))->offset;
                offsets_deusexplayer.bCrouchOn = cls->findProperty(Name("bCrouchOn"))->offset;
                offsets_deusexplayer.bWasCrouchOn = cls->findProperty(Name("bWasCrouchOn"))->offset;
                offsets_deusexplayer.lastbDuck = cls->findProperty(Name("lastbDuck"))->offset;
                offsets_deusexplayer.bCanLean = cls->findProperty(Name("bCanLean"))->offset;
                offsets_deusexplayer.curLeanDist = cls->findProperty(Name("curLeanDist"))->offset;
                if (UProperty* property = cls->findProperty(Name("prevLeanDist"))) {
                    offsets_deusexplayer.prevLeanDist = property->offset;
                }
                offsets_deusexplayer.bToggleWalk = cls->findProperty(Name("bToggleWalk"))->offset;
                if (UProperty* property = cls->findProperty(Name("RunSilentValue"))) {
                    offsets_deusexplayer.RunSilentValue = property->offset;
                }
                offsets_deusexplayer.bWarrenEMPField = cls->findProperty(Name("bWarrenEMPField"))->offset;
                offsets_deusexplayer.WarrenTimer = cls->findProperty(Name("WarrenTimer"))->offset;
                offsets_deusexplayer.WarrenSlot = cls->findProperty(Name("WarrenSlot"))->offset;
                offsets_deusexplayer.FloorMaterial = cls->findProperty(Name("FloorMaterial"))->offset;
                offsets_deusexplayer.WallMaterial = cls->findProperty(Name("WallMaterial"))->offset;
                offsets_deusexplayer.WallNormal = cls->findProperty(Name("WallNormal"))->offset;
                offsets_deusexplayer.drugEffectTimer = cls->findProperty(Name("drugEffectTimer"))->offset;
                offsets_deusexplayer.JoltMagnitude = cls->findProperty(Name("JoltMagnitude"))->offset;
                offsets_deusexplayer.poisonTimer = cls->findProperty(Name("poisonTimer"))->offset;
                offsets_deusexplayer.poisonCounter = cls->findProperty(Name("poisonCounter"))->offset;
                offsets_deusexplayer.poisonDamage = cls->findProperty(Name("poisonDamage"))->offset;
                offsets_deusexplayer.BleedRate = cls->findProperty(Name("BleedRate"))->offset;
                offsets_deusexplayer.DropCounter = cls->findProperty(Name("DropCounter"))->offset;
                offsets_deusexplayer.ClotPeriod = cls->findProperty(Name("ClotPeriod"))->offset;
                if (UProperty* property = cls->findProperty(Name("FlashTimer"))) {
                    offsets_deusexplayer.FlashTimer = property->offset;
                }
                offsets_deusexplayer.swimDuration = cls->findProperty(Name("swimDuration"))->offset;
                offsets_deusexplayer.swimTimer = cls->findProperty(Name("swimTimer"))->offset;
                offsets_deusexplayer.swimBubbleTimer = cls->findProperty(Name("swimBubbleTimer"))->offset;
                offsets_deusexplayer.ConversationActor = cls->findProperty(Name("ConversationActor"))->offset;
                offsets_deusexplayer.lastThirdPersonConvoActor = cls->findProperty(Name("lastThirdPersonConvoActor"))->offset;
                offsets_deusexplayer.lastThirdPersonConvoTime = cls->findProperty(Name("lastThirdPersonConvoTime"))->offset;
                offsets_deusexplayer.lastFirstPersonConvoActor = cls->findProperty(Name("lastFirstPersonConvoActor"))->offset;
                offsets_deusexplayer.lastFirstPersonConvoTime = cls->findProperty(Name("lastFirstPersonConvoTime"))->offset;
                offsets_deusexplayer.bStartingNewGame = cls->findProperty(Name("bStartingNewGame"))->offset;
                offsets_deusexplayer.bSavingSkillsAugs = cls->findProperty(Name("bSavingSkillsAugs"))->offset;
                if (UProperty* property = cls->findProperty(Name("bSpyDroneActive"))) {
                    offsets_deusexplayer.bSpyDroneActive = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("spyDroneLevel"))) {
                    offsets_deusexplayer.spyDroneLevel = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("spyDroneLevelValue"))) {
                    offsets_deusexplayer.spyDroneLevelValue = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("aDrone"))) {
                    offsets_deusexplayer.aDrone = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("bBuySkills"))) {
                    offsets_deusexplayer.bBuySkills = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("bKillerProfile"))) {
                    offsets_deusexplayer.bKillerProfile = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("mpMsgFlags"))) {
                    offsets_deusexplayer.mpMsgFlags = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("mpMsgServerFlags"))) {
                    offsets_deusexplayer.mpMsgServerFlags = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("mpMsgCode"))) {
                    offsets_deusexplayer.mpMsgCode = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("mpMsgTime"))) {
                    offsets_deusexplayer.mpMsgTime = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("mpMsgOptionalParam"))) {
                    offsets_deusexplayer.mpMsgOptionalParam = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("mpMsgOptionalString"))) {
                    offsets_deusexplayer.mpMsgOptionalString = property->offset;
                }
                offsets_deusexplayer.strStartMap = cls->findProperty(Name("strStartMap"))->offset;
                offsets_deusexplayer.bStartNewGameAfterIntro = cls->findProperty(Name("bStartNewGameAfterIntro"))->offset;
                offsets_deusexplayer.bIgnoreNextShowMenu = cls->findProperty(Name("bIgnoreNextShowMenu"))->offset;
                offsets_deusexplayer.NextMap = cls->findProperty(Name("NextMap"))->offset;
                offsets_deusexplayer.bObjectNames = cls->findProperty(Name("bObjectNames"))->offset;
                offsets_deusexplayer.bNPCHighlighting = cls->findProperty(Name("bNPCHighlighting"))->offset;
                offsets_deusexplayer.bSubtitles = cls->findProperty(Name("bSubtitles"))->offset;
                offsets_deusexplayer.bAlwaysRun = cls->findProperty(Name("bAlwaysRun"))->offset;
                offsets_deusexplayer.bToggleCrouch = cls->findProperty(Name("bToggleCrouch"))->offset;
                offsets_deusexplayer.logTimeout = cls->findProperty(Name("logTimeout"))->offset;
                offsets_deusexplayer.maxLogLines = cls->findProperty(Name("maxLogLines"))->offset;
                if (UProperty* property = cls->findProperty(Name("bHelpMessages"))) {
                    offsets_deusexplayer.bHelpMessages = property->offset;
                }
                offsets_deusexplayer.translucencyLevel = cls->findProperty(Name("translucencyLevel"))->offset;
                offsets_deusexplayer.bObjectBeltVisible = cls->findProperty(Name("bObjectBeltVisible"))->offset;
                offsets_deusexplayer.bHitDisplayVisible = cls->findProperty(Name("bHitDisplayVisible"))->offset;
                offsets_deusexplayer.bAmmoDisplayVisible = cls->findProperty(Name("bAmmoDisplayVisible"))->offset;
                offsets_deusexplayer.bAugDisplayVisible = cls->findProperty(Name("bAugDisplayVisible"))->offset;
                offsets_deusexplayer.bDisplayAmmoByClip = cls->findProperty(Name("bDisplayAmmoByClip"))->offset;
                offsets_deusexplayer.bCompassVisible = cls->findProperty(Name("bCompassVisible"))->offset;
                offsets_deusexplayer.bCrosshairVisible = cls->findProperty(Name("bCrosshairVisible"))->offset;
                offsets_deusexplayer.bAutoReload = cls->findProperty(Name("bAutoReload"))->offset;
                offsets_deusexplayer.bDisplayAllGoals = cls->findProperty(Name("bDisplayAllGoals"))->offset;
                offsets_deusexplayer.bHUDShowAllAugs = cls->findProperty(Name("bHUDShowAllAugs"))->offset;
                offsets_deusexplayer.UIBackground = cls->findProperty(Name("UIBackground"))->offset;
                offsets_deusexplayer.bDisplayCompletedGoals = cls->findProperty(Name("bDisplayCompletedGoals"))->offset;
                offsets_deusexplayer.bShowAmmoDescriptions = cls->findProperty(Name("bShowAmmoDescriptions"))->offset;
                offsets_deusexplayer.bConfirmSaveDeletes = cls->findProperty(Name("bConfirmSaveDeletes"))->offset;
                offsets_deusexplayer.bConfirmNoteDeletes = cls->findProperty(Name("bConfirmNoteDeletes"))->offset;
                offsets_deusexplayer.bAskedToTrain = cls->findProperty(Name("bAskedToTrain"))->offset;
                if (UProperty* property = cls->findProperty(Name("AugPrefs"))) {
                    offsets_deusexplayer.AugPrefs = property->offset;
                }
                offsets_deusexplayer.BarkManager = cls->findProperty(Name("BarkManager"))->offset;
                offsets_deusexplayer.ThemeManager = cls->findProperty(Name("ThemeManager"))->offset;
                offsets_deusexplayer.MenuThemeName = cls->findProperty(Name("MenuThemeName"))->offset;
                offsets_deusexplayer.HUDThemeName = cls->findProperty(Name("HUDThemeName"))->offset;
                offsets_deusexplayer.bHUDBordersVisible = cls->findProperty(Name("bHUDBordersVisible"))->offset;
                offsets_deusexplayer.bHUDBordersTranslucent = cls->findProperty(Name("bHUDBordersTranslucent"))->offset;
                offsets_deusexplayer.bHUDBackgroundTranslucent = cls->findProperty(Name("bHUDBackgroundTranslucent"))->offset;
                offsets_deusexplayer.bMenusTranslucent = cls->findProperty(Name("bMenusTranslucent"))->offset;
                offsets_deusexplayer.InventoryFull = cls->findProperty(Name("InventoryFull"))->offset;
                offsets_deusexplayer.TooMuchAmmo = cls->findProperty(Name("TooMuchAmmo"))->offset;
                offsets_deusexplayer.TooHeavyToLift = cls->findProperty(Name("TooHeavyToLift"))->offset;
                offsets_deusexplayer.CannotLift = cls->findProperty(Name("CannotLift"))->offset;
                offsets_deusexplayer.NoRoomToLift = cls->findProperty(Name("NoRoomToLift"))->offset;
                offsets_deusexplayer.CanCarryOnlyOne = cls->findProperty(Name("CanCarryOnlyOne"))->offset;
                offsets_deusexplayer.CannotDropHere = cls->findProperty(Name("CannotDropHere"))->offset;
                offsets_deusexplayer.HandsFull = cls->findProperty(Name("HandsFull"))->offset;
                offsets_deusexplayer.NoteAdded = cls->findProperty(Name("NoteAdded"))->offset;
                offsets_deusexplayer.GoalAdded = cls->findProperty(Name("GoalAdded"))->offset;
                offsets_deusexplayer.PrimaryGoalCompleted = cls->findProperty(Name("PrimaryGoalCompleted"))->offset;
                offsets_deusexplayer.SecondaryGoalCompleted = cls->findProperty(Name("SecondaryGoalCompleted"))->offset;
                offsets_deusexplayer.EnergyDepleted = cls->findProperty(Name("EnergyDepleted"))->offset;
                offsets_deusexplayer.AddedNanoKey = cls->findProperty(Name("AddedNanoKey"))->offset;
                offsets_deusexplayer.HealedPointsLabel = cls->findProperty(Name("HealedPointsLabel"))->offset;
                offsets_deusexplayer.HealedPointLabel = cls->findProperty(Name("HealedPointLabel"))->offset;
                offsets_deusexplayer.SkillPointsAward = cls->findProperty(Name("SkillPointsAward"))->offset;
                offsets_deusexplayer.QuickSaveGameTitle = cls->findProperty(Name("QuickSaveGameTitle"))->offset;
                if (UProperty* property = cls->findProperty(Name("WeaponUnCloak"))) {
                    offsets_deusexplayer.WeaponUnCloak = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("TakenOverString"))) {
                    offsets_deusexplayer.TakenOverString = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("HeadString"))) {
                    offsets_deusexplayer.HeadString = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("TorsoString"))) {
                    offsets_deusexplayer.TorsoString = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("LegsString"))) {
                    offsets_deusexplayer.LegsString = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("WithTheString"))) {
                    offsets_deusexplayer.WithTheString = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("WithString"))) {
                    offsets_deusexplayer.WithString = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("PoisonString"))) {
                    offsets_deusexplayer.PoisonString = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("BurnString"))) {
                    offsets_deusexplayer.BurnString = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("NoneString"))) {
                    offsets_deusexplayer.NoneString = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("DamageShield"))) {
                    offsets_deusexplayer.DamageShield = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("ShieldTimer"))) {
                    offsets_deusexplayer.ShieldTimer = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("ShieldStatus"))) {
                    offsets_deusexplayer.ShieldStatus = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("myBurner"))) {
                    offsets_deusexplayer.myBurner = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("myPoisoner"))) {
                    offsets_deusexplayer.myPoisoner = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("myProjKiller"))) {
                    offsets_deusexplayer.myProjKiller = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("myTurretKiller"))) {
                    offsets_deusexplayer.myTurretKiller = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("myKiller"))) {
                    offsets_deusexplayer.myKiller = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("killProfile"))) {
                    offsets_deusexplayer.killProfile = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("invulnSph"))) {
                    offsets_deusexplayer.invulnSph = property->offset;
                }
                offsets_deusexplayer.musicMode = cls->findProperty(Name("musicMode"))->offset;
                offsets_deusexplayer.savedSection = cls->findProperty(Name("savedSection"))->offset;
                offsets_deusexplayer.musicCheckTimer = cls->findProperty(Name("musicCheckTimer"))->offset;
                offsets_deusexplayer.musicChangeTimer = cls->findProperty(Name("musicChangeTimer"))->offset;
                offsets_deusexplayer.saveCount = cls->findProperty(Name("saveCount"))->offset;
                offsets_deusexplayer.saveTime = cls->findProperty(Name("saveTime"))->offset;
                offsets_deusexplayer.GlobalDebugObj = cls->findProperty(Name("GlobalDebugObj"))->offset;
                offsets_deusexplayer.bQuotesEnabled = cls->findProperty(Name("bQuotesEnabled"))->offset;
                if (UProperty* property = cls->findProperty(Name("DXGame"))) {
                    offsets_deusexplayer.DXGame = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("ServerTimeDiff"))) {
                    offsets_deusexplayer.ServerTimeDiff = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("ServerTimeLastRefresh"))) {
                    offsets_deusexplayer.ServerTimeLastRefresh = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("MPDamageMult"))) {
                    offsets_deusexplayer.MPDamageMult = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("NintendoImmunityTime"))) {
                    offsets_deusexplayer.NintendoImmunityTime = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("NintendoImmunityTimeLeft"))) {
                    offsets_deusexplayer.NintendoImmunityTimeLeft = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("bNintendoImmunity"))) {
                    offsets_deusexplayer.bNintendoImmunity = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("ActiveComputer"))) {
                    offsets_deusexplayer.ActiveComputer = property->offset;
                }
            }
        }
        {
            UClass* cls = objects::findClass("DeusEx.ScriptedPawn");
            if (!cls) {
                std::fprintf(stderr, "missing class DeusEx.ScriptedPawn\n");
            } else {
                offsets_scriptedpawn.lastPoints = cls->findProperty(Name("lastPoints"))->offset;
                offsets_scriptedpawn.Restlessness = cls->findProperty(Name("Restlessness"))->offset;
                offsets_scriptedpawn.Wanderlust = cls->findProperty(Name("Wanderlust"))->offset;
                offsets_scriptedpawn.Cowardice = cls->findProperty(Name("Cowardice"))->offset;
                offsets_scriptedpawn.BaseAccuracy = cls->findProperty(Name("BaseAccuracy"))->offset;
                offsets_scriptedpawn.maxRange = cls->findProperty(Name("maxRange"))->offset;
                offsets_scriptedpawn.MinRange = cls->findProperty(Name("MinRange"))->offset;
                offsets_scriptedpawn.MinHealth = cls->findProperty(Name("MinHealth"))->offset;
                offsets_scriptedpawn.RandomWandering = cls->findProperty(Name("RandomWandering"))->offset;
                offsets_scriptedpawn.sleepTime = cls->findProperty(Name("sleepTime"))->offset;
                offsets_scriptedpawn.destPoint = cls->findProperty(Name("destPoint"))->offset;
                offsets_scriptedpawn.destLoc = cls->findProperty(Name("destLoc"))->offset;
                offsets_scriptedpawn.useLoc = cls->findProperty(Name("useLoc"))->offset;
                offsets_scriptedpawn.useRot = cls->findProperty(Name("useRot"))->offset;
                offsets_scriptedpawn.seekDistance = cls->findProperty(Name("seekDistance"))->offset;
                offsets_scriptedpawn.SeekLevel = cls->findProperty(Name("SeekLevel"))->offset;
                offsets_scriptedpawn.SeekType = cls->findProperty(Name("SeekType"))->offset;
                offsets_scriptedpawn.SeekPawn = cls->findProperty(Name("SeekPawn"))->offset;
                offsets_scriptedpawn.CarcassTimer = cls->findProperty(Name("CarcassTimer"))->offset;
                offsets_scriptedpawn.CarcassHateTimer = cls->findProperty(Name("CarcassHateTimer"))->offset;
                offsets_scriptedpawn.CarcassCheckTimer = cls->findProperty(Name("CarcassCheckTimer"))->offset;
                offsets_scriptedpawn.PotentialEnemyAlliance = cls->findProperty(Name("PotentialEnemyAlliance"))->offset;
                offsets_scriptedpawn.PotentialEnemyTimer = cls->findProperty(Name("PotentialEnemyTimer"))->offset;
                offsets_scriptedpawn.BeamCheckTimer = cls->findProperty(Name("BeamCheckTimer"))->offset;
                offsets_scriptedpawn.bSeekPostCombat = cls->findProperty(Name("bSeekPostCombat"))->offset;
                offsets_scriptedpawn.bSeekLocation = cls->findProperty(Name("bSeekLocation"))->offset;
                offsets_scriptedpawn.bInterruptSeek = cls->findProperty(Name("bInterruptSeek"))->offset;
                offsets_scriptedpawn.bAlliancesChanged = cls->findProperty(Name("bAlliancesChanged"))->offset;
                offsets_scriptedpawn.bNoNegativeAlliances = cls->findProperty(Name("bNoNegativeAlliances"))->offset;
                offsets_scriptedpawn.bSitAnywhere = cls->findProperty(Name("bSitAnywhere"))->offset;
                offsets_scriptedpawn.bSitInterpolation = cls->findProperty(Name("bSitInterpolation"))->offset;
                offsets_scriptedpawn.bStandInterpolation = cls->findProperty(Name("bStandInterpolation"))->offset;
                offsets_scriptedpawn.remainingSitTime = cls->findProperty(Name("remainingSitTime"))->offset;
                offsets_scriptedpawn.remainingStandTime = cls->findProperty(Name("remainingStandTime"))->offset;
                offsets_scriptedpawn.StandRate = cls->findProperty(Name("StandRate"))->offset;
                offsets_scriptedpawn.ReloadTimer = cls->findProperty(Name("ReloadTimer"))->offset;
                offsets_scriptedpawn.bReadyToReload = cls->findProperty(Name("bReadyToReload"))->offset;
                offsets_scriptedpawn.CarcassType = cls->findProperty(Name("CarcassType"))->offset;
                offsets_scriptedpawn.Orders = cls->findProperty(Name("Orders"))->offset;
                offsets_scriptedpawn.OrderTag = cls->findProperty(Name("OrderTag"))->offset;
                offsets_scriptedpawn.HomeTag = cls->findProperty(Name("HomeTag"))->offset;
                offsets_scriptedpawn.HomeExtent = cls->findProperty(Name("HomeExtent"))->offset;
                offsets_scriptedpawn.OrderActor = cls->findProperty(Name("OrderActor"))->offset;
                offsets_scriptedpawn.NextAnim = cls->findProperty(Name("NextAnim"))->offset;
                offsets_scriptedpawn.WalkingSpeed = cls->findProperty(Name("WalkingSpeed"))->offset;
                offsets_scriptedpawn.ProjectileSpeed = cls->findProperty(Name("ProjectileSpeed"))->offset;
                offsets_scriptedpawn.LastPainAnim = cls->findProperty(Name("LastPainAnim"))->offset;
                offsets_scriptedpawn.LastPainTime = cls->findProperty(Name("LastPainTime"))->offset;
                offsets_scriptedpawn.DesiredPrePivot = cls->findProperty(Name("DesiredPrePivot"))->offset;
                offsets_scriptedpawn.PrePivotTime = cls->findProperty(Name("PrePivotTime"))->offset;
                offsets_scriptedpawn.PrePivotOffset = cls->findProperty(Name("PrePivotOffset"))->offset;
                offsets_scriptedpawn.bCanBleed = cls->findProperty(Name("bCanBleed"))->offset;
                offsets_scriptedpawn.BleedRate = cls->findProperty(Name("BleedRate"))->offset;
                offsets_scriptedpawn.DropCounter = cls->findProperty(Name("DropCounter"))->offset;
                offsets_scriptedpawn.ClotPeriod = cls->findProperty(Name("ClotPeriod"))->offset;
                offsets_scriptedpawn.bAcceptBump = cls->findProperty(Name("bAcceptBump"))->offset;
                offsets_scriptedpawn.bCanFire = cls->findProperty(Name("bCanFire"))->offset;
                offsets_scriptedpawn.bKeepWeaponDrawn = cls->findProperty(Name("bKeepWeaponDrawn"))->offset;
                offsets_scriptedpawn.bShowPain = cls->findProperty(Name("bShowPain"))->offset;
                offsets_scriptedpawn.bCanSit = cls->findProperty(Name("bCanSit"))->offset;
                offsets_scriptedpawn.bAlwaysPatrol = cls->findProperty(Name("bAlwaysPatrol"))->offset;
                offsets_scriptedpawn.bPlayIdle = cls->findProperty(Name("bPlayIdle"))->offset;
                offsets_scriptedpawn.bLeaveAfterFleeing = cls->findProperty(Name("bLeaveAfterFleeing"))->offset;
                offsets_scriptedpawn.bLikesNeutral = cls->findProperty(Name("bLikesNeutral"))->offset;
                offsets_scriptedpawn.bUseFirstSeatOnly = cls->findProperty(Name("bUseFirstSeatOnly"))->offset;
                offsets_scriptedpawn.bCower = cls->findProperty(Name("bCower"))->offset;
                offsets_scriptedpawn.HomeActor = cls->findProperty(Name("HomeActor"))->offset;
                offsets_scriptedpawn.HomeLoc = cls->findProperty(Name("HomeLoc"))->offset;
                offsets_scriptedpawn.HomeRot = cls->findProperty(Name("HomeRot"))->offset;
                offsets_scriptedpawn.bUseHome = cls->findProperty(Name("bUseHome"))->offset;
                offsets_scriptedpawn.bInterruptState = cls->findProperty(Name("bInterruptState"))->offset;
                offsets_scriptedpawn.bCanConverse = cls->findProperty(Name("bCanConverse"))->offset;
                offsets_scriptedpawn.bImportant = cls->findProperty(Name("bImportant"))->offset;
                offsets_scriptedpawn.bInvincible = cls->findProperty(Name("bInvincible"))->offset;
                offsets_scriptedpawn.bInitialized = cls->findProperty(Name("bInitialized"))->offset;
                offsets_scriptedpawn.bAvoidAim = cls->findProperty(Name("bAvoidAim"))->offset;
                offsets_scriptedpawn.bAimForHead = cls->findProperty(Name("bAimForHead"))->offset;
                offsets_scriptedpawn.bDefendHome = cls->findProperty(Name("bDefendHome"))->offset;
                offsets_scriptedpawn.bCanCrouch = cls->findProperty(Name("bCanCrouch"))->offset;
                offsets_scriptedpawn.bSeekCover = cls->findProperty(Name("bSeekCover"))->offset;
                offsets_scriptedpawn.bSprint = cls->findProperty(Name("bSprint"))->offset;
                offsets_scriptedpawn.bUseFallbackWeapons = cls->findProperty(Name("bUseFallbackWeapons"))->offset;
                offsets_scriptedpawn.AvoidAccuracy = cls->findProperty(Name("AvoidAccuracy"))->offset;
                offsets_scriptedpawn.bAvoidHarm = cls->findProperty(Name("bAvoidHarm"))->offset;
                offsets_scriptedpawn.HarmAccuracy = cls->findProperty(Name("HarmAccuracy"))->offset;
                offsets_scriptedpawn.CrouchRate = cls->findProperty(Name("CrouchRate"))->offset;
                offsets_scriptedpawn.SprintRate = cls->findProperty(Name("SprintRate"))->offset;
                offsets_scriptedpawn.CloseCombatMult = cls->findProperty(Name("CloseCombatMult"))->offset;
                offsets_scriptedpawn.bHateHacking = cls->findProperty(Name("bHateHacking"))->offset;
                offsets_scriptedpawn.bHateWeapon = cls->findProperty(Name("bHateWeapon"))->offset;
                offsets_scriptedpawn.bHateShot = cls->findProperty(Name("bHateShot"))->offset;
                offsets_scriptedpawn.bHateInjury = cls->findProperty(Name("bHateInjury"))->offset;
                offsets_scriptedpawn.bHateIndirectInjury = cls->findProperty(Name("bHateIndirectInjury"))->offset;
                offsets_scriptedpawn.bHateCarcass = cls->findProperty(Name("bHateCarcass"))->offset;
                offsets_scriptedpawn.bHateDistress = cls->findProperty(Name("bHateDistress"))->offset;
                offsets_scriptedpawn.bReactFutz = cls->findProperty(Name("bReactFutz"))->offset;
                offsets_scriptedpawn.bReactPresence = cls->findProperty(Name("bReactPresence"))->offset;
                offsets_scriptedpawn.bReactLoudNoise = cls->findProperty(Name("bReactLoudNoise"))->offset;
                offsets_scriptedpawn.bReactAlarm = cls->findProperty(Name("bReactAlarm"))->offset;
                offsets_scriptedpawn.bReactShot = cls->findProperty(Name("bReactShot"))->offset;
                offsets_scriptedpawn.bReactCarcass = cls->findProperty(Name("bReactCarcass"))->offset;
                offsets_scriptedpawn.bReactDistress = cls->findProperty(Name("bReactDistress"))->offset;
                offsets_scriptedpawn.bReactProjectiles = cls->findProperty(Name("bReactProjectiles"))->offset;
                offsets_scriptedpawn.bFearHacking = cls->findProperty(Name("bFearHacking"))->offset;
                offsets_scriptedpawn.bFearWeapon = cls->findProperty(Name("bFearWeapon"))->offset;
                offsets_scriptedpawn.bFearShot = cls->findProperty(Name("bFearShot"))->offset;
                offsets_scriptedpawn.bFearInjury = cls->findProperty(Name("bFearInjury"))->offset;
                offsets_scriptedpawn.bFearIndirectInjury = cls->findProperty(Name("bFearIndirectInjury"))->offset;
                offsets_scriptedpawn.bFearCarcass = cls->findProperty(Name("bFearCarcass"))->offset;
                offsets_scriptedpawn.bFearDistress = cls->findProperty(Name("bFearDistress"))->offset;
                offsets_scriptedpawn.bFearAlarm = cls->findProperty(Name("bFearAlarm"))->offset;
                offsets_scriptedpawn.bFearProjectiles = cls->findProperty(Name("bFearProjectiles"))->offset;
                offsets_scriptedpawn.bEmitDistress = cls->findProperty(Name("bEmitDistress"))->offset;
                offsets_scriptedpawn.RaiseAlarm = cls->findProperty(Name("RaiseAlarm"))->offset;
                offsets_scriptedpawn.bLookingForEnemy = cls->findProperty(Name("bLookingForEnemy"))->offset;
                offsets_scriptedpawn.bLookingForLoudNoise = cls->findProperty(Name("bLookingForLoudNoise"))->offset;
                offsets_scriptedpawn.bLookingForAlarm = cls->findProperty(Name("bLookingForAlarm"))->offset;
                offsets_scriptedpawn.bLookingForDistress = cls->findProperty(Name("bLookingForDistress"))->offset;
                offsets_scriptedpawn.bLookingForProjectiles = cls->findProperty(Name("bLookingForProjectiles"))->offset;
                offsets_scriptedpawn.bLookingForFutz = cls->findProperty(Name("bLookingForFutz"))->offset;
                offsets_scriptedpawn.bLookingForHacking = cls->findProperty(Name("bLookingForHacking"))->offset;
                offsets_scriptedpawn.bLookingForShot = cls->findProperty(Name("bLookingForShot"))->offset;
                offsets_scriptedpawn.bLookingForWeapon = cls->findProperty(Name("bLookingForWeapon"))->offset;
                offsets_scriptedpawn.bLookingForCarcass = cls->findProperty(Name("bLookingForCarcass"))->offset;
                offsets_scriptedpawn.bLookingForInjury = cls->findProperty(Name("bLookingForInjury"))->offset;
                offsets_scriptedpawn.bLookingForIndirectInjury = cls->findProperty(Name("bLookingForIndirectInjury"))->offset;
                offsets_scriptedpawn.bFacingTarget = cls->findProperty(Name("bFacingTarget"))->offset;
                offsets_scriptedpawn.bMustFaceTarget = cls->findProperty(Name("bMustFaceTarget"))->offset;
                offsets_scriptedpawn.FireAngle = cls->findProperty(Name("FireAngle"))->offset;
                offsets_scriptedpawn.FireElevation = cls->findProperty(Name("FireElevation"))->offset;
                offsets_scriptedpawn.MaxProvocations = cls->findProperty(Name("MaxProvocations"))->offset;
                offsets_scriptedpawn.AgitationSustainTime = cls->findProperty(Name("AgitationSustainTime"))->offset;
                offsets_scriptedpawn.AgitationDecayRate = cls->findProperty(Name("AgitationDecayRate"))->offset;
                offsets_scriptedpawn.AgitationTimer = cls->findProperty(Name("AgitationTimer"))->offset;
                offsets_scriptedpawn.AgitationCheckTimer = cls->findProperty(Name("AgitationCheckTimer"))->offset;
                offsets_scriptedpawn.PlayerAgitationTimer = cls->findProperty(Name("PlayerAgitationTimer"))->offset;
                offsets_scriptedpawn.FearSustainTime = cls->findProperty(Name("FearSustainTime"))->offset;
                offsets_scriptedpawn.FearDecayRate = cls->findProperty(Name("FearDecayRate"))->offset;
                offsets_scriptedpawn.FearTimer = cls->findProperty(Name("FearTimer"))->offset;
                offsets_scriptedpawn.FearLevel = cls->findProperty(Name("FearLevel"))->offset;
                offsets_scriptedpawn.EnemyReadiness = cls->findProperty(Name("EnemyReadiness"))->offset;
                offsets_scriptedpawn.ReactionLevel = cls->findProperty(Name("ReactionLevel"))->offset;
                offsets_scriptedpawn.SurprisePeriod = cls->findProperty(Name("SurprisePeriod"))->offset;
                offsets_scriptedpawn.SightPercentage = cls->findProperty(Name("SightPercentage"))->offset;
                offsets_scriptedpawn.CycleTimer = cls->findProperty(Name("CycleTimer"))->offset;
                offsets_scriptedpawn.CyclePeriod = cls->findProperty(Name("CyclePeriod"))->offset;
                offsets_scriptedpawn.CycleCumulative = cls->findProperty(Name("CycleCumulative"))->offset;
                offsets_scriptedpawn.CycleCandidate = cls->findProperty(Name("CycleCandidate"))->offset;
                offsets_scriptedpawn.CycleDistance = cls->findProperty(Name("CycleDistance"))->offset;
                offsets_scriptedpawn.AlarmActor = cls->findProperty(Name("AlarmActor"))->offset;
                offsets_scriptedpawn.AlarmTimer = cls->findProperty(Name("AlarmTimer"))->offset;
                offsets_scriptedpawn.WeaponTimer = cls->findProperty(Name("WeaponTimer"))->offset;
                offsets_scriptedpawn.FireTimer = cls->findProperty(Name("FireTimer"))->offset;
                offsets_scriptedpawn.SpecialTimer = cls->findProperty(Name("SpecialTimer"))->offset;
                offsets_scriptedpawn.CrouchTimer = cls->findProperty(Name("CrouchTimer"))->offset;
                offsets_scriptedpawn.BackpedalTimer = cls->findProperty(Name("BackpedalTimer"))->offset;
                offsets_scriptedpawn.bHasShadow = cls->findProperty(Name("bHasShadow"))->offset;
                offsets_scriptedpawn.ShadowScale = cls->findProperty(Name("ShadowScale"))->offset;
                offsets_scriptedpawn.bDisappear = cls->findProperty(Name("bDisappear"))->offset;
                offsets_scriptedpawn.bInTransientState = cls->findProperty(Name("bInTransientState"))->offset;
                offsets_scriptedpawn.InitialAlliances = cls->findProperty(Name("InitialAlliances"))->offset;
                offsets_scriptedpawn.AlliancesEx = cls->findProperty(Name("AlliancesEx"))->offset;
                offsets_scriptedpawn.bReverseAlliances = cls->findProperty(Name("bReverseAlliances"))->offset;
                offsets_scriptedpawn.BaseAssHeight = cls->findProperty(Name("BaseAssHeight"))->offset;
                offsets_scriptedpawn.EnemyTimeout = cls->findProperty(Name("EnemyTimeout"))->offset;
                offsets_scriptedpawn.CheckPeriod = cls->findProperty(Name("CheckPeriod"))->offset;
                offsets_scriptedpawn.EnemyLastSeen = cls->findProperty(Name("EnemyLastSeen"))->offset;
                offsets_scriptedpawn.SeatSlot = cls->findProperty(Name("SeatSlot"))->offset;
                offsets_scriptedpawn.SeatActor = cls->findProperty(Name("SeatActor"))->offset;
                offsets_scriptedpawn.cycleIndex = cls->findProperty(Name("cycleIndex"))->offset;
                offsets_scriptedpawn.BodyIndex = cls->findProperty(Name("BodyIndex"))->offset;
                offsets_scriptedpawn.bRunningStealthy = cls->findProperty(Name("bRunningStealthy"))->offset;
                offsets_scriptedpawn.bPausing = cls->findProperty(Name("bPausing"))->offset;
                offsets_scriptedpawn.bStaring = cls->findProperty(Name("bStaring"))->offset;
                offsets_scriptedpawn.bAttacking = cls->findProperty(Name("bAttacking"))->offset;
                offsets_scriptedpawn.bDistressed = cls->findProperty(Name("bDistressed"))->offset;
                offsets_scriptedpawn.bStunned = cls->findProperty(Name("bStunned"))->offset;
                offsets_scriptedpawn.bSitting = cls->findProperty(Name("bSitting"))->offset;
                offsets_scriptedpawn.bDancing = cls->findProperty(Name("bDancing"))->offset;
                offsets_scriptedpawn.bCrouching = cls->findProperty(Name("bCrouching"))->offset;
                offsets_scriptedpawn.bCanTurnHead = cls->findProperty(Name("bCanTurnHead"))->offset;
                offsets_scriptedpawn.bTickVisibleOnly = cls->findProperty(Name("bTickVisibleOnly"))->offset;
                offsets_scriptedpawn.bInWorld = cls->findProperty(Name("bInWorld"))->offset;
                offsets_scriptedpawn.WorldPosition = cls->findProperty(Name("WorldPosition"))->offset;
                offsets_scriptedpawn.bWorldCollideActors = cls->findProperty(Name("bWorldCollideActors"))->offset;
                offsets_scriptedpawn.bWorldBlockActors = cls->findProperty(Name("bWorldBlockActors"))->offset;
                offsets_scriptedpawn.bWorldBlockPlayers = cls->findProperty(Name("bWorldBlockPlayers"))->offset;
                offsets_scriptedpawn.bHighlight = cls->findProperty(Name("bHighlight"))->offset;
                offsets_scriptedpawn.bHokeyPokey = cls->findProperty(Name("bHokeyPokey"))->offset;
                offsets_scriptedpawn.bConvEndState = cls->findProperty(Name("bConvEndState"))->offset;
                offsets_scriptedpawn.InitialInventory = cls->findProperty(Name("InitialInventory"))->offset;
                offsets_scriptedpawn.bConversationEndedNormally = cls->findProperty(Name("bConversationEndedNormally"))->offset;
                offsets_scriptedpawn.bInConversation = cls->findProperty(Name("bInConversation"))->offset;
                offsets_scriptedpawn.ConversationActor = cls->findProperty(Name("ConversationActor"))->offset;
                offsets_scriptedpawn.WalkSound = cls->findProperty(Name("WalkSound"))->offset;
                offsets_scriptedpawn.swimBubbleTimer = cls->findProperty(Name("swimBubbleTimer"))->offset;
                offsets_scriptedpawn.bSpawnBubbles = cls->findProperty(Name("bSpawnBubbles"))->offset;
                offsets_scriptedpawn.bUseSecondaryAttack = cls->findProperty(Name("bUseSecondaryAttack"))->offset;
                offsets_scriptedpawn.bWalkAround = cls->findProperty(Name("bWalkAround"))->offset;
                offsets_scriptedpawn.bClearedObstacle = cls->findProperty(Name("bClearedObstacle"))->offset;
                offsets_scriptedpawn.bEnableCheckDest = cls->findProperty(Name("bEnableCheckDest"))->offset;
                offsets_scriptedpawn.TurnDirection = cls->findProperty(Name("TurnDirection"))->offset;
                offsets_scriptedpawn.NextDirection = cls->findProperty(Name("NextDirection"))->offset;
                offsets_scriptedpawn.ActorAvoiding = cls->findProperty(Name("ActorAvoiding"))->offset;
                offsets_scriptedpawn.AvoidWallTimer = cls->findProperty(Name("AvoidWallTimer"))->offset;
                offsets_scriptedpawn.AvoidBumpTimer = cls->findProperty(Name("AvoidBumpTimer"))->offset;
                offsets_scriptedpawn.ObstacleTimer = cls->findProperty(Name("ObstacleTimer"))->offset;
                offsets_scriptedpawn.LastDestLoc = cls->findProperty(Name("LastDestLoc"))->offset;
                offsets_scriptedpawn.LastDestPoint = cls->findProperty(Name("LastDestPoint"))->offset;
                offsets_scriptedpawn.DestAttempts = cls->findProperty(Name("DestAttempts"))->offset;
                offsets_scriptedpawn.DeathTimer = cls->findProperty(Name("DeathTimer"))->offset;
                offsets_scriptedpawn.EnemyTimer = cls->findProperty(Name("EnemyTimer"))->offset;
                offsets_scriptedpawn.TakeHitTimer = cls->findProperty(Name("TakeHitTimer"))->offset;
                offsets_scriptedpawn.ConvOrders = cls->findProperty(Name("ConvOrders"))->offset;
                offsets_scriptedpawn.ConvOrderTag = cls->findProperty(Name("ConvOrderTag"))->offset;
                offsets_scriptedpawn.BurnPeriod = cls->findProperty(Name("BurnPeriod"))->offset;
                offsets_scriptedpawn.FutzTimer = cls->findProperty(Name("FutzTimer"))->offset;
                offsets_scriptedpawn.DistressTimer = cls->findProperty(Name("DistressTimer"))->offset;
                offsets_scriptedpawn.SeatLocation = cls->findProperty(Name("SeatLocation"))->offset;
                offsets_scriptedpawn.SeatHack = cls->findProperty(Name("SeatHack"))->offset;
                offsets_scriptedpawn.bSeatLocationValid = cls->findProperty(Name("bSeatLocationValid"))->offset;
                offsets_scriptedpawn.bSeatHackUsed = cls->findProperty(Name("bSeatHackUsed"))->offset;
                offsets_scriptedpawn.bBurnedToDeath = cls->findProperty(Name("bBurnedToDeath"))->offset;
                offsets_scriptedpawn.bHasCloak = cls->findProperty(Name("bHasCloak"))->offset;
                offsets_scriptedpawn.bCloakOn = cls->findProperty(Name("bCloakOn"))->offset;
                offsets_scriptedpawn.CloakThreshold = cls->findProperty(Name("CloakThreshold"))->offset;
                offsets_scriptedpawn.CloakEMPTimer = cls->findProperty(Name("CloakEMPTimer"))->offset;
                offsets_scriptedpawn.poisonTimer = cls->findProperty(Name("poisonTimer"))->offset;
                offsets_scriptedpawn.poisonCounter = cls->findProperty(Name("poisonCounter"))->offset;
                offsets_scriptedpawn.poisonDamage = cls->findProperty(Name("poisonDamage"))->offset;
                offsets_scriptedpawn.poisoner = cls->findProperty(Name("poisoner"))->offset;
                offsets_scriptedpawn.Carcasses = cls->findProperty(Name("Carcasses"))->offset;
                offsets_scriptedpawn.NumCarcasses = cls->findProperty(Name("NumCarcasses"))->offset;
                offsets_scriptedpawn.walkAnimMult = cls->findProperty(Name("walkAnimMult"))->offset;
                offsets_scriptedpawn.runAnimMult = cls->findProperty(Name("runAnimMult"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("DeusEx.DeusExDecoration");
            if (!cls) {
                std::fprintf(stderr, "missing class DeusEx.DeusExDecoration\n");
            } else {
                offsets_deusexdecoration.HitPoints = cls->findProperty(Name("HitPoints"))->offset;
                offsets_deusexdecoration.minDamageThreshold = cls->findProperty(Name("minDamageThreshold"))->offset;
                offsets_deusexdecoration.bInvincible = cls->findProperty(Name("bInvincible"))->offset;
                offsets_deusexdecoration.FragType = cls->findProperty(Name("FragType"))->offset;
                offsets_deusexdecoration.bFloating = cls->findProperty(Name("bFloating"))->offset;
                offsets_deusexdecoration.origRot = cls->findProperty(Name("origRot"))->offset;
                offsets_deusexdecoration.moverTag = cls->findProperty(Name("moverTag"))->offset;
                offsets_deusexdecoration.bFlammable = cls->findProperty(Name("bFlammable"))->offset;
                offsets_deusexdecoration.Flammability = cls->findProperty(Name("Flammability"))->offset;
                offsets_deusexdecoration.bExplosive = cls->findProperty(Name("bExplosive"))->offset;
                offsets_deusexdecoration.explosionDamage = cls->findProperty(Name("explosionDamage"))->offset;
                offsets_deusexdecoration.explosionRadius = cls->findProperty(Name("explosionRadius"))->offset;
                offsets_deusexdecoration.bHighlight = cls->findProperty(Name("bHighlight"))->offset;
                offsets_deusexdecoration.bCanBeBase = cls->findProperty(Name("bCanBeBase"))->offset;
                offsets_deusexdecoration.bGenerateFlies = cls->findProperty(Name("bGenerateFlies"))->offset;
                offsets_deusexdecoration.pushSoundId = cls->findProperty(Name("pushSoundId"))->offset;
                offsets_deusexdecoration.gradualHurtSteps = cls->findProperty(Name("gradualHurtSteps"))->offset;
                offsets_deusexdecoration.gradualHurtCounter = cls->findProperty(Name("gradualHurtCounter"))->offset;
                offsets_deusexdecoration.NextState = cls->findProperty(Name("NextState"))->offset;
                offsets_deusexdecoration.NextLabel = cls->findProperty(Name("NextLabel"))->offset;
                offsets_deusexdecoration.flyGen = cls->findProperty(Name("flyGen"))->offset;
                if (UProperty* property = cls->findProperty(Name("ItemArticle"))) {
                    offsets_deusexdecoration.ItemArticle = property->offset;
                }
                offsets_deusexdecoration.ItemName = cls->findProperty(Name("ItemName"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("DeusEx.DeusExSaveInfo");
            if (!cls) {
                std::fprintf(stderr, "missing class DeusEx.DeusExSaveInfo\n");
            } else {
                offsets_deusexsaveinfo.Year = cls->findProperty(Name("Year"))->offset;
                offsets_deusexsaveinfo.Month = cls->findProperty(Name("Month"))->offset;
                offsets_deusexsaveinfo.Day = cls->findProperty(Name("Day"))->offset;
                offsets_deusexsaveinfo.Hour = cls->findProperty(Name("Hour"))->offset;
                offsets_deusexsaveinfo.Minute = cls->findProperty(Name("Minute"))->offset;
                offsets_deusexsaveinfo.Second = cls->findProperty(Name("Second"))->offset;
                offsets_deusexsaveinfo.DirectoryIndex = cls->findProperty(Name("DirectoryIndex"))->offset;
                offsets_deusexsaveinfo.Description = cls->findProperty(Name("Description"))->offset;
                offsets_deusexsaveinfo.MissionLocation = cls->findProperty(Name("MissionLocation"))->offset;
                offsets_deusexsaveinfo.MapName = cls->findProperty(Name("MapName"))->offset;
                offsets_deusexsaveinfo.Snapshot = cls->findProperty(Name("Snapshot"))->offset;
                offsets_deusexsaveinfo.saveCount = cls->findProperty(Name("saveCount"))->offset;
                offsets_deusexsaveinfo.saveTime = cls->findProperty(Name("saveTime"))->offset;
                offsets_deusexsaveinfo.bCheatsEnabled = cls->findProperty(Name("bCheatsEnabled"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("DeusEx.GameDirectory");
            if (!cls) {
                std::fprintf(stderr, "missing class DeusEx.GameDirectory\n");
            } else {
                offsets_gamedirectory.gameDirectoryType = cls->findProperty(Name("gameDirectoryType"))->offset;
                offsets_gamedirectory.currentFilter = cls->findProperty(Name("currentFilter"))->offset;
                offsets_gamedirectory.directoryList = cls->findProperty(Name("directoryList"))->offset;
                offsets_gamedirectory.loadedSaveInfoPointers = cls->findProperty(Name("loadedSaveInfoPointers"))->offset;
                offsets_gamedirectory.tempSaveInfo = cls->findProperty(Name("tempSaveInfo"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("DeusEx.DumpLocation");
            if (!cls) {
                std::fprintf(stderr, "missing class DeusEx.DumpLocation\n");
            } else {
                offsets_dumplocation.currentDumpFileLocation = cls->findProperty(Name("currentDumpFileLocation"))->offset;
                offsets_dumplocation.currentDumpLocation = cls->findProperty(Name("currentDumpLocation"))->offset;
                offsets_dumplocation.dumpFileDirectory = cls->findProperty(Name("dumpFileDirectory"))->offset;
                offsets_dumplocation.currentDumpFileIndex = cls->findProperty(Name("currentDumpFileIndex"))->offset;
                offsets_dumplocation.currentDumpLocationIndex = cls->findProperty(Name("currentDumpLocationIndex"))->offset;
                offsets_dumplocation.currentUser = cls->findProperty(Name("currentUser"))->offset;
                offsets_dumplocation.dumpFile = cls->findProperty(Name("dumpFile"))->offset;
                offsets_dumplocation.dumpLocationCount = cls->findProperty(Name("dumpLocationCount"))->offset;
                offsets_dumplocation.Player = cls->findProperty(Name("Player"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("DeusEx.ParticleIterator");
            if (!cls) {
                std::fprintf(stderr, "missing class DeusEx.ParticleIterator\n");
            } else {
                offsets_particleiterator.Particles = cls->findProperty(Name("Particles"))->offset;
                offsets_particleiterator.nextFreeParticle = cls->findProperty(Name("nextFreeParticle"))->offset;
                offsets_particleiterator.proxy = cls->findProperty(Name("proxy"))->offset;
                if (UProperty* property = cls->findProperty(Name("bOwnerUsesGravity"))) {
                    offsets_particleiterator.bOwnerUsesGravity = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("bOwnerScales"))) {
                    offsets_particleiterator.bOwnerScales = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("bOwnerFades"))) {
                    offsets_particleiterator.bOwnerFades = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("OwnerZoneGravity"))) {
                    offsets_particleiterator.OwnerZoneGravity = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("OwnerRiseRate"))) {
                    offsets_particleiterator.OwnerRiseRate = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("OwnerLifeSpan"))) {
                    offsets_particleiterator.OwnerLifeSpan = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("OwnerDrawScale"))) {
                    offsets_particleiterator.OwnerDrawScale = property->offset;
                }
            }
        }
        {
            UClass* cls = objects::findClass("DeusEx.DeusExLevelInfo");
            if (!cls) {
                std::fprintf(stderr, "missing class DeusEx.DeusExLevelInfo\n");
            } else {
                offsets_deusexlevelinfo.MapName = cls->findProperty(Name("MapName"))->offset;
                offsets_deusexlevelinfo.MapAuthor = cls->findProperty(Name("MapAuthor"))->offset;
                offsets_deusexlevelinfo.MissionLocation = cls->findProperty(Name("MissionLocation"))->offset;
                offsets_deusexlevelinfo.missionNumber = cls->findProperty(Name("missionNumber"))->offset;
                offsets_deusexlevelinfo.bMultiPlayerMap = cls->findProperty(Name("bMultiPlayerMap"))->offset;
                offsets_deusexlevelinfo.Script = cls->findProperty(Name("Script"))->offset;
                offsets_deusexlevelinfo.TrueNorth = cls->findProperty(Name("TrueNorth"))->offset;
                offsets_deusexlevelinfo.startupMessage = cls->findProperty(Name("startupMessage"))->offset;
                if (UProperty* property = cls->findProperty(Name("ConversationPackage"))) {
                    offsets_deusexlevelinfo.ConversationPackage = property->offset;
                }
            }
        }
        {
            UClass* cls = objects::findClass("DeusEx.DeusExRootWindow");
            if (!cls) {
                std::fprintf(stderr, "missing class DeusEx.DeusExRootWindow\n");
            } else {
                offsets_deusexrootwindow.HUD = cls->findProperty(Name("HUD"))->offset;
                offsets_deusexrootwindow.actorDisplay = cls->findProperty(Name("actorDisplay"))->offset;
                offsets_deusexrootwindow.scopeView = cls->findProperty(Name("scopeView"))->offset;
                offsets_deusexrootwindow.MaxWinStack = cls->findProperty(Name("MaxWinStack"))->offset;
                offsets_deusexrootwindow.winStack = cls->findProperty(Name("winStack"))->offset;
                offsets_deusexrootwindow.winCount = cls->findProperty(Name("winCount"))->offset;
                offsets_deusexrootwindow.bIgnoreHotkeys = cls->findProperty(Name("bIgnoreHotkeys"))->offset;
                offsets_deusexrootwindow.defaultMenuColorSchemes = cls->findProperty(Name("defaultMenuColorSchemes"))->offset;
                offsets_deusexrootwindow.defaultHUDColorSchemes = cls->findProperty(Name("defaultHUDColorSchemes"))->offset;
                offsets_deusexrootwindow.menuColorScheme = cls->findProperty(Name("menuColorScheme"))->offset;
                offsets_deusexrootwindow.bMenuTranslucency = cls->findProperty(Name("bMenuTranslucency"))->offset;
                offsets_deusexrootwindow.HUDColorScheme = cls->findProperty(Name("HUDColorScheme"))->offset;
                offsets_deusexrootwindow.bHUDTranslucency = cls->findProperty(Name("bHUDTranslucency"))->offset;
                offsets_deusexrootwindow.colSnapshot = cls->findProperty(Name("colSnapshot"))->offset;
                offsets_deusexrootwindow.Snapshot = cls->findProperty(Name("Snapshot"))->offset;
                offsets_deusexrootwindow.snapshotWidth = cls->findProperty(Name("snapshotWidth"))->offset;
                offsets_deusexrootwindow.snapshotHeight = cls->findProperty(Name("snapshotHeight"))->offset;
                offsets_deusexrootwindow.bUIPaused = cls->findProperty(Name("bUIPaused"))->offset;
                offsets_deusexrootwindow.DataVaultFunctions = cls->findProperty(Name("DataVaultFunctions"))->offset;
                offsets_deusexrootwindow.QuickLoadTitle = cls->findProperty(Name("QuickLoadTitle"))->offset;
                offsets_deusexrootwindow.QuickLoadMessage = cls->findProperty(Name("QuickLoadMessage"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("DeusEx.DeusExGameInfo");
            if (!cls) {
                std::fprintf(stderr, "missing class DeusEx.DeusExGameInfo\n");
            } else {
            }
        }
        {
            UClass* cls = objects::findClass("DeusEx.DeusExWeapon");
            if (!cls) {
                std::fprintf(stderr, "missing class DeusEx.DeusExWeapon\n");
            } else {
                offsets_deusexweapon.bReadyToFire = cls->findProperty(Name("bReadyToFire"))->offset;
                offsets_deusexweapon.LowAmmoWaterMark = cls->findProperty(Name("LowAmmoWaterMark"))->offset;
                offsets_deusexweapon.ClipCount = cls->findProperty(Name("ClipCount"))->offset;
                offsets_deusexweapon.GoverningSkill = cls->findProperty(Name("GoverningSkill"))->offset;
                offsets_deusexweapon.NoiseLevel = cls->findProperty(Name("NoiseLevel"))->offset;
                offsets_deusexweapon.EnemyEffective = cls->findProperty(Name("EnemyEffective"))->offset;
                offsets_deusexweapon.EnviroEffective = cls->findProperty(Name("EnviroEffective"))->offset;
                offsets_deusexweapon.Concealability = cls->findProperty(Name("Concealability"))->offset;
                offsets_deusexweapon.bAutomatic = cls->findProperty(Name("bAutomatic"))->offset;
                offsets_deusexweapon.ShotTime = cls->findProperty(Name("ShotTime"))->offset;
                offsets_deusexweapon.reloadTime = cls->findProperty(Name("reloadTime"))->offset;
                offsets_deusexweapon.HitDamage = cls->findProperty(Name("HitDamage"))->offset;
                offsets_deusexweapon.maxRange = cls->findProperty(Name("maxRange"))->offset;
                offsets_deusexweapon.AccurateRange = cls->findProperty(Name("AccurateRange"))->offset;
                offsets_deusexweapon.BaseAccuracy = cls->findProperty(Name("BaseAccuracy"))->offset;
                offsets_deusexweapon.bCanHaveScope = cls->findProperty(Name("bCanHaveScope"))->offset;
                offsets_deusexweapon.bHasScope = cls->findProperty(Name("bHasScope"))->offset;
                offsets_deusexweapon.ScopeFOV = cls->findProperty(Name("ScopeFOV"))->offset;
                offsets_deusexweapon.bZoomed = cls->findProperty(Name("bZoomed"))->offset;
                offsets_deusexweapon.bWasZoomed = cls->findProperty(Name("bWasZoomed"))->offset;
                offsets_deusexweapon.bCanHaveLaser = cls->findProperty(Name("bCanHaveLaser"))->offset;
                offsets_deusexweapon.bHasLaser = cls->findProperty(Name("bHasLaser"))->offset;
                offsets_deusexweapon.bLasing = cls->findProperty(Name("bLasing"))->offset;
                offsets_deusexweapon.emitter = cls->findProperty(Name("emitter"))->offset;
                offsets_deusexweapon.bCanHaveSilencer = cls->findProperty(Name("bCanHaveSilencer"))->offset;
                offsets_deusexweapon.bHasSilencer = cls->findProperty(Name("bHasSilencer"))->offset;
                offsets_deusexweapon.bCanTrack = cls->findProperty(Name("bCanTrack"))->offset;
                offsets_deusexweapon.LockTime = cls->findProperty(Name("LockTime"))->offset;
                offsets_deusexweapon.LockTimer = cls->findProperty(Name("LockTimer"))->offset;
                if (UProperty* property = cls->findProperty(Name("MaintainLockTimer"))) {
                    offsets_deusexweapon.MaintainLockTimer = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("LockTarget"))) {
                    offsets_deusexweapon.LockTarget = property->offset;
                }
                offsets_deusexweapon.Target = cls->findProperty(Name("Target"))->offset;
                offsets_deusexweapon.LockMode = cls->findProperty(Name("LockMode"))->offset;
                offsets_deusexweapon.TargetMessage = cls->findProperty(Name("TargetMessage"))->offset;
                offsets_deusexweapon.TargetRange = cls->findProperty(Name("TargetRange"))->offset;
                offsets_deusexweapon.LockedSound = cls->findProperty(Name("LockedSound"))->offset;
                offsets_deusexweapon.TrackingSound = cls->findProperty(Name("TrackingSound"))->offset;
                offsets_deusexweapon.SoundTimer = cls->findProperty(Name("SoundTimer"))->offset;
                offsets_deusexweapon.AmmoNames = cls->findProperty(Name("AmmoNames"))->offset;
                offsets_deusexweapon.ProjectileNames = cls->findProperty(Name("ProjectileNames"))->offset;
                offsets_deusexweapon.AreaOfEffect = cls->findProperty(Name("AreaOfEffect"))->offset;
                offsets_deusexweapon.bPenetrating = cls->findProperty(Name("bPenetrating"))->offset;
                offsets_deusexweapon.StunDuration = cls->findProperty(Name("StunDuration"))->offset;
                offsets_deusexweapon.bHasMuzzleFlash = cls->findProperty(Name("bHasMuzzleFlash"))->offset;
                offsets_deusexweapon.bHandToHand = cls->findProperty(Name("bHandToHand"))->offset;
                if (UProperty* property = cls->findProperty(Name("SwingOffset"))) {
                    offsets_deusexweapon.SwingOffset = property->offset;
                }
                offsets_deusexweapon.recoilStrength = cls->findProperty(Name("recoilStrength"))->offset;
                offsets_deusexweapon.bFiring = cls->findProperty(Name("bFiring"))->offset;
                offsets_deusexweapon.bOwnerWillNotify = cls->findProperty(Name("bOwnerWillNotify"))->offset;
                offsets_deusexweapon.bFallbackWeapon = cls->findProperty(Name("bFallbackWeapon"))->offset;
                offsets_deusexweapon.bNativeAttack = cls->findProperty(Name("bNativeAttack"))->offset;
                offsets_deusexweapon.bEmitWeaponDrawn = cls->findProperty(Name("bEmitWeaponDrawn"))->offset;
                offsets_deusexweapon.bUseWhileCrouched = cls->findProperty(Name("bUseWhileCrouched"))->offset;
                offsets_deusexweapon.bUseAsDrawnWeapon = cls->findProperty(Name("bUseAsDrawnWeapon"))->offset;
                if (UProperty* property = cls->findProperty(Name("bWasInFiring"))) {
                    offsets_deusexweapon.bWasInFiring = property->offset;
                }
                offsets_deusexweapon.bNearWall = cls->findProperty(Name("bNearWall"))->offset;
                offsets_deusexweapon.placeLocation = cls->findProperty(Name("placeLocation"))->offset;
                offsets_deusexweapon.placeNormal = cls->findProperty(Name("placeNormal"))->offset;
                offsets_deusexweapon.placeMover = cls->findProperty(Name("placeMover"))->offset;
                offsets_deusexweapon.shaketimer = cls->findProperty(Name("shaketimer"))->offset;
                offsets_deusexweapon.ShakeYaw = cls->findProperty(Name("ShakeYaw"))->offset;
                offsets_deusexweapon.ShakePitch = cls->findProperty(Name("ShakePitch"))->offset;
                offsets_deusexweapon.AIMinRange = cls->findProperty(Name("AIMinRange"))->offset;
                offsets_deusexweapon.AIMaxRange = cls->findProperty(Name("AIMaxRange"))->offset;
                offsets_deusexweapon.AITimeLimit = cls->findProperty(Name("AITimeLimit"))->offset;
                offsets_deusexweapon.AIFireDelay = cls->findProperty(Name("AIFireDelay"))->offset;
                offsets_deusexweapon.standingTimer = cls->findProperty(Name("standingTimer"))->offset;
                offsets_deusexweapon.currentAccuracy = cls->findProperty(Name("currentAccuracy"))->offset;
                offsets_deusexweapon.flash = cls->findProperty(Name("flash"))->offset;
                if (UProperty* property = cls->findProperty(Name("MinSpreadAcc"))) {
                    offsets_deusexweapon.MinSpreadAcc = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("MinProjSpreadAcc"))) {
                    offsets_deusexweapon.MinProjSpreadAcc = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("MinWeaponAcc"))) {
                    offsets_deusexweapon.MinWeaponAcc = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("bNeedToSetMPPickupAmmo"))) {
                    offsets_deusexweapon.bNeedToSetMPPickupAmmo = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("bDestroyOnFinish"))) {
                    offsets_deusexweapon.bDestroyOnFinish = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("mpReloadTime"))) {
                    offsets_deusexweapon.mpReloadTime = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("mpHitDamage"))) {
                    offsets_deusexweapon.mpHitDamage = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("mpBaseAccuracy"))) {
                    offsets_deusexweapon.mpBaseAccuracy = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("mpAccurateRange"))) {
                    offsets_deusexweapon.mpAccurateRange = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("mpMaxRange"))) {
                    offsets_deusexweapon.mpMaxRange = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("mpReloadCount"))) {
                    offsets_deusexweapon.mpReloadCount = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("mpPickupAmmoCount"))) {
                    offsets_deusexweapon.mpPickupAmmoCount = property->offset;
                }
                offsets_deusexweapon.bCanHaveModBaseAccuracy = cls->findProperty(Name("bCanHaveModBaseAccuracy"))->offset;
                offsets_deusexweapon.bCanHaveModReloadCount = cls->findProperty(Name("bCanHaveModReloadCount"))->offset;
                offsets_deusexweapon.bCanHaveModAccurateRange = cls->findProperty(Name("bCanHaveModAccurateRange"))->offset;
                offsets_deusexweapon.bCanHaveModReloadTime = cls->findProperty(Name("bCanHaveModReloadTime"))->offset;
                offsets_deusexweapon.bCanHaveModRecoilStrength = cls->findProperty(Name("bCanHaveModRecoilStrength"))->offset;
                offsets_deusexweapon.ModBaseAccuracy = cls->findProperty(Name("ModBaseAccuracy"))->offset;
                offsets_deusexweapon.ModReloadCount = cls->findProperty(Name("ModReloadCount"))->offset;
                offsets_deusexweapon.ModAccurateRange = cls->findProperty(Name("ModAccurateRange"))->offset;
                offsets_deusexweapon.ModReloadTime = cls->findProperty(Name("ModReloadTime"))->offset;
                offsets_deusexweapon.ModRecoilStrength = cls->findProperty(Name("ModRecoilStrength"))->offset;
                offsets_deusexweapon.msgCannotBeReloaded = cls->findProperty(Name("msgCannotBeReloaded"))->offset;
                offsets_deusexweapon.msgOutOf = cls->findProperty(Name("msgOutOf"))->offset;
                offsets_deusexweapon.msgNowHas = cls->findProperty(Name("msgNowHas"))->offset;
                offsets_deusexweapon.msgAlreadyHas = cls->findProperty(Name("msgAlreadyHas"))->offset;
                offsets_deusexweapon.msgNone = cls->findProperty(Name("msgNone"))->offset;
                offsets_deusexweapon.msgLockInvalid = cls->findProperty(Name("msgLockInvalid"))->offset;
                offsets_deusexweapon.msgLockRange = cls->findProperty(Name("msgLockRange"))->offset;
                offsets_deusexweapon.msgLockAcquire = cls->findProperty(Name("msgLockAcquire"))->offset;
                offsets_deusexweapon.msgLockLocked = cls->findProperty(Name("msgLockLocked"))->offset;
                offsets_deusexweapon.msgRangeUnit = cls->findProperty(Name("msgRangeUnit"))->offset;
                offsets_deusexweapon.msgTimeUnit = cls->findProperty(Name("msgTimeUnit"))->offset;
                offsets_deusexweapon.msgMassUnit = cls->findProperty(Name("msgMassUnit"))->offset;
                offsets_deusexweapon.msgNotWorking = cls->findProperty(Name("msgNotWorking"))->offset;
                offsets_deusexweapon.msgInfoAmmoLoaded = cls->findProperty(Name("msgInfoAmmoLoaded"))->offset;
                offsets_deusexweapon.msgInfoAmmo = cls->findProperty(Name("msgInfoAmmo"))->offset;
                offsets_deusexweapon.msgInfoDamage = cls->findProperty(Name("msgInfoDamage"))->offset;
                offsets_deusexweapon.msgInfoClip = cls->findProperty(Name("msgInfoClip"))->offset;
                offsets_deusexweapon.msgInfoROF = cls->findProperty(Name("msgInfoROF"))->offset;
                offsets_deusexweapon.msgInfoReload = cls->findProperty(Name("msgInfoReload"))->offset;
                offsets_deusexweapon.msgInfoRecoil = cls->findProperty(Name("msgInfoRecoil"))->offset;
                offsets_deusexweapon.msgInfoAccuracy = cls->findProperty(Name("msgInfoAccuracy"))->offset;
                offsets_deusexweapon.msgInfoAccRange = cls->findProperty(Name("msgInfoAccRange"))->offset;
                offsets_deusexweapon.msgInfoMaxRange = cls->findProperty(Name("msgInfoMaxRange"))->offset;
                offsets_deusexweapon.msgInfoMass = cls->findProperty(Name("msgInfoMass"))->offset;
                offsets_deusexweapon.msgInfoLaser = cls->findProperty(Name("msgInfoLaser"))->offset;
                offsets_deusexweapon.msgInfoScope = cls->findProperty(Name("msgInfoScope"))->offset;
                offsets_deusexweapon.msgInfoSilencer = cls->findProperty(Name("msgInfoSilencer"))->offset;
                offsets_deusexweapon.msgInfoNA = cls->findProperty(Name("msgInfoNA"))->offset;
                offsets_deusexweapon.msgInfoYes = cls->findProperty(Name("msgInfoYes"))->offset;
                offsets_deusexweapon.msgInfoNo = cls->findProperty(Name("msgInfoNo"))->offset;
                offsets_deusexweapon.msgInfoAuto = cls->findProperty(Name("msgInfoAuto"))->offset;
                offsets_deusexweapon.msgInfoSingle = cls->findProperty(Name("msgInfoSingle"))->offset;
                offsets_deusexweapon.msgInfoRounds = cls->findProperty(Name("msgInfoRounds"))->offset;
                offsets_deusexweapon.msgInfoRoundsPerSec = cls->findProperty(Name("msgInfoRoundsPerSec"))->offset;
                offsets_deusexweapon.msgInfoSkill = cls->findProperty(Name("msgInfoSkill"))->offset;
                offsets_deusexweapon.msgInfoWeaponStats = cls->findProperty(Name("msgInfoWeaponStats"))->offset;
                if (UProperty* property = cls->findProperty(Name("bClientReadyToFire"))) {
                    offsets_deusexweapon.bClientReadyToFire = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("bClientReady"))) {
                    offsets_deusexweapon.bClientReady = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("bInProcess"))) {
                    offsets_deusexweapon.bInProcess = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("bFlameOn"))) {
                    offsets_deusexweapon.bFlameOn = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("bLooping"))) {
                    offsets_deusexweapon.bLooping = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("SimClipCount"))) {
                    offsets_deusexweapon.SimClipCount = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("flameShotCount"))) {
                    offsets_deusexweapon.flameShotCount = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("SimAmmoAmount"))) {
                    offsets_deusexweapon.SimAmmoAmount = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("TimeLockSet"))) {
                    offsets_deusexweapon.TimeLockSet = property->offset;
                }
            }
        }
        {
            UClass* cls = objects::findClass("DeusEx.DeusExCarcass");
            if (!cls) {
                std::fprintf(stderr, "missing class DeusEx.DeusExCarcass\n");
            } else {
                offsets_deusexcarcass.Mesh2 = cls->findProperty(Name("Mesh2"))->offset;
                offsets_deusexcarcass.Mesh3 = cls->findProperty(Name("Mesh3"))->offset;
                offsets_deusexcarcass.InitialInventory = cls->findProperty(Name("InitialInventory"))->offset;
                offsets_deusexcarcass.bHighlight = cls->findProperty(Name("bHighlight"))->offset;
                offsets_deusexcarcass.KillerBindName = cls->findProperty(Name("KillerBindName"))->offset;
                offsets_deusexcarcass.KillerAlliance = cls->findProperty(Name("KillerAlliance"))->offset;
                offsets_deusexcarcass.bGenerateFlies = cls->findProperty(Name("bGenerateFlies"))->offset;
                offsets_deusexcarcass.flyGen = cls->findProperty(Name("flyGen"))->offset;
                offsets_deusexcarcass.Alliance = cls->findProperty(Name("Alliance"))->offset;
                offsets_deusexcarcass.CarcassName = cls->findProperty(Name("CarcassName"))->offset;
                offsets_deusexcarcass.MaxDamage = cls->findProperty(Name("MaxDamage"))->offset;
                offsets_deusexcarcass.bNotDead = cls->findProperty(Name("bNotDead"))->offset;
                offsets_deusexcarcass.bEmitCarcass = cls->findProperty(Name("bEmitCarcass"))->offset;
                if (UProperty* property = cls->findProperty(Name("bQueuedDestroy"))) {
                    offsets_deusexcarcass.bQueuedDestroy = property->offset;
                }
                offsets_deusexcarcass.bInit = cls->findProperty(Name("bInit"))->offset;
                offsets_deusexcarcass.bSearchMsgPrinted = cls->findProperty(Name("bSearchMsgPrinted"))->offset;
                offsets_deusexcarcass.msgSearching = cls->findProperty(Name("msgSearching"))->offset;
                offsets_deusexcarcass.msgEmpty = cls->findProperty(Name("msgEmpty"))->offset;
                offsets_deusexcarcass.msgNotDead = cls->findProperty(Name("msgNotDead"))->offset;
                offsets_deusexcarcass.msgAnimalCarcass = cls->findProperty(Name("msgAnimalCarcass"))->offset;
                offsets_deusexcarcass.msgCannotPickup = cls->findProperty(Name("msgCannotPickup"))->offset;
                if (UProperty* property = cls->findProperty(Name("msgRecharged"))) {
                    offsets_deusexcarcass.msgRecharged = property->offset;
                }
                offsets_deusexcarcass.ItemName = cls->findProperty(Name("ItemName"))->offset;
                offsets_deusexcarcass.bInvincible = cls->findProperty(Name("bInvincible"))->offset;
                offsets_deusexcarcass.bAnimalCarcass = cls->findProperty(Name("bAnimalCarcass"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("DeusEx.DeusExPickup");
            if (!cls) {
                std::fprintf(stderr, "missing class DeusEx.DeusExPickup\n");
            } else {
                offsets_deusexpickup.bBreakable = cls->findProperty(Name("bBreakable"))->offset;
                offsets_deusexpickup.FragType = cls->findProperty(Name("FragType"))->offset;
                offsets_deusexpickup.maxCopies = cls->findProperty(Name("maxCopies"))->offset;
                offsets_deusexpickup.CountLabel = cls->findProperty(Name("CountLabel"))->offset;
                offsets_deusexpickup.msgTooMany = cls->findProperty(Name("msgTooMany"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("DeusEx.DeusExProjectile");
            if (!cls) {
                std::fprintf(stderr, "missing class DeusEx.DeusExProjectile\n");
            } else {
                offsets_deusexprojectile.bExplodes = cls->findProperty(Name("bExplodes"))->offset;
                offsets_deusexprojectile.bBlood = cls->findProperty(Name("bBlood"))->offset;
                offsets_deusexprojectile.bDebris = cls->findProperty(Name("bDebris"))->offset;
                offsets_deusexprojectile.bStickToWall = cls->findProperty(Name("bStickToWall"))->offset;
                offsets_deusexprojectile.bStuck = cls->findProperty(Name("bStuck"))->offset;
                offsets_deusexprojectile.initDir = cls->findProperty(Name("initDir"))->offset;
                offsets_deusexprojectile.blastRadius = cls->findProperty(Name("blastRadius"))->offset;
                offsets_deusexprojectile.damagee = cls->findProperty(Name("damagee"))->offset;
                offsets_deusexprojectile.DamageType = cls->findProperty(Name("DamageType"))->offset;
                offsets_deusexprojectile.AccurateRange = cls->findProperty(Name("AccurateRange"))->offset;
                offsets_deusexprojectile.maxRange = cls->findProperty(Name("maxRange"))->offset;
                offsets_deusexprojectile.initLoc = cls->findProperty(Name("initLoc"))->offset;
                offsets_deusexprojectile.bTracking = cls->findProperty(Name("bTracking"))->offset;
                offsets_deusexprojectile.Target = cls->findProperty(Name("Target"))->offset;
                offsets_deusexprojectile.Time = cls->findProperty(Name("Time"))->offset;
                offsets_deusexprojectile.MinDrawScale = cls->findProperty(Name("MinDrawScale"))->offset;
                offsets_deusexprojectile.maxDrawScale = cls->findProperty(Name("maxDrawScale"))->offset;
                if (UProperty* property = cls->findProperty(Name("LastSeenLoc"))) {
                    offsets_deusexprojectile.LastSeenLoc = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("NetworkTargetLoc"))) {
                    offsets_deusexprojectile.NetworkTargetLoc = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("bHasNetworkTarget"))) {
                    offsets_deusexprojectile.bHasNetworkTarget = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("bHadLocalTarget"))) {
                    offsets_deusexprojectile.bHadLocalTarget = property->offset;
                }
                offsets_deusexprojectile.gradualHurtSteps = cls->findProperty(Name("gradualHurtSteps"))->offset;
                offsets_deusexprojectile.gradualHurtCounter = cls->findProperty(Name("gradualHurtCounter"))->offset;
                offsets_deusexprojectile.bEmitDanger = cls->findProperty(Name("bEmitDanger"))->offset;
                offsets_deusexprojectile.spawnWeaponClass = cls->findProperty(Name("spawnWeaponClass"))->offset;
                offsets_deusexprojectile.spawnAmmoClass = cls->findProperty(Name("spawnAmmoClass"))->offset;
                if (UProperty* property = cls->findProperty(Name("bIgnoresNanoDefense"))) {
                    offsets_deusexprojectile.bIgnoresNanoDefense = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("bAggressiveExploded"))) {
                    offsets_deusexprojectile.bAggressiveExploded = property->offset;
                }
                offsets_deusexprojectile.ItemName = cls->findProperty(Name("ItemName"))->offset;
                if (UProperty* property = cls->findProperty(Name("ItemArticle"))) {
                    offsets_deusexprojectile.ItemArticle = property->offset;
                }
            }
        }
        {
            UClass* cls = objects::findClass("DeusEx.ConPlay");
            if (!cls) {
                std::fprintf(stderr, "missing class DeusEx.ConPlay\n");
            } else {
                offsets_conplay.currentEvent = cls->findProperty(Name("currentEvent"))->offset;
                offsets_conplay.conWinFirst = cls->findProperty(Name("conWinFirst"))->offset;
                offsets_conplay.currentSpeaker = cls->findProperty(Name("currentSpeaker"))->offset;
                offsets_conplay.currentSpeakingTo = cls->findProperty(Name("currentSpeakingTo"))->offset;
                offsets_conplay.SetupInitialCamera = cls->findProperty(Name("SetupInitialCamera"))->offset;
                offsets_conplay.speakingActor = cls->findProperty(Name("speakingActor"))->offset;
                offsets_conplay.cameraInfo = cls->findProperty(Name("cameraInfo"))->offset;
                offsets_conplay.randomCamera = cls->findProperty(Name("randomCamera"))->offset;
                offsets_conplay.interactiveCamera = cls->findProperty(Name("interactiveCamera"))->offset;
                offsets_conplay.pendingCameraEvent = cls->findProperty(Name("pendingCameraEvent"))->offset;
                offsets_conplay.bSetupInitialCamera = cls->findProperty(Name("bSetupInitialCamera"))->offset;
                offsets_conplay.bEndingConversation = cls->findProperty(Name("bEndingConversation"))->offset;
                offsets_conplay.bWaitingForConWin = cls->findProperty(Name("bWaitingForConWin"))->offset;
                offsets_conplay.bActorsTurned = cls->findProperty(Name("bActorsTurned"))->offset;
                offsets_conplay.bSaveContinueSpeech = cls->findProperty(Name("bSaveContinueSpeech"))->offset;
                offsets_conplay.bSaveNoPlayedFlag = cls->findProperty(Name("bSaveNoPlayedFlag"))->offset;
                offsets_conplay.playerInHand = cls->findProperty(Name("playerInHand"))->offset;
                offsets_conplay.perCharDelay = cls->findProperty(Name("perCharDelay"))->offset;
                offsets_conplay.minimumTextPause = cls->findProperty(Name("minimumTextPause"))->offset;
                offsets_conplay.ConversationSpeechFonts = cls->findProperty(Name("ConversationSpeechFonts"))->offset;
                offsets_conplay.ConversationNameFonts = cls->findProperty(Name("ConversationNameFonts"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("DeusEx.DeusExMover");
            if (!cls) {
                std::fprintf(stderr, "missing class DeusEx.DeusExMover\n");
            } else {
                offsets_deusexmover.bOneWay = cls->findProperty(Name("bOneWay"))->offset;
                offsets_deusexmover.bLocked = cls->findProperty(Name("bLocked"))->offset;
                offsets_deusexmover.bPickable = cls->findProperty(Name("bPickable"))->offset;
                offsets_deusexmover.lockStrength = cls->findProperty(Name("lockStrength"))->offset;
                if (UProperty* property = cls->findProperty(Name("initiallockStrength"))) {
                    offsets_deusexmover.initiallockStrength = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("bInitialLocked"))) {
                    offsets_deusexmover.bInitialLocked = property->offset;
                }
                offsets_deusexmover.bBreakable = cls->findProperty(Name("bBreakable"))->offset;
                offsets_deusexmover.doorStrength = cls->findProperty(Name("doorStrength"))->offset;
                offsets_deusexmover.KeyIDNeeded = cls->findProperty(Name("KeyIDNeeded"))->offset;
                offsets_deusexmover.bHighlight = cls->findProperty(Name("bHighlight"))->offset;
                offsets_deusexmover.bFrobbable = cls->findProperty(Name("bFrobbable"))->offset;
                offsets_deusexmover.bPicking = cls->findProperty(Name("bPicking"))->offset;
                offsets_deusexmover.pickValue = cls->findProperty(Name("pickValue"))->offset;
                offsets_deusexmover.pickTime = cls->findProperty(Name("pickTime"))->offset;
                offsets_deusexmover.numPicks = cls->findProperty(Name("numPicks"))->offset;
                if (UProperty* property = cls->findProperty(Name("TicksSinceLastPick"))) {
                    offsets_deusexmover.TicksSinceLastPick = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("TicksPerPick"))) {
                    offsets_deusexmover.TicksPerPick = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("LastTickTime"))) {
                    offsets_deusexmover.LastTickTime = property->offset;
                }
                offsets_deusexmover.pickPlayer = cls->findProperty(Name("pickPlayer"))->offset;
                offsets_deusexmover.curPick = cls->findProperty(Name("curPick"))->offset;
                offsets_deusexmover.minDamageThreshold = cls->findProperty(Name("minDamageThreshold"))->offset;
                offsets_deusexmover.bDestroyed = cls->findProperty(Name("bDestroyed"))->offset;
                offsets_deusexmover.NumFragments = cls->findProperty(Name("NumFragments"))->offset;
                offsets_deusexmover.FragmentScale = cls->findProperty(Name("FragmentScale"))->offset;
                offsets_deusexmover.FragmentSpread = cls->findProperty(Name("FragmentSpread"))->offset;
                offsets_deusexmover.FragmentClass = cls->findProperty(Name("FragmentClass"))->offset;
                offsets_deusexmover.FragmentTexture = cls->findProperty(Name("FragmentTexture"))->offset;
                offsets_deusexmover.bFragmentTranslucent = cls->findProperty(Name("bFragmentTranslucent"))->offset;
                offsets_deusexmover.bFragmentUnlit = cls->findProperty(Name("bFragmentUnlit"))->offset;
                offsets_deusexmover.ExplodeSound1 = cls->findProperty(Name("ExplodeSound1"))->offset;
                offsets_deusexmover.ExplodeSound2 = cls->findProperty(Name("ExplodeSound2"))->offset;
                offsets_deusexmover.bDrawExplosion = cls->findProperty(Name("bDrawExplosion"))->offset;
                offsets_deusexmover.bIsDoor = cls->findProperty(Name("bIsDoor"))->offset;
                if (UProperty* property = cls->findProperty(Name("TimeSinceReset"))) {
                    offsets_deusexmover.TimeSinceReset = property->offset;
                }
                if (UProperty* property = cls->findProperty(Name("TimeToReset"))) {
                    offsets_deusexmover.TimeToReset = property->offset;
                }
                offsets_deusexmover.msgKeyLocked = cls->findProperty(Name("msgKeyLocked"))->offset;
                offsets_deusexmover.msgKeyUnlocked = cls->findProperty(Name("msgKeyUnlocked"))->offset;
                offsets_deusexmover.msgLockpickSuccess = cls->findProperty(Name("msgLockpickSuccess"))->offset;
                offsets_deusexmover.msgOneWayFail = cls->findProperty(Name("msgOneWayFail"))->offset;
                offsets_deusexmover.msgLocked = cls->findProperty(Name("msgLocked"))->offset;
                offsets_deusexmover.msgPicking = cls->findProperty(Name("msgPicking"))->offset;
                offsets_deusexmover.msgAlreadyUnlocked = cls->findProperty(Name("msgAlreadyUnlocked"))->offset;
                offsets_deusexmover.msgNoNanoKey = cls->findProperty(Name("msgNoNanoKey"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("DeusExText.DeusExTextParser");
            if (!cls) {
                std::fprintf(stderr, "missing class DeusExText.DeusExTextParser\n");
            } else {
                offsets_deusextextparser.Text = cls->findProperty(Name("Text"))->offset;
                offsets_deusextextparser.textPos = cls->findProperty(Name("textPos"))->offset;
                offsets_deusextextparser.tagEndPos = cls->findProperty(Name("tagEndPos"))->offset;
                offsets_deusextextparser.lastText = cls->findProperty(Name("lastText"))->offset;
                offsets_deusextextparser.lastTag = cls->findProperty(Name("lastTag"))->offset;
                offsets_deusextextparser.lastName = cls->findProperty(Name("lastName"))->offset;
                offsets_deusextextparser.lastColor = cls->findProperty(Name("lastColor"))->offset;
                offsets_deusextextparser.DefaultColor = cls->findProperty(Name("DefaultColor"))->offset;
                offsets_deusextextparser.bParagraphStarted = cls->findProperty(Name("bParagraphStarted"))->offset;
                offsets_deusextextparser.PlayerName = cls->findProperty(Name("PlayerName"))->offset;
                offsets_deusextextparser.playerFirstName = cls->findProperty(Name("playerFirstName"))->offset;
                offsets_deusextextparser.lastEmailName = cls->findProperty(Name("lastEmailName"))->offset;
                offsets_deusextextparser.lastEmailSubject = cls->findProperty(Name("lastEmailSubject"))->offset;
                offsets_deusextextparser.lastEmailFrom = cls->findProperty(Name("lastEmailFrom"))->offset;
                offsets_deusextextparser.lastEmailTo = cls->findProperty(Name("lastEmailTo"))->offset;
                offsets_deusextextparser.lastEmailCC = cls->findProperty(Name("lastEmailCC"))->offset;
                offsets_deusextextparser.lastFileName = cls->findProperty(Name("lastFileName"))->offset;
                offsets_deusextextparser.lastFileDescription = cls->findProperty(Name("lastFileDescription"))->offset;
            }
        }
    }
}
