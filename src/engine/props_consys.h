#pragma once

#include "core/math.h"
#include "core/object.h"

#include <string>

namespace dx {
    namespace conobject {
        struct Offsets {
        };
    }

    extern conobject::Offsets offsets_conobject;

    namespace conobject {

    }

    namespace conevent {
        struct Offsets {
            size_t eventType = 0;
            size_t Label = 0;
            size_t nextEvent = 0;
            size_t Conversation = 0;
        };
    }

    extern conevent::Offsets offsets_conevent;

    namespace conevent {
        inline uint8_t& eventType(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_conevent.eventType);
        }

        inline std::string& Label(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_conevent.Label);
        }

        inline UObject*& nextEvent(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_conevent.nextEvent);
        }

        inline UObject*& Conversation(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_conevent.Conversation);
        }

    }

    namespace coneventspeech {
        struct Offsets {
            size_t speaker = 0;
            size_t speakerName = 0;
            size_t speakingTo = 0;
            size_t speakingToName = 0;
            size_t ConSpeech = 0;
            size_t bContinued = 0;
            size_t bBold = 0;
            size_t speechFont = 0;
        };
    }

    extern coneventspeech::Offsets offsets_coneventspeech;

    namespace coneventspeech {
        inline UObject*& speaker(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_coneventspeech.speaker);
        }

        inline std::string& speakerName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_coneventspeech.speakerName);
        }

        inline UObject*& speakingTo(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_coneventspeech.speakingTo);
        }

        inline std::string& speakingToName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_coneventspeech.speakingToName);
        }

        inline UObject*& ConSpeech(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_coneventspeech.ConSpeech);
        }

        inline uint32_t& bContinued(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_coneventspeech.bContinued);
        }

        inline uint32_t& bBold(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_coneventspeech.bBold);
        }

        inline uint8_t& speechFont(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_coneventspeech.speechFont);
        }

    }

    namespace concamera {
        struct Offsets {
            size_t cameraActor = 0;
            size_t cameraPosition = 0;
            size_t cameraType = 0;
            size_t cameraMode = 0;
            size_t conLightSpeaker = 0;
            size_t conLightSpeakingTo = 0;
            size_t cameraOffset = 0;
            size_t Rotation = 0;
            size_t cosAngle = 0;
            size_t firstActorRotation = 0;
            size_t setActorCount = 0;
            size_t bCameraLocationSaved = 0;
            size_t cameraFallbackPositions = 0;
            size_t cameraHeightPositions = 0;
            size_t currentFallback = 0;
            size_t bUsingFallback = 0;
            size_t heightModifier = 0;
            size_t centerModifier = 0;
            size_t distanceMultiplier = 0;
            size_t heightFallbackTrigger = 0;
            size_t firstActor = 0;
            size_t secondActor = 0;
            size_t lastFirstActor = 0;
            size_t lastSecondActor = 0;
            size_t ignoreSetActors = 0;
            size_t bDebug = 0;
            size_t lastLocation = 0;
            size_t lastRotation = 0;
            size_t bInteractiveCamera = 0;
        };
    }

    extern concamera::Offsets offsets_concamera;

    namespace concamera {
        inline UObject*& cameraActor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_concamera.cameraActor);
        }

        inline uint8_t& cameraPosition(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_concamera.cameraPosition);
        }

        inline uint8_t& cameraType(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_concamera.cameraType);
        }

        inline uint8_t& cameraMode(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_concamera.cameraMode);
        }

        inline UObject*& conLightSpeaker(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_concamera.conLightSpeaker);
        }

        inline UObject*& conLightSpeakingTo(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_concamera.conLightSpeakingTo);
        }

        inline Vector& cameraOffset(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_concamera.cameraOffset);
        }

        inline Rotator& Rotation(UObject* o)
        {
            return *reinterpret_cast<Rotator*>(o->data + offsets_concamera.Rotation);
        }

        inline float& cosAngle(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_concamera.cosAngle);
        }

        inline int32_t& firstActorRotation(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_concamera.firstActorRotation);
        }

        inline int32_t& setActorCount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_concamera.setActorCount);
        }

        inline uint32_t& bCameraLocationSaved(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_concamera.bCameraLocationSaved);
        }

        inline uint8_t& cameraFallbackPositions(UObject* o, int index)
        {
            return reinterpret_cast<uint8_t*>(o->data + offsets_concamera.cameraFallbackPositions)[index];
        }

        inline uint8_t& cameraHeightPositions(UObject* o, int index)
        {
            return reinterpret_cast<uint8_t*>(o->data + offsets_concamera.cameraHeightPositions)[index];
        }

        inline int32_t& currentFallback(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_concamera.currentFallback);
        }

        inline uint32_t& bUsingFallback(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_concamera.bUsingFallback);
        }

        inline float& heightModifier(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_concamera.heightModifier);
        }

        inline float& centerModifier(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_concamera.centerModifier);
        }

        inline float& distanceMultiplier(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_concamera.distanceMultiplier);
        }

        inline float& heightFallbackTrigger(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_concamera.heightFallbackTrigger);
        }

        inline UObject*& firstActor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_concamera.firstActor);
        }

        inline UObject*& secondActor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_concamera.secondActor);
        }

        inline UObject*& lastFirstActor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_concamera.lastFirstActor);
        }

        inline UObject*& lastSecondActor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_concamera.lastSecondActor);
        }

        inline uint32_t& ignoreSetActors(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_concamera.ignoreSetActors);
        }

        inline uint32_t& bDebug(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_concamera.bDebug);
        }

        inline Vector& lastLocation(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_concamera.lastLocation);
        }

        inline Rotator& lastRotation(UObject* o)
        {
            return *reinterpret_cast<Rotator*>(o->data + offsets_concamera.lastRotation);
        }

        inline uint32_t& bInteractiveCamera(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_concamera.bInteractiveCamera);
        }

    }

    namespace conflagref {
        struct Offsets {
            size_t FlagName = 0;
            size_t Value = 0;
            size_t expiration = 0;
            size_t nextFlagRef = 0;
        };
    }

    extern conflagref::Offsets offsets_conflagref;

    namespace conflagref {
        inline Name& FlagName(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_conflagref.FlagName);
        }

        inline uint32_t& Value(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_conflagref.Value);
        }

        inline int32_t& expiration(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_conflagref.expiration);
        }

        inline UObject*& nextFlagRef(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_conflagref.nextFlagRef);
        }

    }

    namespace conversation {
        struct Offsets {
            size_t conName = 0;
            size_t Description = 0;
            size_t CreatedBy = 0;
            size_t conOwnerName = 0;
            size_t bDataLinkCon = 0;
            size_t bDisplayOnce = 0;
            size_t bFirstPerson = 0;
            size_t bNonInteractive = 0;
            size_t bRandomCamera = 0;
            size_t bCanBeInterrupted = 0;
            size_t bCannotBeInterrupted = 0;
            size_t bGenerateAudioNames = 0;
            size_t bInvokeBump = 0;
            size_t bInvokeFrob = 0;
            size_t bInvokeSight = 0;
            size_t bInvokeRadius = 0;
            size_t radiusDistance = 0;
            size_t eventList = 0;
            size_t flagRefList = 0;
            size_t conversationID = 0;
            size_t audioPackageName = 0;
            size_t lastPlayedTime = 0;
            size_t ownerRefCount = 0;
        };
    }

    extern conversation::Offsets offsets_conversation;

    namespace conversation {
        inline Name& conName(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_conversation.conName);
        }

        inline std::string& Description(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_conversation.Description);
        }

        inline std::string& CreatedBy(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_conversation.CreatedBy);
        }

        inline std::string& conOwnerName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_conversation.conOwnerName);
        }

        inline uint32_t& bDataLinkCon(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_conversation.bDataLinkCon);
        }

        inline uint32_t& bDisplayOnce(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_conversation.bDisplayOnce);
        }

        inline uint32_t& bFirstPerson(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_conversation.bFirstPerson);
        }

        inline uint32_t& bNonInteractive(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_conversation.bNonInteractive);
        }

        inline uint32_t& bRandomCamera(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_conversation.bRandomCamera);
        }

        inline uint32_t& bCanBeInterrupted(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_conversation.bCanBeInterrupted);
        }

        inline uint32_t& bCannotBeInterrupted(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_conversation.bCannotBeInterrupted);
        }

        inline uint32_t& bGenerateAudioNames(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_conversation.bGenerateAudioNames);
        }

        inline uint32_t& bInvokeBump(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_conversation.bInvokeBump);
        }

        inline uint32_t& bInvokeFrob(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_conversation.bInvokeFrob);
        }

        inline uint32_t& bInvokeSight(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_conversation.bInvokeSight);
        }

        inline uint32_t& bInvokeRadius(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_conversation.bInvokeRadius);
        }

        inline int32_t& radiusDistance(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_conversation.radiusDistance);
        }

        inline UObject*& eventList(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_conversation.eventList);
        }

        inline UObject*& flagRefList(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_conversation.flagRefList);
        }

        inline int32_t& conversationID(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_conversation.conversationID);
        }

        inline std::string& audioPackageName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_conversation.audioPackageName);
        }

        inline float& lastPlayedTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_conversation.lastPlayedTime);
        }

        inline int32_t& ownerRefCount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_conversation.ownerRefCount);
        }

    }

    namespace coneventjump {
        struct Offsets {
            size_t jumpLabel = 0;
            size_t jumpCon = 0;
            size_t conID = 0;
        };
    }

    extern coneventjump::Offsets offsets_coneventjump;

    namespace coneventjump {
        inline std::string& jumpLabel(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_coneventjump.jumpLabel);
        }

        inline UObject*& jumpCon(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_coneventjump.jumpCon);
        }

        inline int32_t& conID(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_coneventjump.conID);
        }

    }

    namespace conchoice {
        struct Offsets {
            size_t choiceText = 0;
            size_t choiceLabel = 0;
            size_t bDisplayAsSpeech = 0;
            size_t soundID = 0;
            size_t skillNeeded = 0;
            size_t skillLevelNeeded = 0;
            size_t flagRef = 0;
            size_t nextChoice = 0;
        };
    }

    extern conchoice::Offsets offsets_conchoice;

    namespace conchoice {
        inline std::string& choiceText(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_conchoice.choiceText);
        }

        inline std::string& choiceLabel(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_conchoice.choiceLabel);
        }

        inline uint32_t& bDisplayAsSpeech(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_conchoice.bDisplayAsSpeech);
        }

        inline int32_t& soundID(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_conchoice.soundID);
        }

        inline UObject*& skillNeeded(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_conchoice.skillNeeded);
        }

        inline int32_t& skillLevelNeeded(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_conchoice.skillLevelNeeded);
        }

        inline UObject*& flagRef(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_conchoice.flagRef);
        }

        inline UObject*& nextChoice(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_conchoice.nextChoice);
        }

    }

    namespace conspeech {
        struct Offsets {
            size_t Speech = 0;
            size_t soundID = 0;
        };
    }

    extern conspeech::Offsets offsets_conspeech;

    namespace conspeech {
        inline std::string& Speech(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_conspeech.Speech);
        }

        inline int32_t& soundID(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_conspeech.soundID);
        }

    }

    namespace conitem {
        struct Offsets {
            size_t ConObject = 0;
            size_t Next = 0;
        };
    }

    extern conitem::Offsets offsets_conitem;

    namespace conitem {
        inline UObject*& ConObject(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_conitem.ConObject);
        }

        inline UObject*& Next(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_conitem.Next);
        }

    }

    namespace conlistitem {
        struct Offsets {
            size_t con = 0;
            size_t Next = 0;
        };
    }

    extern conlistitem::Offsets offsets_conlistitem;

    namespace conlistitem {
        inline UObject*& con(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_conlistitem.con);
        }

        inline UObject*& Next(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_conlistitem.Next);
        }

    }

    namespace conhistory {
        struct Offsets {
            size_t conOwnerName = 0;
            size_t strLocation = 0;
            size_t strDescription = 0;
            size_t bInfoLink = 0;
            size_t firstEvent = 0;
            size_t lastEvent = 0;
            size_t Next = 0;
        };
    }

    extern conhistory::Offsets offsets_conhistory;

    namespace conhistory {
        inline std::string& conOwnerName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_conhistory.conOwnerName);
        }

        inline std::string& strLocation(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_conhistory.strLocation);
        }

        inline std::string& strDescription(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_conhistory.strDescription);
        }

        inline uint32_t& bInfoLink(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_conhistory.bInfoLink);
        }

        inline UObject*& firstEvent(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_conhistory.firstEvent);
        }

        inline UObject*& lastEvent(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_conhistory.lastEvent);
        }

        inline UObject*& Next(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_conhistory.Next);
        }

    }

    namespace conhistoryevent {
        struct Offsets {
            size_t conSpeaker = 0;
            size_t Speech = 0;
            size_t soundID = 0;
            size_t Next = 0;
        };
    }

    extern conhistoryevent::Offsets offsets_conhistoryevent;

    namespace conhistoryevent {
        inline std::string& conSpeaker(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_conhistoryevent.conSpeaker);
        }

        inline std::string& Speech(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_conhistoryevent.Speech);
        }

        inline int32_t& soundID(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_conhistoryevent.soundID);
        }

        inline UObject*& Next(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_conhistoryevent.Next);
        }

    }

    namespace conversationlist {
        struct Offsets {
            size_t conversations = 0;
            size_t missionDescription = 0;
            size_t missionNumber = 0;
        };
    }

    extern conversationlist::Offsets offsets_conversationlist;

    namespace conversationlist {
        inline UObject*& conversations(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_conversationlist.conversations);
        }

        inline std::string& missionDescription(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_conversationlist.missionDescription);
        }

        inline int32_t& missionNumber(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_conversationlist.missionNumber);
        }

    }

    namespace conversationmissionlist {
        struct Offsets {
            size_t missions = 0;
        };
    }

    extern conversationmissionlist::Offsets offsets_conversationmissionlist;

    namespace conversationmissionlist {
        inline UObject*& missions(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_conversationmissionlist.missions);
        }

    }

    namespace conaudiolist {
        struct Offsets {
            size_t ConAudioList = 0;
            size_t audioCount = 0;
        };
    }

    extern conaudiolist::Offsets offsets_conaudiolist;

    namespace conaudiolist {
        inline uint8_t* ConAudioList(UObject* o)
        {
            return o->data + offsets_conaudiolist.ConAudioList;
        }

        inline int32_t& audioCount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_conaudiolist.audioCount);
        }

    }

    namespace coneventchoice {
        struct Offsets {
            size_t bClearScreen = 0;
            size_t ChoiceList = 0;
        };
    }

    extern coneventchoice::Offsets offsets_coneventchoice;

    namespace coneventchoice {
        inline uint32_t& bClearScreen(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_coneventchoice.bClearScreen);
        }

        inline UObject*& ChoiceList(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_coneventchoice.ChoiceList);
        }

    }

    namespace coneventrandomlabel {
        struct Offsets {
            size_t labels = 0;
            size_t bCycleEvents = 0;
            size_t bCycleOnce = 0;
            size_t bCycleRandom = 0;
            size_t cycleIndex = 0;
            size_t bLabelsCycled = 0;
        };
    }

    extern coneventrandomlabel::Offsets offsets_coneventrandomlabel;

    namespace coneventrandomlabel {
        inline uint8_t* labels(UObject* o)
        {
            return o->data + offsets_coneventrandomlabel.labels;
        }

        inline uint32_t& bCycleEvents(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_coneventrandomlabel.bCycleEvents);
        }

        inline uint32_t& bCycleOnce(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_coneventrandomlabel.bCycleOnce);
        }

        inline uint32_t& bCycleRandom(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_coneventrandomlabel.bCycleRandom);
        }

        inline int32_t& cycleIndex(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_coneventrandomlabel.cycleIndex);
        }

        inline uint32_t& bLabelsCycled(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_coneventrandomlabel.bLabelsCycled);
        }

    }

    namespace coneventsetflag {
        struct Offsets {
            size_t flagRef = 0;
        };
    }

    extern coneventsetflag::Offsets offsets_coneventsetflag;

    namespace coneventsetflag {
        inline UObject*& flagRef(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_coneventsetflag.flagRef);
        }

    }

    namespace coneventcheckflag {
        struct Offsets {
            size_t flagRef = 0;
            size_t setLabel = 0;
        };
    }

    extern coneventcheckflag::Offsets offsets_coneventcheckflag;

    namespace coneventcheckflag {
        inline UObject*& flagRef(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_coneventcheckflag.flagRef);
        }

        inline std::string& setLabel(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_coneventcheckflag.setLabel);
        }

    }

    namespace coneventanimation {
        struct Offsets {
            size_t eventOwner = 0;
            size_t eventOwnerName = 0;
            size_t Sequence = 0;
            size_t bFinishAnim = 0;
            size_t bLoopAnim = 0;
        };
    }

    extern coneventanimation::Offsets offsets_coneventanimation;

    namespace coneventanimation {
        inline UObject*& eventOwner(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_coneventanimation.eventOwner);
        }

        inline std::string& eventOwnerName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_coneventanimation.eventOwnerName);
        }

        inline Name& Sequence(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_coneventanimation.Sequence);
        }

        inline uint32_t& bFinishAnim(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_coneventanimation.bFinishAnim);
        }

        inline uint32_t& bLoopAnim(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_coneventanimation.bLoopAnim);
        }

    }

    namespace coneventend {
        struct Offsets {
        };
    }

    extern coneventend::Offsets offsets_coneventend;

    namespace coneventend {

    }

    namespace coneventtrigger {
        struct Offsets {
            size_t triggerTag = 0;
        };
    }

    extern coneventtrigger::Offsets offsets_coneventtrigger;

    namespace coneventtrigger {
        inline Name& triggerTag(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_coneventtrigger.triggerTag);
        }

    }

    namespace coneventtransferobject {
        struct Offsets {
            size_t ObjectName = 0;
            size_t giveObject = 0;
            size_t transferCount = 0;
            size_t fromActor = 0;
            size_t fromName = 0;
            size_t toActor = 0;
            size_t toName = 0;
            size_t failLabel = 0;
        };
    }

    extern coneventtransferobject::Offsets offsets_coneventtransferobject;

    namespace coneventtransferobject {
        inline std::string& ObjectName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_coneventtransferobject.ObjectName);
        }

        inline UObject*& giveObject(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_coneventtransferobject.giveObject);
        }

        inline int32_t& transferCount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_coneventtransferobject.transferCount);
        }

        inline UObject*& fromActor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_coneventtransferobject.fromActor);
        }

        inline std::string& fromName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_coneventtransferobject.fromName);
        }

        inline UObject*& toActor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_coneventtransferobject.toActor);
        }

        inline std::string& toName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_coneventtransferobject.toName);
        }

        inline std::string& failLabel(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_coneventtransferobject.failLabel);
        }

    }

    namespace coneventcheckobject {
        struct Offsets {
            size_t ObjectName = 0;
            size_t checkObject = 0;
            size_t failLabel = 0;
        };
    }

    extern coneventcheckobject::Offsets offsets_coneventcheckobject;

    namespace coneventcheckobject {
        inline std::string& ObjectName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_coneventcheckobject.ObjectName);
        }

        inline UObject*& checkObject(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_coneventcheckobject.checkObject);
        }

        inline std::string& failLabel(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_coneventcheckobject.failLabel);
        }

    }

    namespace coneventcheckpersona {
        struct Offsets {
            size_t personaType = 0;
            size_t condition = 0;
            size_t Value = 0;
            size_t jumpLabel = 0;
        };
    }

    extern coneventcheckpersona::Offsets offsets_coneventcheckpersona;

    namespace coneventcheckpersona {
        inline uint8_t& personaType(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_coneventcheckpersona.personaType);
        }

        inline uint8_t& condition(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_coneventcheckpersona.condition);
        }

        inline int32_t& Value(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_coneventcheckpersona.Value);
        }

        inline std::string& jumpLabel(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_coneventcheckpersona.jumpLabel);
        }

    }

    namespace coneventaddgoal {
        struct Offsets {
            size_t goalName = 0;
            size_t bGoalCompleted = 0;
            size_t goalText = 0;
            size_t bPrimaryGoal = 0;
        };
    }

    extern coneventaddgoal::Offsets offsets_coneventaddgoal;

    namespace coneventaddgoal {
        inline Name& goalName(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_coneventaddgoal.goalName);
        }

        inline uint32_t& bGoalCompleted(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_coneventaddgoal.bGoalCompleted);
        }

        inline std::string& goalText(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_coneventaddgoal.goalText);
        }

        inline uint32_t& bPrimaryGoal(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_coneventaddgoal.bPrimaryGoal);
        }

    }

    namespace coneventaddnote {
        struct Offsets {
            size_t noteText = 0;
            size_t bNoteAdded = 0;
        };
    }

    extern coneventaddnote::Offsets offsets_coneventaddnote;

    namespace coneventaddnote {
        inline std::string& noteText(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_coneventaddnote.noteText);
        }

        inline uint32_t& bNoteAdded(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_coneventaddnote.bNoteAdded);
        }

    }

    namespace coneventaddskillpoints {
        struct Offsets {
            size_t pointsToAdd = 0;
            size_t awardMessage = 0;
        };
    }

    extern coneventaddskillpoints::Offsets offsets_coneventaddskillpoints;

    namespace coneventaddskillpoints {
        inline int32_t& pointsToAdd(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_coneventaddskillpoints.pointsToAdd);
        }

        inline std::string& awardMessage(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_coneventaddskillpoints.awardMessage);
        }

    }

    namespace coneventaddcredits {
        struct Offsets {
            size_t creditsToAdd = 0;
        };
    }

    extern coneventaddcredits::Offsets offsets_coneventaddcredits;

    namespace coneventaddcredits {
        inline int32_t& creditsToAdd(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_coneventaddcredits.creditsToAdd);
        }

    }

    namespace coneventtrade {
        struct Offsets {
            size_t eventOwner = 0;
            size_t eventOwnerName = 0;
        };
    }

    extern coneventtrade::Offsets offsets_coneventtrade;

    namespace coneventtrade {
        inline UObject*& eventOwner(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_coneventtrade.eventOwner);
        }

        inline std::string& eventOwnerName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_coneventtrade.eventOwnerName);
        }

    }

    namespace coneventmovecamera {
        struct Offsets {
            size_t cameraType = 0;
            size_t cameraPosition = 0;
            size_t cameraTransition = 0;
            size_t cameraOffset = 0;
            size_t Rotation = 0;
            size_t heightModifier = 0;
            size_t centerModifier = 0;
            size_t distanceMultiplier = 0;
            size_t cameraActor = 0;
            size_t cameraActorName = 0;
        };
    }

    extern coneventmovecamera::Offsets offsets_coneventmovecamera;

    namespace coneventmovecamera {
        inline uint8_t& cameraType(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_coneventmovecamera.cameraType);
        }

        inline uint8_t& cameraPosition(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_coneventmovecamera.cameraPosition);
        }

        inline uint8_t& cameraTransition(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_coneventmovecamera.cameraTransition);
        }

        inline Vector& cameraOffset(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_coneventmovecamera.cameraOffset);
        }

        inline Rotator& Rotation(UObject* o)
        {
            return *reinterpret_cast<Rotator*>(o->data + offsets_coneventmovecamera.Rotation);
        }

        inline float& heightModifier(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_coneventmovecamera.heightModifier);
        }

        inline float& centerModifier(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_coneventmovecamera.centerModifier);
        }

        inline float& distanceMultiplier(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_coneventmovecamera.distanceMultiplier);
        }

        inline UObject*& cameraActor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_coneventmovecamera.cameraActor);
        }

        inline std::string& cameraActorName(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_coneventmovecamera.cameraActorName);
        }

    }

    namespace coneventcomment {
        struct Offsets {
            size_t commentText = 0;
        };
    }

    extern coneventcomment::Offsets offsets_coneventcomment;

    namespace coneventcomment {
        inline std::string& commentText(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_coneventcomment.commentText);
        }

    }

    namespace concamerawindow {
        struct Offsets {
            size_t Camera = 0;
        };
    }

    extern concamerawindow::Offsets offsets_concamerawindow;

    namespace concamerawindow {
        inline UObject*& Camera(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_concamerawindow.Camera);
        }

    }

    void bindProps_consys();
}
