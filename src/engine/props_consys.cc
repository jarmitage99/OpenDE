#include "engine/props_consys.h"

#include "core/objects.h"

#include <cstdio>

namespace dx {
    conobject::Offsets offsets_conobject;
    conevent::Offsets offsets_conevent;
    coneventspeech::Offsets offsets_coneventspeech;
    concamera::Offsets offsets_concamera;
    conflagref::Offsets offsets_conflagref;
    conversation::Offsets offsets_conversation;
    coneventjump::Offsets offsets_coneventjump;
    conchoice::Offsets offsets_conchoice;
    conspeech::Offsets offsets_conspeech;
    conitem::Offsets offsets_conitem;
    conlistitem::Offsets offsets_conlistitem;
    conhistory::Offsets offsets_conhistory;
    conhistoryevent::Offsets offsets_conhistoryevent;
    conversationlist::Offsets offsets_conversationlist;
    conversationmissionlist::Offsets offsets_conversationmissionlist;
    conaudiolist::Offsets offsets_conaudiolist;
    coneventchoice::Offsets offsets_coneventchoice;
    coneventrandomlabel::Offsets offsets_coneventrandomlabel;
    coneventsetflag::Offsets offsets_coneventsetflag;
    coneventcheckflag::Offsets offsets_coneventcheckflag;
    coneventanimation::Offsets offsets_coneventanimation;
    coneventend::Offsets offsets_coneventend;
    coneventtrigger::Offsets offsets_coneventtrigger;
    coneventtransferobject::Offsets offsets_coneventtransferobject;
    coneventcheckobject::Offsets offsets_coneventcheckobject;
    coneventcheckpersona::Offsets offsets_coneventcheckpersona;
    coneventaddgoal::Offsets offsets_coneventaddgoal;
    coneventaddnote::Offsets offsets_coneventaddnote;
    coneventaddskillpoints::Offsets offsets_coneventaddskillpoints;
    coneventaddcredits::Offsets offsets_coneventaddcredits;
    coneventtrade::Offsets offsets_coneventtrade;
    coneventmovecamera::Offsets offsets_coneventmovecamera;
    coneventcomment::Offsets offsets_coneventcomment;
    concamerawindow::Offsets offsets_concamerawindow;

    void bindProps_consys()
    {
        {
            UClass* cls = objects::findClass("ConSys.ConObject");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConObject\n");
            } else {
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConEvent");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConEvent\n");
            } else {
                offsets_conevent.eventType = cls->findProperty(Name("eventType"))->offset;
                offsets_conevent.Label = cls->findProperty(Name("Label"))->offset;
                offsets_conevent.nextEvent = cls->findProperty(Name("nextEvent"))->offset;
                offsets_conevent.Conversation = cls->findProperty(Name("Conversation"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConEventSpeech");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConEventSpeech\n");
            } else {
                offsets_coneventspeech.speaker = cls->findProperty(Name("speaker"))->offset;
                offsets_coneventspeech.speakerName = cls->findProperty(Name("speakerName"))->offset;
                offsets_coneventspeech.speakingTo = cls->findProperty(Name("speakingTo"))->offset;
                offsets_coneventspeech.speakingToName = cls->findProperty(Name("speakingToName"))->offset;
                offsets_coneventspeech.ConSpeech = cls->findProperty(Name("ConSpeech"))->offset;
                offsets_coneventspeech.bContinued = cls->findProperty(Name("bContinued"))->offset;
                offsets_coneventspeech.bBold = cls->findProperty(Name("bBold"))->offset;
                offsets_coneventspeech.speechFont = cls->findProperty(Name("speechFont"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConCamera");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConCamera\n");
            } else {
                offsets_concamera.cameraActor = cls->findProperty(Name("cameraActor"))->offset;
                offsets_concamera.cameraPosition = cls->findProperty(Name("cameraPosition"))->offset;
                offsets_concamera.cameraType = cls->findProperty(Name("cameraType"))->offset;
                offsets_concamera.cameraMode = cls->findProperty(Name("cameraMode"))->offset;
                offsets_concamera.conLightSpeaker = cls->findProperty(Name("conLightSpeaker"))->offset;
                offsets_concamera.conLightSpeakingTo = cls->findProperty(Name("conLightSpeakingTo"))->offset;
                offsets_concamera.cameraOffset = cls->findProperty(Name("cameraOffset"))->offset;
                offsets_concamera.Rotation = cls->findProperty(Name("Rotation"))->offset;
                offsets_concamera.cosAngle = cls->findProperty(Name("cosAngle"))->offset;
                offsets_concamera.firstActorRotation = cls->findProperty(Name("firstActorRotation"))->offset;
                offsets_concamera.setActorCount = cls->findProperty(Name("setActorCount"))->offset;
                offsets_concamera.bCameraLocationSaved = cls->findProperty(Name("bCameraLocationSaved"))->offset;
                offsets_concamera.cameraFallbackPositions = cls->findProperty(Name("cameraFallbackPositions"))->offset;
                offsets_concamera.cameraHeightPositions = cls->findProperty(Name("cameraHeightPositions"))->offset;
                offsets_concamera.currentFallback = cls->findProperty(Name("currentFallback"))->offset;
                offsets_concamera.bUsingFallback = cls->findProperty(Name("bUsingFallback"))->offset;
                offsets_concamera.heightModifier = cls->findProperty(Name("heightModifier"))->offset;
                offsets_concamera.centerModifier = cls->findProperty(Name("centerModifier"))->offset;
                offsets_concamera.distanceMultiplier = cls->findProperty(Name("distanceMultiplier"))->offset;
                offsets_concamera.heightFallbackTrigger = cls->findProperty(Name("heightFallbackTrigger"))->offset;
                offsets_concamera.firstActor = cls->findProperty(Name("firstActor"))->offset;
                offsets_concamera.secondActor = cls->findProperty(Name("secondActor"))->offset;
                offsets_concamera.lastFirstActor = cls->findProperty(Name("lastFirstActor"))->offset;
                offsets_concamera.lastSecondActor = cls->findProperty(Name("lastSecondActor"))->offset;
                offsets_concamera.ignoreSetActors = cls->findProperty(Name("ignoreSetActors"))->offset;
                offsets_concamera.bDebug = cls->findProperty(Name("bDebug"))->offset;
                offsets_concamera.lastLocation = cls->findProperty(Name("lastLocation"))->offset;
                offsets_concamera.lastRotation = cls->findProperty(Name("lastRotation"))->offset;
                offsets_concamera.bInteractiveCamera = cls->findProperty(Name("bInteractiveCamera"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConFlagRef");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConFlagRef\n");
            } else {
                offsets_conflagref.FlagName = cls->findProperty(Name("FlagName"))->offset;
                offsets_conflagref.Value = cls->findProperty(Name("Value"))->offset;
                offsets_conflagref.expiration = cls->findProperty(Name("expiration"))->offset;
                offsets_conflagref.nextFlagRef = cls->findProperty(Name("nextFlagRef"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.Conversation");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.Conversation\n");
            } else {
                offsets_conversation.conName = cls->findProperty(Name("conName"))->offset;
                offsets_conversation.Description = cls->findProperty(Name("Description"))->offset;
                offsets_conversation.CreatedBy = cls->findProperty(Name("CreatedBy"))->offset;
                offsets_conversation.conOwnerName = cls->findProperty(Name("conOwnerName"))->offset;
                offsets_conversation.bDataLinkCon = cls->findProperty(Name("bDataLinkCon"))->offset;
                offsets_conversation.bDisplayOnce = cls->findProperty(Name("bDisplayOnce"))->offset;
                offsets_conversation.bFirstPerson = cls->findProperty(Name("bFirstPerson"))->offset;
                offsets_conversation.bNonInteractive = cls->findProperty(Name("bNonInteractive"))->offset;
                offsets_conversation.bRandomCamera = cls->findProperty(Name("bRandomCamera"))->offset;
                offsets_conversation.bCanBeInterrupted = cls->findProperty(Name("bCanBeInterrupted"))->offset;
                offsets_conversation.bCannotBeInterrupted = cls->findProperty(Name("bCannotBeInterrupted"))->offset;
                offsets_conversation.bGenerateAudioNames = cls->findProperty(Name("bGenerateAudioNames"))->offset;
                offsets_conversation.bInvokeBump = cls->findProperty(Name("bInvokeBump"))->offset;
                offsets_conversation.bInvokeFrob = cls->findProperty(Name("bInvokeFrob"))->offset;
                offsets_conversation.bInvokeSight = cls->findProperty(Name("bInvokeSight"))->offset;
                offsets_conversation.bInvokeRadius = cls->findProperty(Name("bInvokeRadius"))->offset;
                offsets_conversation.radiusDistance = cls->findProperty(Name("radiusDistance"))->offset;
                offsets_conversation.eventList = cls->findProperty(Name("eventList"))->offset;
                offsets_conversation.flagRefList = cls->findProperty(Name("flagRefList"))->offset;
                offsets_conversation.conversationID = cls->findProperty(Name("conversationID"))->offset;
                offsets_conversation.audioPackageName = cls->findProperty(Name("audioPackageName"))->offset;
                offsets_conversation.lastPlayedTime = cls->findProperty(Name("lastPlayedTime"))->offset;
                offsets_conversation.ownerRefCount = cls->findProperty(Name("ownerRefCount"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConEventJump");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConEventJump\n");
            } else {
                offsets_coneventjump.jumpLabel = cls->findProperty(Name("jumpLabel"))->offset;
                offsets_coneventjump.jumpCon = cls->findProperty(Name("jumpCon"))->offset;
                offsets_coneventjump.conID = cls->findProperty(Name("conID"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConChoice");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConChoice\n");
            } else {
                offsets_conchoice.choiceText = cls->findProperty(Name("choiceText"))->offset;
                offsets_conchoice.choiceLabel = cls->findProperty(Name("choiceLabel"))->offset;
                offsets_conchoice.bDisplayAsSpeech = cls->findProperty(Name("bDisplayAsSpeech"))->offset;
                offsets_conchoice.soundID = cls->findProperty(Name("soundID"))->offset;
                offsets_conchoice.skillNeeded = cls->findProperty(Name("skillNeeded"))->offset;
                offsets_conchoice.skillLevelNeeded = cls->findProperty(Name("skillLevelNeeded"))->offset;
                offsets_conchoice.flagRef = cls->findProperty(Name("flagRef"))->offset;
                offsets_conchoice.nextChoice = cls->findProperty(Name("nextChoice"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConSpeech");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConSpeech\n");
            } else {
                offsets_conspeech.Speech = cls->findProperty(Name("Speech"))->offset;
                offsets_conspeech.soundID = cls->findProperty(Name("soundID"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConItem");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConItem\n");
            } else {
                offsets_conitem.ConObject = cls->findProperty(Name("ConObject"))->offset;
                offsets_conitem.Next = cls->findProperty(Name("Next"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConListItem");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConListItem\n");
            } else {
                offsets_conlistitem.con = cls->findProperty(Name("con"))->offset;
                offsets_conlistitem.Next = cls->findProperty(Name("Next"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConHistory");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConHistory\n");
            } else {
                offsets_conhistory.conOwnerName = cls->findProperty(Name("conOwnerName"))->offset;
                offsets_conhistory.strLocation = cls->findProperty(Name("strLocation"))->offset;
                offsets_conhistory.strDescription = cls->findProperty(Name("strDescription"))->offset;
                offsets_conhistory.bInfoLink = cls->findProperty(Name("bInfoLink"))->offset;
                offsets_conhistory.firstEvent = cls->findProperty(Name("firstEvent"))->offset;
                offsets_conhistory.lastEvent = cls->findProperty(Name("lastEvent"))->offset;
                offsets_conhistory.Next = cls->findProperty(Name("Next"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConHistoryEvent");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConHistoryEvent\n");
            } else {
                offsets_conhistoryevent.conSpeaker = cls->findProperty(Name("conSpeaker"))->offset;
                offsets_conhistoryevent.Speech = cls->findProperty(Name("Speech"))->offset;
                offsets_conhistoryevent.soundID = cls->findProperty(Name("soundID"))->offset;
                offsets_conhistoryevent.Next = cls->findProperty(Name("Next"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConversationList");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConversationList\n");
            } else {
                offsets_conversationlist.conversations = cls->findProperty(Name("conversations"))->offset;
                offsets_conversationlist.missionDescription = cls->findProperty(Name("missionDescription"))->offset;
                offsets_conversationlist.missionNumber = cls->findProperty(Name("missionNumber"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConversationMissionList");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConversationMissionList\n");
            } else {
                offsets_conversationmissionlist.missions = cls->findProperty(Name("missions"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConAudioList");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConAudioList\n");
            } else {
                offsets_conaudiolist.ConAudioList = cls->findProperty(Name("ConAudioList"))->offset;
                offsets_conaudiolist.audioCount = cls->findProperty(Name("audioCount"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConEventChoice");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConEventChoice\n");
            } else {
                offsets_coneventchoice.bClearScreen = cls->findProperty(Name("bClearScreen"))->offset;
                offsets_coneventchoice.ChoiceList = cls->findProperty(Name("ChoiceList"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConEventRandomLabel");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConEventRandomLabel\n");
            } else {
                offsets_coneventrandomlabel.labels = cls->findProperty(Name("labels"))->offset;
                offsets_coneventrandomlabel.bCycleEvents = cls->findProperty(Name("bCycleEvents"))->offset;
                offsets_coneventrandomlabel.bCycleOnce = cls->findProperty(Name("bCycleOnce"))->offset;
                offsets_coneventrandomlabel.bCycleRandom = cls->findProperty(Name("bCycleRandom"))->offset;
                offsets_coneventrandomlabel.cycleIndex = cls->findProperty(Name("cycleIndex"))->offset;
                offsets_coneventrandomlabel.bLabelsCycled = cls->findProperty(Name("bLabelsCycled"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConEventSetFlag");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConEventSetFlag\n");
            } else {
                offsets_coneventsetflag.flagRef = cls->findProperty(Name("flagRef"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConEventCheckFlag");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConEventCheckFlag\n");
            } else {
                offsets_coneventcheckflag.flagRef = cls->findProperty(Name("flagRef"))->offset;
                offsets_coneventcheckflag.setLabel = cls->findProperty(Name("setLabel"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConEventAnimation");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConEventAnimation\n");
            } else {
                offsets_coneventanimation.eventOwner = cls->findProperty(Name("eventOwner"))->offset;
                offsets_coneventanimation.eventOwnerName = cls->findProperty(Name("eventOwnerName"))->offset;
                offsets_coneventanimation.Sequence = cls->findProperty(Name("Sequence"))->offset;
                offsets_coneventanimation.bFinishAnim = cls->findProperty(Name("bFinishAnim"))->offset;
                offsets_coneventanimation.bLoopAnim = cls->findProperty(Name("bLoopAnim"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConEventEnd");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConEventEnd\n");
            } else {
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConEventTrigger");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConEventTrigger\n");
            } else {
                offsets_coneventtrigger.triggerTag = cls->findProperty(Name("triggerTag"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConEventTransferObject");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConEventTransferObject\n");
            } else {
                offsets_coneventtransferobject.ObjectName = cls->findProperty(Name("ObjectName"))->offset;
                offsets_coneventtransferobject.giveObject = cls->findProperty(Name("giveObject"))->offset;
                offsets_coneventtransferobject.transferCount = cls->findProperty(Name("transferCount"))->offset;
                offsets_coneventtransferobject.fromActor = cls->findProperty(Name("fromActor"))->offset;
                offsets_coneventtransferobject.fromName = cls->findProperty(Name("fromName"))->offset;
                offsets_coneventtransferobject.toActor = cls->findProperty(Name("toActor"))->offset;
                offsets_coneventtransferobject.toName = cls->findProperty(Name("toName"))->offset;
                offsets_coneventtransferobject.failLabel = cls->findProperty(Name("failLabel"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConEventCheckObject");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConEventCheckObject\n");
            } else {
                offsets_coneventcheckobject.ObjectName = cls->findProperty(Name("ObjectName"))->offset;
                offsets_coneventcheckobject.checkObject = cls->findProperty(Name("checkObject"))->offset;
                offsets_coneventcheckobject.failLabel = cls->findProperty(Name("failLabel"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConEventCheckPersona");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConEventCheckPersona\n");
            } else {
                offsets_coneventcheckpersona.personaType = cls->findProperty(Name("personaType"))->offset;
                offsets_coneventcheckpersona.condition = cls->findProperty(Name("condition"))->offset;
                offsets_coneventcheckpersona.Value = cls->findProperty(Name("Value"))->offset;
                offsets_coneventcheckpersona.jumpLabel = cls->findProperty(Name("jumpLabel"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConEventAddGoal");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConEventAddGoal\n");
            } else {
                offsets_coneventaddgoal.goalName = cls->findProperty(Name("goalName"))->offset;
                offsets_coneventaddgoal.bGoalCompleted = cls->findProperty(Name("bGoalCompleted"))->offset;
                offsets_coneventaddgoal.goalText = cls->findProperty(Name("goalText"))->offset;
                offsets_coneventaddgoal.bPrimaryGoal = cls->findProperty(Name("bPrimaryGoal"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConEventAddNote");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConEventAddNote\n");
            } else {
                offsets_coneventaddnote.noteText = cls->findProperty(Name("noteText"))->offset;
                offsets_coneventaddnote.bNoteAdded = cls->findProperty(Name("bNoteAdded"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConEventAddSkillPoints");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConEventAddSkillPoints\n");
            } else {
                offsets_coneventaddskillpoints.pointsToAdd = cls->findProperty(Name("pointsToAdd"))->offset;
                offsets_coneventaddskillpoints.awardMessage = cls->findProperty(Name("awardMessage"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConEventAddCredits");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConEventAddCredits\n");
            } else {
                offsets_coneventaddcredits.creditsToAdd = cls->findProperty(Name("creditsToAdd"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConEventTrade");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConEventTrade\n");
            } else {
                offsets_coneventtrade.eventOwner = cls->findProperty(Name("eventOwner"))->offset;
                offsets_coneventtrade.eventOwnerName = cls->findProperty(Name("eventOwnerName"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConEventMoveCamera");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConEventMoveCamera\n");
            } else {
                offsets_coneventmovecamera.cameraType = cls->findProperty(Name("cameraType"))->offset;
                offsets_coneventmovecamera.cameraPosition = cls->findProperty(Name("cameraPosition"))->offset;
                offsets_coneventmovecamera.cameraTransition = cls->findProperty(Name("cameraTransition"))->offset;
                offsets_coneventmovecamera.cameraOffset = cls->findProperty(Name("cameraOffset"))->offset;
                offsets_coneventmovecamera.Rotation = cls->findProperty(Name("Rotation"))->offset;
                offsets_coneventmovecamera.heightModifier = cls->findProperty(Name("heightModifier"))->offset;
                offsets_coneventmovecamera.centerModifier = cls->findProperty(Name("centerModifier"))->offset;
                offsets_coneventmovecamera.distanceMultiplier = cls->findProperty(Name("distanceMultiplier"))->offset;
                offsets_coneventmovecamera.cameraActor = cls->findProperty(Name("cameraActor"))->offset;
                offsets_coneventmovecamera.cameraActorName = cls->findProperty(Name("cameraActorName"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConEventComment");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConEventComment\n");
            } else {
                offsets_coneventcomment.commentText = cls->findProperty(Name("commentText"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("ConSys.ConCameraWindow");
            if (!cls) {
                std::fprintf(stderr, "missing class ConSys.ConCameraWindow\n");
            } else {
                offsets_concamerawindow.Camera = cls->findProperty(Name("Camera"))->offset;
            }
        }
    }
}
