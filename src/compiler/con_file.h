#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace dx::compiler {
    enum class ConEventType : int32_t {
        Speech = 0,
        Choice = 1,
        SetFlag = 2,
        CheckFlag = 3,
        CheckObject = 4,
        TransferObject = 5,
        MoveCamera = 6,
        Animation = 7,
        Trade = 8,
        Jump = 9,
        RandomLabel = 10,
        Trigger = 11,
        AddGoal = 12,
        AddNote = 13,
        AddSkillPoints = 14,
        AddCredits = 15,
        CheckPersona = 16,
        Comment = 17,
        End = 18,
    };

    constexpr int32_t ConEventTypeCount = 19;

    struct ConFlagRef {
        int32_t id = 0;
        std::string name;
        bool value = true;
        int32_t expiration = 0;
    };

    struct ConChoice {
        int32_t id = 0;
        std::string text;
        bool displayAsSpeech = true;
        bool hasSkill = false;
        int32_t skillId = 0;
        std::string skill;
        int32_t skillLevel = 0;
        std::string label;
        std::string audio;
        std::vector<ConFlagRef> flags;
    };

    struct ConEvent {
        int32_t reserved = 0;
        int32_t id = 0;
        ConEventType type = ConEventType::Speech;
        std::string label;

        std::string speaker;
        std::string speakingTo;
        std::string speech;
        std::string audio;
        bool continued = false;
        bool bold = false;
        int32_t font = 0;

        int32_t choiceReserved = 0;
        bool clearScreen = false;
        std::vector<ConChoice> choices;

        std::vector<ConFlagRef> flags;
        std::string setLabel;

        int32_t objectId = 0;
        std::string objectName;
        std::string failLabel;
        int32_t transferCount = 1;
        std::string fromName;
        std::string toName;

        int32_t cameraType = 0;
        int32_t cameraPosition = 0;
        int32_t cameraTransition = 0;
        std::array<int32_t, 3> cameraRotation {};
        float heightModifier = 0;
        float centerModifier = 0;
        float distanceMultiplier = 0;
        std::array<float, 3> cameraOffset {};

        std::string ownerName;
        std::string sequence;
        int32_t animMode = 0;
        std::array<int32_t, 2> animReserved {};

        std::string jumpLabel;
        int32_t conId = -1;

        std::vector<std::string> labels;
        bool cycleEvents = false;
        bool cycleOnce = false;
        bool cycleRandom = false;

        std::string triggerTag;

        std::string goalName;
        bool goalCompleted = false;
        std::string goalText;
        bool primaryGoal = false;

        std::string noteText;

        int32_t points = 0;
        std::string awardMessage;

        int32_t credits = 0;

        int32_t personaType = 0;
        int32_t condition = 0;
        int32_t personaValue = 0;

        std::string comment;
    };

    struct ConConversation {
        int32_t reserved = 0;
        int32_t id = 0;
        std::string name;
        std::string description;
        std::array<uint8_t, 8> createdOn {};
        std::string createdBy;
        std::array<uint8_t, 8> modifiedOn {};
        std::string modifiedBy;
        std::string owner;
        bool dataLink = false;
        std::string notes;
        bool displayOnce = false;
        bool firstPerson = false;
        bool nonInteractive = false;
        bool randomCamera = false;
        bool canBeInterrupted = false;
        bool cannotBeInterrupted = false;
        bool invokeBump = false;
        bool invokeFrob = true;
        bool invokeSight = false;
        bool invokeRadius = false;
        int32_t radius = 0;
        std::vector<ConFlagRef> flags;
        std::vector<ConEvent> events;
    };

    struct ConActor {
        int32_t id = 0;
        std::string name;
    };

    struct ConFile {
        int32_t version = 0x1c;
        std::array<uint8_t, 8> createdOn {};
        std::string createdBy;
        std::array<uint8_t, 8> modifiedOn {};
        std::string modifiedBy;
        std::string audioPackage;
        std::string notes;
        std::vector<int32_t> missions;
        std::vector<ConActor> actors;
        int32_t reserved = 0;
        std::vector<ConConversation> conversations;
    };

    const char* conEventTypeName(ConEventType type);
    std::string conUnitsToText(const std::vector<uint16_t>& units);
    std::vector<uint16_t> conTextToUnits(const std::string& value);

    bool readConFile(const std::vector<uint8_t>& data, ConFile& file, std::string* error);
    std::vector<uint8_t> writeConFile(const ConFile& file);
    bool loadConFile(const std::filesystem::path& path, ConFile& file, std::string* error);
    bool saveConFile(const std::filesystem::path& path, const ConFile& file, std::string* error);

    std::string conSpeechAudioPath(const ConConversation& conversation, size_t index);
    std::string conChoiceAudioPath(const ConConversation& conversation, size_t index, size_t choice);
    std::vector<std::string> packageConFiles(const std::string& packageName);
    bool exportPackageConFile(const std::string& packageName, const std::string& file, const std::filesystem::path& directory, std::vector<std::string>& log);
    int32_t compileConFile(const std::filesystem::path& directory, const std::string& file, const std::string& packageName, std::vector<std::string>& log);

    ConEvent makeConEvent(ConEventType type);
    ConConversation& addConversation(ConFile& file, const std::string& name, const std::string& owner);
    void insertConEvent(ConConversation& conversation, size_t index, const ConEvent& event);
    void removeConEvent(ConConversation& conversation, size_t index);
    bool moveConEvent(ConConversation& conversation, size_t from, size_t to);
    int32_t renameConLabel(ConConversation& conversation, const std::string& from, const std::string& to);
    std::vector<std::string> conLabelTargets(const ConEvent& event);
    int32_t findConLabel(const ConConversation& conversation, const std::string& label);
    std::vector<std::string> checkConFile(const ConFile& file);

    struct ConPreview {
        size_t conversation = 0;
        int32_t event = -1;
        std::map<std::string, bool> flags;
        std::map<std::string, int32_t> inventory;
        std::map<std::pair<size_t, size_t>, size_t> cycles;
        int32_t credits = 0;
        int32_t health = 100;
        int32_t skillPoints = 0;
    };

    bool conChoiceAvailable(const ConChoice& choice, const ConPreview& preview);
    void startConPreview(const ConFile& file, size_t conversation, ConPreview& preview);
    bool advanceConPreview(const ConFile& file, ConPreview& preview, int32_t choice);
}
