#include "compiler/assets.h"

#include "compiler/con_file.h"
#include "compiler/exec_parse.h"
#include "compiler/script_package.h"
#include "core/objects.h"
#include "engine/package.h"
#include "engine/resources.h"
#include "extension/consys.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <map>
#include <unordered_map>
#include <vector>

namespace dx::compiler {
    namespace {
        enum EventType : uint8_t {
            EventSpeech = 0,
            EventChoice = 1,
            EventSetFlag = 2,
            EventCheckFlag = 3,
            EventCheckObject = 4,
            EventTransferObject = 5,
            EventMoveCamera = 6,
            EventAnimation = 7,
            EventTrade = 8,
            EventJump = 9,
            EventRandomLabel = 10,
            EventTrigger = 11,
            EventAddGoal = 12,
            EventAddNote = 13,
            EventAddSkillPoints = 14,
            EventAddCredits = 15,
            EventCheckPersona = 16,
            EventComment = 17,
            EventEnd = 18,
        };

        constexpr int32_t ConVersion = 0x1c;
        constexpr const char* ConSignature = "Deus Ex Conversation File";

        const char* eventClassNames[] = {
            "ConEventSpeech",
            "ConEventChoice",
            "ConEventSetFlag",
            "ConEventCheckFlag",
            "ConEventCheckObject",
            "ConEventTransferObject",
            "ConEventMoveCamera",
            "ConEventAnimation",
            "ConEventTrade",
            "ConEventJump",
            "ConEventRandomLabel",
            "ConEventTrigger",
            "ConEventAddGoal",
            "ConEventAddNote",
            "ConEventAddSkillPoints",
            "ConEventAddCredits",
            "ConEventCheckPersona",
            "ConEventComment",
            "ConEventEnd",
        };

        UClass* conClass(const std::string& name)
        {
            return objects::findClass("ConSys." + name);
        }

        template <typename T>
        T& field(UObject* object, const char* name)
        {
            return *reinterpret_cast<T*>(object->propertyAddress(Name(name)));
        }

        template <typename T>
        T& member(UObject* object, const char* name, const char* memberName)
        {
            UProperty* property = object->cls->findProperty(Name(name));
            UProperty* inner = property->structType->findProperty(Name(memberName));
            return *reinterpret_cast<T*>(object->data + property->offset + inner->offset);
        }

        std::string& text(UObject* object, const char* name)
        {
            return field<std::string>(object, name);
        }

        int32_t& integer(UObject* object, const char* name)
        {
            return field<int32_t>(object, name);
        }

        uint8_t& byte(UObject* object, const char* name)
        {
            return field<uint8_t>(object, name);
        }

        bool flag(UObject* object, const char* name)
        {
            return field<uint32_t>(object, name) != 0;
        }

        void setFlag(UObject* object, const char* name, bool value)
        {
            field<uint32_t>(object, name) = value ? 1 : 0;
        }

        UObject*& reference(UObject* object, const char* name)
        {
            return field<UObject*>(object, name);
        }

        Name nameFrom(const std::string& value)
        {
            return value.empty() ? Name() : Name(value);
        }

        std::string nameText(UObject* object, const char* name)
        {
            Name value = Name::fromIndex(field<uint32_t>(object, name));
            return value.none() ? std::string() : value.str();
        }

        void setName(UObject* object, const char* name, const std::string& value)
        {
            field<uint32_t>(object, name) = nameFrom(value).index();
        }

        std::filesystem::path resolve(const std::filesystem::path& directory, std::string file)
        {
            std::replace(file.begin(), file.end(), '\\', '/');
            std::filesystem::path result = directory;
            for (const std::filesystem::path& part : std::filesystem::path(file)) {
                std::filesystem::path exact = result / part;
                if (!std::filesystem::exists(exact) && std::filesystem::is_directory(result)) {
                    for (const auto& item : std::filesystem::directory_iterator(result)) {
                        if (iequals(item.path().filename().string(), part.string())) {
                            exact = item.path();
                            break;
                        }
                    }
                }
                result = exact;
            }
            return result;
        }

        bool readFile(const std::filesystem::path& file, std::vector<uint8_t>& data)
        {
            std::ifstream in(file, std::ios::binary);
            if (!in) {
                return false;
            }
            data.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
            return true;
        }

        UObject* findChild(UObject* outer, Name name, UClass* cls)
        {
            for (UObject* object : objects::all()) {
                if (object && !object->destroyed && object->outer == outer && object->name == name && object->cls == cls) {
                    return object;
                }
            }
            return nullptr;
        }

        std::string textPrefix(UObject* package)
        {
            return iequals(package->name.str(), "DeusExConversations") ? "DeusExCon" : package->name.str();
        }

        std::string audioDirectory(const std::string& audioPackage, const std::string& audioName)
        {
            std::string suffix = "Audio" + audioName;
            std::string result = "DeusExCon";
            if (audioPackage.size() > suffix.size() && iequals(audioPackage.substr(audioPackage.size() - suffix.size()), suffix)) {
                result = audioPackage.substr(0, audioPackage.size() - suffix.size());
            }
            if (iequals(result, "DeusExCon")) {
                result = "DeusExConversations";
            }
            return result;
        }

        std::string numbered(const std::string& base, int32_t value)
        {
            char buffer[32];
            std::snprintf(buffer, sizeof(buffer), "%02d", value);
            return base + buffer;
        }

        class ConReader {
        public:
            explicit ConReader(std::vector<uint8_t> source)
                : data(std::move(source))
            {
            }

            int32_t readLong()
            {
                uint32_t value = 0;
                for (int i = 0; i < 4; i++) {
                    value |= static_cast<uint32_t>(at < data.size() ? data[at] : 0) << (8 * i);
                    at++;
                }
                return static_cast<int32_t>(value);
            }

            float readFloat()
            {
                int32_t bits = readLong();
                float value = 0;
                std::memcpy(&value, &bits, sizeof(value));
                return value;
            }

            void skip(int64_t count)
            {
                at = static_cast<size_t>(std::max<int64_t>(0, static_cast<int64_t>(at) + count));
            }

            std::vector<uint8_t> readBytes(int32_t count)
            {
                std::vector<uint8_t> result;
                for (int32_t i = 0; i < count; i++) {
                    result.push_back(at < data.size() ? data[at] : 0);
                    at++;
                }
                return result;
            }

            std::string readString()
            {
                int32_t length = readLong();
                if (length <= 0) {
                    return {};
                }
                std::vector<uint8_t> bytes = readBytes(length);
                std::string result(bytes.begin(), bytes.end());
                return result.substr(0, result.find('\0'));
            }

            void discardString()
            {
                int32_t length = readLong();
                if (length > 0) {
                    skip(length);
                }
            }

            int32_t readActor(std::string& name)
            {
                int32_t id = readLong();
                if (id > -1) {
                    name = readString();
                }
                return id;
            }

        private:
            std::vector<uint8_t> data;
            size_t at = 0;
        };

        class ConImport {
        public:
            ConImport(ExecContext& execContext, std::vector<uint8_t> file)
                : context(execContext)
                , reader(std::move(file))
            {
            }

            void run(const std::string& fileName)
            {
                reader.skip(static_cast<int64_t>(std::strlen(ConSignature)) + 1);
                version = reader.readLong();
                if (version < 0x1a) {
                    context.log("CONIMPORT WARNING: Attempt to import obsolete conversation file " + fileName);
                    return;
                }
                reader.skip(8);
                reader.discardString();
                reader.skip(8);
                reader.discardString();
                audioName = reader.readString();
                reader.discardString();
                int32_t missionCount = reader.readLong();
                for (int32_t i = 0; i < missionCount; i++) {
                    missions.push_back(reader.readLong());
                }
                std::string prefix = textPrefix(context.package);
                textPackage = objects::packageObject(Name(prefix + "Text"));
                audioPackage = objects::packageObject(Name(prefix + "Audio" + audioName));
                missionList = missionConList();
                audioList = conAudioList();
                reader.skip(reader.readLong());
                reader.skip(4);
                int32_t count = reader.readLong();
                for (int32_t i = 0; i < count; i++) {
                    speechLabels = 0;
                    choiceEndLabels = 0;
                    choiceCount = 0;
                    actorReferences.clear();
                    conversation = create("Conversation", textPackage);
                    importConversation();
                    text(conversation, "audioPackageName") = audioName;
                    addConversation(conversation);
                }
                bindJumpLinks();
                std::filesystem::path system = context.directory.parent_path() / "System";
                save(textPackage, system / (prefix + "Text.u"));
                save(audioPackage, system / (prefix + "Audio" + audioName + ".u"));
            }

        private:
            UObject* create(const std::string& className, UObject* outer, Name name = Name(), uint32_t flags = RF_Public)
            {
                return objects::newObject(conClass(className), outer, name, flags);
            }

            void save(UObject* package, const std::filesystem::path& path)
            {
                std::string error;
                std::filesystem::create_directories(path.parent_path());
                if (!saveScriptPackage(package, path, &error)) {
                    context.log("Failed to save " + path.string() + ": " + error);
                }
            }

            UObject* missionConList()
            {
                UClass* cls = conClass("ConversationMissionList");
                UObject* found = findChild(textPackage, Name("ConMissionList"), cls);
                return found ? found : objects::newObject(cls, textPackage, Name("ConMissionList"), RF_Public | RF_Standalone);
            }

            UObject* conAudioList()
            {
                UClass* cls = conClass("ConAudioList");
                Name name("ConAudioList_" + audioName);
                UObject* found = findChild(audioPackage, name, cls);
                return found ? found : objects::newObject(cls, audioPackage, name, RF_Public | RF_Standalone);
            }

            void prepend(UObject* owner, const char* head, UObject* object)
            {
                UObject* item = create("ConItem", textPackage);
                reference(item, "Next") = reference(owner, head);
                reference(item, "ConObject") = object;
                reference(owner, head) = item;
            }

            UObject* conList(int32_t mission)
            {
                for (UObject* item = reference(missionList, "missions"); item; item = reference(item, "Next")) {
                    UObject* list = reference(item, "ConObject");
                    if (list && integer(list, "missionNumber") == mission) {
                        return list;
                    }
                }
                UObject* list = create("ConversationList", textPackage, Name(numbered("ConList_Mission", mission)), RF_Public | RF_Standalone);
                integer(list, "missionNumber") = mission;
                text(list, "missionDescription") = "Mission " + std::to_string(mission);
                prepend(missionList, "missions", list);
                return list;
            }

            void addConversation(UObject* added)
            {
                for (int32_t mission : missions) {
                    prepend(conList(mission), "conversations", added);
                }
            }

            UObject* importFlagRefs()
            {
                int32_t count = reader.readLong();
                UObject* head = nullptr;
                UObject* previous = nullptr;
                for (int32_t i = 0; i < count; i++) {
                    UObject* flagRef = create("ConFlagRef", textPackage);
                    reader.readLong();
                    setName(flagRef, "FlagName", reader.readString());
                    setFlag(flagRef, "Value", (reader.readLong() & 1) != 0);
                    integer(flagRef, "expiration") = reader.readLong();
                    if (!head) {
                        head = flagRef;
                    }
                    if (previous) {
                        reference(previous, "nextFlagRef") = flagRef;
                    }
                    previous = flagRef;
                }
                return head;
            }

            void setSpeakerName(const std::string& name)
            {
                speakerName = name;
                if (!iequals(name, "JCDenton")) {
                    lastNonPlayerSpeaker = name;
                }
            }

            int32_t incActorReference(int32_t id)
            {
                for (auto& [actor, count] : actorReferences) {
                    if (actor == id) {
                        count++;
                        return count;
                    }
                }
                actorReferences.emplace_back(id, 1);
                return 1;
            }

            int32_t actorReferenceCount(int32_t id) const
            {
                for (const auto& [actor, count] : actorReferences) {
                    if (actor == id) {
                        return count;
                    }
                }
                return 0;
            }

            int32_t importAudio(const std::string& path)
            {
                std::string directory = audioDirectory(audioPackage->name.str(), audioName);
                std::filesystem::path file = resolve(context.directory.parent_path(), directory + "\\Audio\\" + audioName + "\\" + path + ".mp3");
                std::vector<uint8_t> data;
                if (!readFile(file, data) || data.empty()) {
                    context.log("ImportSpeechAudio Failed: " + file.string());
                    return -1;
                }
                int32_t& count = integer(audioList, "audioCount");
                Name name("ConAudio" + audioName + "_" + std::to_string(count));
                UClass* soundClass = objects::findClass("Engine.Sound");
                UObject* object = findChild(audioPackage, name, soundClass);
                if (!object) {
                    object = objects::newObject(soundClass, audioPackage, name, RF_Public | RF_Standalone);
                }
                USound* sound = cast<USound>(object);
                sound->format = Name("mp3");
                sound->bytes = std::move(data);
                cast<extension::UConAudioList>(audioList)->sounds.push_back(sound);
                return count++;
            }

            std::string ownerPath() const
            {
                return text(conversation, "conOwnerName") + "\\" + nameText(conversation, "conName") + "\\";
            }

            int32_t importSpeechAudio()
            {
                reader.readString();
                std::string path = flag(conversation, "bDataLinkCon") ? "InfoLink\\" + nameText(conversation, "conName") + "\\" : ownerPath();
                return importAudio(path + numbered(speakerName, actorReferenceCount(speakerID)));
            }

            int32_t importChoiceAudio(int32_t index)
            {
                reader.readString();
                return importAudio(ownerPath() + numbered("Choice", choiceCount) + static_cast<char>('a' + index));
            }

            void importChoice(UObject* choice, int32_t index)
            {
                reader.readLong();
                text(choice, "choiceText") = reader.readString();
                setFlag(choice, "bDisplayAsSpeech", (reader.readLong() & 1) != 0);
                if (reader.readLong() > -1) {
                    std::string skill = reader.readString();
                    integer(choice, "skillLevelNeeded") = reader.readLong();
                    reference(choice, "skillNeeded") = skill.empty() ? nullptr : findChild(textPackage, Name(skill), objects::classClass());
                }
                text(choice, "choiceLabel") = reader.readString();
                integer(choice, "soundID") = importChoiceAudio(index);
                reference(choice, "flagRef") = importFlagRefs();
            }

            void importSpeech(UObject* event)
            {
                std::string& speaker = text(event, "speakerName");
                speakerID = reader.readActor(speaker);
                setSpeakerName(speaker);
                incActorReference(speakerID);
                reader.readActor(text(event, "speakingToName"));
                UObject* speech = create("ConSpeech", textPackage);
                reference(event, "ConSpeech") = speech;
                text(speech, "speech") = reader.readString();
                integer(speech, "soundID") = importSpeechAudio();
                setFlag(event, "bContinued", (reader.readLong() & 1) != 0);
                setFlag(event, "bBold", (reader.readLong() & 1) != 0);
                byte(event, "speechFont") = static_cast<uint8_t>(reader.readLong());
            }

            void importEvent(UObject* event, uint8_t type)
            {
                switch (type) {
                case EventSpeech:
                    importSpeech(event);
                    break;
                case EventChoice: {
                    reader.readLong();
                    setFlag(event, "bClearScreen", (reader.readLong() & 1) != 0);
                    int32_t count = reader.readLong();
                    choiceCount++;
                    UObject* previous = nullptr;
                    for (int32_t i = 0; i < count; i++) {
                        UObject* choice = create("ConChoice", textPackage);
                        importChoice(choice, i);
                        if (!reference(event, "ChoiceList")) {
                            reference(event, "ChoiceList") = choice;
                        }
                        if (previous) {
                            reference(previous, "nextChoice") = choice;
                        }
                        previous = choice;
                    }
                    break;
                }
                case EventSetFlag:
                    reference(event, "flagRef") = importFlagRefs();
                    break;
                case EventCheckFlag:
                    reference(event, "flagRef") = importFlagRefs();
                    text(event, "setLabel") = reader.readString();
                    break;
                case EventCheckObject:
                    reader.readLong();
                    text(event, "ObjectName") = reader.readString();
                    text(event, "failLabel") = reader.readString();
                    break;
                case EventTransferObject:
                    reader.readLong();
                    text(event, "ObjectName") = reader.readString();
                    integer(event, "transferCount") = version > 0x1a ? reader.readLong() : 1;
                    reader.readActor(text(event, "fromName"));
                    reader.readActor(text(event, "toName"));
                    text(event, "failLabel") = reader.readString();
                    break;
                case EventMoveCamera:
                    byte(event, "cameraType") = static_cast<uint8_t>(reader.readLong());
                    byte(event, "cameraPosition") = static_cast<uint8_t>(reader.readLong());
                    byte(event, "cameraTransition") = static_cast<uint8_t>(reader.readLong());
                    if (byte(event, "cameraType") == 1) {
                        member<int32_t>(event, "Rotation", "Pitch") = reader.readLong();
                        member<int32_t>(event, "Rotation", "Yaw") = reader.readLong();
                        member<int32_t>(event, "Rotation", "Roll") = reader.readLong();
                        field<float>(event, "heightModifier") = reader.readFloat();
                        field<float>(event, "centerModifier") = reader.readFloat();
                        field<float>(event, "distanceMultiplier") = reader.readFloat();
                    } else if (byte(event, "cameraType") == 2) {
                        member<float>(event, "cameraOffset", "X") = reader.readFloat();
                        member<float>(event, "cameraOffset", "Y") = reader.readFloat();
                        member<float>(event, "cameraOffset", "Z") = reader.readFloat();
                    }
                    break;
                case EventAnimation: {
                    reader.readActor(text(event, "eventOwnerName"));
                    setName(event, "Sequence", reader.readString());
                    int32_t mode = reader.readLong();
                    setFlag(event, "bFinishAnim", (mode & 1) != 0);
                    setFlag(event, "bLoopAnim", (mode & 2) != 0);
                    reader.readLong();
                    reader.readLong();
                    break;
                }
                case EventTrade:
                    reader.readActor(text(event, "eventOwnerName"));
                    break;
                case EventJump:
                    text(event, "jumpLabel") = reader.readString();
                    integer(event, "conID") = reader.readLong();
                    jumps.emplace_back(integer(event, "conID"), event);
                    break;
                case EventRandomLabel: {
                    int32_t count = reader.readLong();
                    std::vector<std::string>& labels = cast<extension::UConEventRandomLabel>(event)->labels;
                    for (int32_t i = 0; i < count; i++) {
                        labels.push_back(reader.readString());
                    }
                    setFlag(event, "bCycleEvents", (reader.readLong() & 1) != 0);
                    setFlag(event, "bCycleOnce", (reader.readLong() & 1) != 0);
                    setFlag(event, "bCycleRandom", (reader.readLong() & 1) != 0);
                    break;
                }
                case EventTrigger:
                    setName(event, "triggerTag", reader.readString());
                    break;
                case EventAddGoal:
                    setName(event, "goalName", reader.readString());
                    setFlag(event, "bGoalCompleted", (reader.readLong() & 1) != 0);
                    if (!flag(event, "bGoalCompleted")) {
                        text(event, "goalText") = reader.readString();
                        setFlag(event, "bPrimaryGoal", (reader.readLong() & 1) != 0);
                    }
                    break;
                case EventAddNote:
                    text(event, "noteText") = reader.readString();
                    break;
                case EventAddSkillPoints:
                    integer(event, "pointsToAdd") = reader.readLong();
                    text(event, "awardMessage") = version > 0x1a ? reader.readString() : std::string();
                    break;
                case EventAddCredits:
                    integer(event, "creditsToAdd") = reader.readLong();
                    break;
                case EventCheckPersona:
                    byte(event, "personaType") = static_cast<uint8_t>(reader.readLong());
                    byte(event, "condition") = static_cast<uint8_t>(reader.readLong());
                    integer(event, "Value") = reader.readLong();
                    text(event, "jumpLabel") = reader.readString();
                    break;
                case EventComment: {
                    int32_t count = reader.readLong();
                    std::vector<uint8_t> bytes = reader.readBytes(count);
                    std::vector<uint16_t> units;
                    for (size_t i = 0; i < bytes.size(); i += 2) {
                        uint16_t unit = static_cast<uint16_t>(bytes[i] | (i + 1 < bytes.size() ? bytes[i + 1] << 8 : 0));
                        if (unit == 0) {
                            break;
                        }
                        units.push_back(unit);
                    }
                    text(event, "commentText") = conUnitsToText(units);
                    break;
                }
                default:
                    break;
                }
            }

            void importConversation()
            {
                reader.readLong();
                integer(conversation, "conversationID") = reader.readLong();
                conversations.emplace_back(integer(conversation, "conversationID"), conversation);
                setName(conversation, "conName", reader.readString());
                text(conversation, "Description") = reader.readString();
                reader.skip(8);
                reader.discardString();
                reader.skip(8);
                reader.discardString();
                reader.readActor(text(conversation, "conOwnerName"));
                setFlag(conversation, "bDataLinkCon", (reader.readLong() & 1) != 0);
                reader.discardString();
                setFlag(conversation, "bDisplayOnce", (reader.readLong() & 1) != 0);
                setFlag(conversation, "bFirstPerson", (reader.readLong() & 1) != 0);
                setFlag(conversation, "bNonInteractive", (reader.readLong() & 1) != 0);
                setFlag(conversation, "bRandomCamera", (reader.readLong() & 1) != 0);
                setFlag(conversation, "bCanBeInterrupted", (reader.readLong() & 1) != 0);
                setFlag(conversation, "bCannotBeInterrupted", version > 0x1b && (reader.readLong() & 1) != 0);
                setFlag(conversation, "bInvokeBump", (reader.readLong() & 1) != 0);
                setFlag(conversation, "bInvokeFrob", (reader.readLong() & 1) != 0);
                setFlag(conversation, "bInvokeSight", (reader.readLong() & 1) != 0);
                setFlag(conversation, "bInvokeRadius", (reader.readLong() & 1) != 0);
                integer(conversation, "radiusDistance") = reader.readLong();
                reference(conversation, "flagRefList") = importFlagRefs();
                int32_t count = reader.readLong();
                UObject* previous = nullptr;
                bool choices = false;
                for (int32_t i = 0; i < count; i++) {
                    reader.readLong();
                    reader.readLong();
                    uint8_t type = static_cast<uint8_t>(reader.readLong());
                    std::string label = reader.readString();
                    if (type > EventEnd) {
                        context.log("CONIMPORT WARNING: unknown event type " + std::to_string(type));
                        continue;
                    }
                    choices = choices || type == EventChoice;
                    UObject* event = create(eventClassNames[type], textPackage);
                    byte(event, "eventType") = type;
                    importEvent(event, type);
                    reference(event, "Conversation") = conversation;
                    text(event, "Label") = label;
                    if (!reference(conversation, "eventList")) {
                        reference(conversation, "eventList") = event;
                    }
                    if (previous) {
                        reference(previous, "nextEvent") = event;
                    }
                    previous = event;
                }
                if (choices) {
                    processChoices();
                }
            }

            UObject* createChoiceEvents(UObject* choiceEvent)
            {
                UObject* following = reference(choiceEvent, "nextEvent");
                UObject* last = choiceEvent;
                for (UObject* choice = reference(choiceEvent, "ChoiceList"); choice; choice = reference(choice, "nextChoice")) {
                    if (!flag(choice, "bDisplayAsSpeech")) {
                        continue;
                    }
                    UObject* speech = create("ConEventSpeech", textPackage);
                    UObject* jump = create("ConEventJump", textPackage);
                    byte(speech, "eventType") = EventSpeech;
                    byte(jump, "eventType") = EventJump;
                    reference(speech, "Conversation") = conversation;
                    reference(jump, "jumpCon") = conversation;
                    std::string label = "ChoiceSpeechLabel_" + std::to_string(speechLabels++);
                    integer(jump, "conID") = -1;
                    text(speech, "Label") = label;
                    text(speech, "speakerName") = "JCDenton";
                    text(speech, "speakingToName") = lastNonPlayerSpeaker;
                    UObject* line = create("ConSpeech", textPackage);
                    text(line, "speech") = text(choice, "choiceText");
                    reference(speech, "ConSpeech") = line;
                    integer(line, "soundID") = integer(choice, "soundID");
                    text(jump, "jumpLabel") = text(choice, "choiceLabel");
                    text(choice, "choiceLabel") = label;
                    reference(last, "nextEvent") = speech;
                    reference(speech, "nextEvent") = jump;
                    reference(jump, "nextEvent") = following;
                    last = jump;
                }
                return last;
            }

            void processChoices()
            {
                for (UObject* event = reference(conversation, "eventList"); event; event = reference(event, "nextEvent")) {
                    if (byte(event, "eventType") == EventSpeech) {
                        setSpeakerName(text(event, "speakerName"));
                    } else if (byte(event, "eventType") == EventChoice) {
                        event = createChoiceEvents(event);
                    }
                }
            }

            void bindJumpLinks()
            {
                for (const auto& [id, event] : jumps) {
                    for (const auto& [conversationID, target] : conversations) {
                        if (conversationID == id) {
                            reference(event, "jumpCon") = target;
                            break;
                        }
                    }
                }
            }

            ExecContext& context;
            ConReader reader;
            int32_t version = 0;
            std::string audioName;
            std::vector<int32_t> missions;
            UObject* textPackage = nullptr;
            UObject* audioPackage = nullptr;
            UObject* missionList = nullptr;
            UObject* audioList = nullptr;
            UObject* conversation = nullptr;
            std::vector<std::pair<int32_t, UObject*>> conversations;
            std::vector<std::pair<int32_t, UObject*>> jumps;
            std::vector<std::pair<int32_t, int32_t>> actorReferences;
            int32_t speakerID = 0;
            std::string speakerName;
            std::string lastNonPlayerSpeaker;
            int32_t choiceCount = 0;
            int32_t speechLabels = 0;
            int32_t choiceEndLabels = 0;
        };

        class ConWriter {
        public:
            void writeLong(int32_t value)
            {
                for (int i = 0; i < 4; i++) {
                    data.push_back(static_cast<uint8_t>(static_cast<uint32_t>(value) >> (8 * i)));
                }
            }

            void writeFloat(float value)
            {
                int32_t bits = 0;
                std::memcpy(&bits, &value, sizeof(bits));
                writeLong(bits);
            }

            void writeBytes(const std::vector<uint8_t>& bytes)
            {
                data.insert(data.end(), bytes.begin(), bytes.end());
            }

            void writeString(const std::string& value)
            {
                writeLong(static_cast<int32_t>(value.size()));
                data.insert(data.end(), value.begin(), value.end());
            }

            std::vector<uint8_t> data;
        };

        struct ConFileSet {
            std::vector<std::string> files;
            std::vector<std::vector<UObject*>> conversations;
            std::map<UObject*, std::vector<int32_t>> missions;
            UObject* textPackage = nullptr;
        };

        int32_t suffixNumber(UObject* object)
        {
            const std::string& name = object->name.str();
            size_t digits = name.find_last_not_of("0123456789");
            return digits + 1 < name.size() ? std::atoi(name.c_str() + digits + 1) : 0;
        }

        std::vector<std::string> conversationFiles(UObject* package)
        {
            std::vector<std::string> files;
            for (UObject* object : objects::all()) {
                UClass* cls = cast<UClass>(object);
                if (!cls || cls->destroyed || cls->outer != package || !cls->scriptText) {
                    continue;
                }
                objects::ensureLoaded(cls->scriptText);
                size_t start = 0;
                const std::string& script = cls->scriptText->text;
                while (start < script.size()) {
                    size_t end = script.find('\n', start);
                    std::string_view line(script.data() + start, (end == std::string::npos ? script.size() : end) - start);
                    start = end == std::string::npos ? script.size() : end + 1;
                    std::string file;
                    if (parseCommand(line, "#exec") && parseCommand(line, "CONVERSATION") && parseCommand(line, "IMPORT") && parseString(line, "FILE=", file)) {
                        files.push_back(file);
                    }
                }
            }
            return files;
        }

        ConFileSet partition(UObject* package)
        {
            ConFileSet set;
            set.files = conversationFiles(package);
            Linker* linker = objects::loadPackage(Name(textPrefix(package) + "Text"));
            if (!linker) {
                return set;
            }
            for (size_t i = 0; i < linker->file().exports().size(); i++) {
                objects::ensureLoaded(linker->object(static_cast<int32_t>(i + 1)));
            }
            objects::flushLoads();
            set.textPackage = linker->packageObject;
            UClass* conversationClass = conClass("Conversation");
            std::vector<UObject*> all;
            for (UObject* object : objects::all()) {
                if (object && !object->destroyed && object->cls == conversationClass && object->outermost() == set.textPackage) {
                    all.push_back(object);
                }
            }
            std::stable_sort(all.begin(), all.end(), [](UObject* a, UObject* b) {
                return suffixNumber(a) < suffixNumber(b);
            });
            UObject* missionList = findChild(set.textPackage, Name("ConMissionList"), conClass("ConversationMissionList"));
            std::map<UObject*, std::vector<std::pair<int32_t, int32_t>>> items;
            for (UObject* entry = missionList ? reference(missionList, "missions") : nullptr; entry; entry = reference(entry, "Next")) {
                UObject* list = reference(entry, "ConObject");
                for (UObject* item = list ? reference(list, "conversations") : nullptr; item; item = reference(item, "Next")) {
                    items[reference(item, "ConObject")].emplace_back(suffixNumber(item), integer(list, "missionNumber"));
                }
            }
            for (auto& [conversation, found] : items) {
                std::sort(found.begin(), found.end());
                for (const auto& [item, mission] : found) {
                    set.missions[conversation].push_back(mission);
                }
            }
            auto split = [&](bool withDataLink) {
                std::vector<std::vector<UObject*>> runs;
                UObject* previous = nullptr;
                for (UObject* conversation : all) {
                    bool same = previous && text(previous, "audioPackageName") == text(conversation, "audioPackageName") && set.missions[previous] == set.missions[conversation] && (!withDataLink || flag(previous, "bDataLinkCon") == flag(conversation, "bDataLinkCon"));
                    if (!same) {
                        runs.emplace_back();
                    }
                    runs.back().push_back(conversation);
                    previous = conversation;
                }
                return runs;
            };
            set.conversations = split(true);
            if (set.conversations.size() != set.files.size()) {
                set.conversations = split(false);
            }
            return set;
        }

        bool current(const ConFileSet& set, UObject* package)
        {
            Linker* linker = objects::findLinker(Name(textPrefix(package) + "Text"));
            if (!linker || linker->packageObject != set.textPackage) {
                return false;
            }
            for (const std::vector<UObject*>& run : set.conversations) {
                for (UObject* conversation : run) {
                    if (conversation->destroyed) {
                        return false;
                    }
                }
            }
            return true;
        }

        class ConExport {
        public:
            ConExport(ExecContext& execContext, const ConFileSet& fileSet)
                : context(execContext)
                , set(fileSet)
            {
            }

            bool run(size_t index)
            {
                if (set.conversations.size() != set.files.size()) {
                    context.log("Warning: cannot split " + std::to_string(set.conversations.size()) + " conversation groups over " + std::to_string(set.files.size()) + " conversation files");
                }
                std::vector<UObject*> conversations;
                if (index < set.conversations.size()) {
                    conversations = set.conversations[index];
                }
                if (index + 1 == set.files.size()) {
                    for (size_t i = index + 1; i < set.conversations.size(); i++) {
                        conversations.insert(conversations.end(), set.conversations[i].begin(), set.conversations[i].end());
                    }
                }
                audioName = conversations.empty() ? std::string() : text(conversations.front(), "audioPackageName");
                std::string audioPackageName = textPrefix(context.package) + "Audio" + audioName;
                if (Linker* linker = objects::loadPackage(Name(audioPackageName))) {
                    for (size_t i = 0; i < linker->file().exports().size(); i++) {
                        objects::ensureLoaded(linker->object(static_cast<int32_t>(i + 1)));
                    }
                    objects::flushLoads();
                    UObject* list = findChild(linker->packageObject, Name("ConAudioList_" + audioName), conClass("ConAudioList"));
                    if (list) {
                        sounds = cast<extension::UConAudioList>(list)->sounds;
                    }
                }
                audioRoot = context.directory.parent_path() / audioDirectory(audioPackageName, audioName) / "Audio" / audioName;
                for (UObject* conversation : conversations) {
                    collectActors(conversation);
                }
                ConWriter actorTable;
                actorTable.writeLong(static_cast<int32_t>(actorNames.size()));
                for (size_t i = 0; i < actorNames.size(); i++) {
                    actorTable.writeLong(static_cast<int32_t>(i));
                    actorTable.writeString(actorNames[i]);
                }
                out.writeBytes(std::vector<uint8_t>(ConSignature, ConSignature + std::strlen(ConSignature)));
                out.writeBytes({ 0x1a });
                out.writeLong(ConVersion);
                out.writeBytes(std::vector<uint8_t>(8));
                out.writeString({});
                out.writeBytes(std::vector<uint8_t>(8));
                out.writeString({});
                out.writeString(audioName);
                out.writeString({});
                std::vector<int32_t> missions;
                if (!conversations.empty() && set.missions.count(conversations.front())) {
                    missions = set.missions.at(conversations.front());
                }
                out.writeLong(static_cast<int32_t>(missions.size()));
                for (int32_t mission : missions) {
                    out.writeLong(mission);
                }
                out.writeLong(static_cast<int32_t>(actorTable.data.size()));
                out.writeBytes(actorTable.data);
                out.writeLong(0);
                out.writeLong(static_cast<int32_t>(conversations.size()));
                for (UObject* conversation : conversations) {
                    exportConversation(conversation);
                }
                std::filesystem::path file = context.directory / set.files[index];
                std::string path = file.string();
                std::replace(path.begin(), path.end(), '\\', '/');
                file = path;
                std::filesystem::create_directories(file.parent_path());
                std::ofstream stream(file, std::ios::binary);
                stream.write(reinterpret_cast<const char*>(out.data.data()), static_cast<std::streamsize>(out.data.size()));
                return static_cast<bool>(stream) && success;
            }

        private:
            void addActor(const std::string& name)
            {
                if (!name.empty() && std::find(actorNames.begin(), actorNames.end(), name) == actorNames.end()) {
                    actorNames.push_back(name);
                }
            }

            void collectActors(UObject* conversation)
            {
                addActor(text(conversation, "conOwnerName"));
                for (UObject* event = reference(conversation, "eventList"); event; event = reference(event, "nextEvent")) {
                    switch (byte(event, "eventType")) {
                    case EventSpeech:
                        addActor(text(event, "speakerName"));
                        addActor(text(event, "speakingToName"));
                        break;
                    case EventTransferObject:
                        addActor(text(event, "fromName"));
                        addActor(text(event, "toName"));
                        break;
                    case EventAnimation:
                    case EventTrade:
                        addActor(text(event, "eventOwnerName"));
                        break;
                    default:
                        break;
                    }
                }
            }

            int32_t actorID(const std::string& name) const
            {
                auto found = std::find(actorNames.begin(), actorNames.end(), name);
                return name.empty() || found == actorNames.end() ? -1 : static_cast<int32_t>(found - actorNames.begin());
            }

            void writeActor(const std::string& name)
            {
                int32_t id = actorID(name);
                out.writeLong(id);
                if (id > -1) {
                    out.writeString(name);
                }
            }

            void writeFlagRefs(UObject* head)
            {
                std::vector<UObject*> refs;
                for (UObject* flagRef = head; flagRef; flagRef = reference(flagRef, "nextFlagRef")) {
                    refs.push_back(flagRef);
                }
                out.writeLong(static_cast<int32_t>(refs.size()));
                for (UObject* flagRef : refs) {
                    out.writeLong(0);
                    out.writeString(nameText(flagRef, "FlagName"));
                    out.writeLong(flag(flagRef, "Value") ? 1 : 0);
                    out.writeLong(integer(flagRef, "expiration"));
                }
            }

            void writeAudio(const std::string& path, int32_t soundID)
            {
                out.writeString(path + ".mp3");
                if (soundID < 0 || static_cast<size_t>(soundID) >= sounds.size()) {
                    return;
                }
                USound* sound = cast<USound>(sounds[static_cast<size_t>(soundID)]);
                if (!sound) {
                    return;
                }
                std::string relative = path + ".mp3";
                std::replace(relative.begin(), relative.end(), '\\', '/');
                std::filesystem::path file = audioRoot / relative;
                auto written = writtenAudio.find(file.string());
                if (written != writtenAudio.end()) {
                    if (written->second != sound && cast<USound>(written->second)->bytes != sound->bytes) {
                        context.log("Warning: two sounds map to " + file.string());
                        success = false;
                    }
                    return;
                }
                writtenAudio.emplace(file.string(), sound);
                std::filesystem::create_directories(file.parent_path());
                std::ofstream stream(file, std::ios::binary);
                stream.write(reinterpret_cast<const char*>(sound->bytes.data()), static_cast<std::streamsize>(sound->bytes.size()));
                success = success && static_cast<bool>(stream);
            }

            int32_t incActorReference(int32_t id)
            {
                for (auto& [actor, count] : actorReferences) {
                    if (actor == id) {
                        count++;
                        return count;
                    }
                }
                actorReferences.emplace_back(id, 1);
                return 1;
            }

            void writeSpeech(UObject* conversation, UObject* event)
            {
                const std::string& speaker = text(event, "speakerName");
                writeActor(speaker);
                int32_t references = incActorReference(actorID(speaker));
                writeActor(text(event, "speakingToName"));
                UObject* line = reference(event, "ConSpeech");
                out.writeString(line ? text(line, "speech") : std::string());
                std::string conName = nameText(conversation, "conName");
                std::string path = flag(conversation, "bDataLinkCon") ? "InfoLink\\" + conName + "\\" : text(conversation, "conOwnerName") + "\\" + conName + "\\";
                writeAudio(path + numbered(speaker, references), line ? integer(line, "soundID") : -1);
                out.writeLong(flag(event, "bContinued") ? 1 : 0);
                out.writeLong(flag(event, "bBold") ? 1 : 0);
                out.writeLong(byte(event, "speechFont"));
            }

            void writeChoices(UObject* conversation, UObject* event, const std::vector<std::string>& labels)
            {
                out.writeLong(0);
                out.writeLong(flag(event, "bClearScreen") ? 1 : 0);
                std::vector<UObject*> choices;
                for (UObject* choice = reference(event, "ChoiceList"); choice; choice = reference(choice, "nextChoice")) {
                    choices.push_back(choice);
                }
                out.writeLong(static_cast<int32_t>(choices.size()));
                choiceCount++;
                for (size_t i = 0; i < choices.size(); i++) {
                    UObject* choice = choices[i];
                    out.writeLong(0);
                    out.writeString(text(choice, "choiceText"));
                    out.writeLong(flag(choice, "bDisplayAsSpeech") ? 1 : 0);
                    UObject* skill = reference(choice, "skillNeeded");
                    if (skill || integer(choice, "skillLevelNeeded") != 0) {
                        out.writeLong(0);
                        out.writeString(skill ? skill->name.str() : std::string());
                        out.writeLong(integer(choice, "skillLevelNeeded"));
                    } else {
                        out.writeLong(-1);
                    }
                    out.writeString(labels[i]);
                    std::string path = text(conversation, "conOwnerName") + "\\" + nameText(conversation, "conName") + "\\" + numbered("Choice", choiceCount) + static_cast<char>('a' + i);
                    writeAudio(path, integer(choice, "soundID"));
                    writeFlagRefs(reference(choice, "flagRef"));
                }
            }

            void writeEvent(UObject* conversation, UObject* event, const std::vector<std::string>& choiceLabels)
            {
                uint8_t type = byte(event, "eventType");
                out.writeLong(0);
                out.writeLong(0);
                out.writeLong(type);
                out.writeString(text(event, "Label"));
                switch (type) {
                case EventSpeech:
                    writeSpeech(conversation, event);
                    break;
                case EventChoice:
                    writeChoices(conversation, event, choiceLabels);
                    break;
                case EventSetFlag:
                    writeFlagRefs(reference(event, "flagRef"));
                    break;
                case EventCheckFlag:
                    writeFlagRefs(reference(event, "flagRef"));
                    out.writeString(text(event, "setLabel"));
                    break;
                case EventCheckObject:
                    out.writeLong(0);
                    out.writeString(text(event, "ObjectName"));
                    out.writeString(text(event, "failLabel"));
                    break;
                case EventTransferObject:
                    out.writeLong(0);
                    out.writeString(text(event, "ObjectName"));
                    out.writeLong(integer(event, "transferCount"));
                    writeActor(text(event, "fromName"));
                    writeActor(text(event, "toName"));
                    out.writeString(text(event, "failLabel"));
                    break;
                case EventMoveCamera:
                    out.writeLong(byte(event, "cameraType"));
                    out.writeLong(byte(event, "cameraPosition"));
                    out.writeLong(byte(event, "cameraTransition"));
                    if (byte(event, "cameraType") == 1) {
                        out.writeLong(member<int32_t>(event, "Rotation", "Pitch"));
                        out.writeLong(member<int32_t>(event, "Rotation", "Yaw"));
                        out.writeLong(member<int32_t>(event, "Rotation", "Roll"));
                        out.writeFloat(field<float>(event, "heightModifier"));
                        out.writeFloat(field<float>(event, "centerModifier"));
                        out.writeFloat(field<float>(event, "distanceMultiplier"));
                    } else if (byte(event, "cameraType") == 2) {
                        out.writeFloat(member<float>(event, "cameraOffset", "X"));
                        out.writeFloat(member<float>(event, "cameraOffset", "Y"));
                        out.writeFloat(member<float>(event, "cameraOffset", "Z"));
                    }
                    break;
                case EventAnimation:
                    writeActor(text(event, "eventOwnerName"));
                    out.writeString(nameText(event, "Sequence"));
                    out.writeLong((flag(event, "bFinishAnim") ? 1 : 0) | (flag(event, "bLoopAnim") ? 2 : 0));
                    out.writeLong(0);
                    out.writeLong(0);
                    break;
                case EventTrade:
                    writeActor(text(event, "eventOwnerName"));
                    break;
                case EventJump:
                    out.writeString(text(event, "jumpLabel"));
                    out.writeLong(integer(event, "conID"));
                    break;
                case EventRandomLabel: {
                    const std::vector<std::string>& labels = cast<extension::UConEventRandomLabel>(event)->labels;
                    out.writeLong(static_cast<int32_t>(labels.size()));
                    for (const std::string& label : labels) {
                        out.writeString(label);
                    }
                    out.writeLong(flag(event, "bCycleEvents") ? 1 : 0);
                    out.writeLong(flag(event, "bCycleOnce") ? 1 : 0);
                    out.writeLong(flag(event, "bCycleRandom") ? 1 : 0);
                    break;
                }
                case EventTrigger:
                    out.writeString(nameText(event, "triggerTag"));
                    break;
                case EventAddGoal:
                    out.writeString(nameText(event, "goalName"));
                    out.writeLong(flag(event, "bGoalCompleted") ? 1 : 0);
                    if (!flag(event, "bGoalCompleted")) {
                        out.writeString(text(event, "goalText"));
                        out.writeLong(flag(event, "bPrimaryGoal") ? 1 : 0);
                    }
                    break;
                case EventAddNote:
                    out.writeString(text(event, "noteText"));
                    break;
                case EventAddSkillPoints:
                    out.writeLong(integer(event, "pointsToAdd"));
                    out.writeString(text(event, "awardMessage"));
                    break;
                case EventAddCredits:
                    out.writeLong(integer(event, "creditsToAdd"));
                    break;
                case EventCheckPersona:
                    out.writeLong(byte(event, "personaType"));
                    out.writeLong(byte(event, "condition"));
                    out.writeLong(integer(event, "Value"));
                    out.writeString(text(event, "jumpLabel"));
                    break;
                case EventComment: {
                    std::vector<uint8_t> bytes;
                    for (uint16_t unit : conTextToUnits(text(event, "commentText"))) {
                        bytes.push_back(static_cast<uint8_t>(unit));
                        bytes.push_back(static_cast<uint8_t>(unit >> 8));
                    }
                    bytes.push_back(0);
                    bytes.push_back(0);
                    out.writeLong(static_cast<int32_t>(bytes.size()));
                    out.writeBytes(bytes);
                    break;
                }
                default:
                    break;
                }
            }

            void exportConversation(UObject* conversation)
            {
                actorReferences.clear();
                choiceCount = 0;
                out.writeLong(0);
                out.writeLong(integer(conversation, "conversationID"));
                out.writeString(nameText(conversation, "conName"));
                out.writeString(text(conversation, "Description"));
                out.writeBytes(std::vector<uint8_t>(8));
                out.writeString({});
                out.writeBytes(std::vector<uint8_t>(8));
                out.writeString({});
                writeActor(text(conversation, "conOwnerName"));
                out.writeLong(flag(conversation, "bDataLinkCon") ? 1 : 0);
                out.writeString({});
                const char* flags[] = { "bDisplayOnce", "bFirstPerson", "bNonInteractive", "bRandomCamera", "bCanBeInterrupted", "bCannotBeInterrupted", "bInvokeBump", "bInvokeFrob", "bInvokeSight", "bInvokeRadius" };
                for (const char* name : flags) {
                    out.writeLong(flag(conversation, name) ? 1 : 0);
                }
                out.writeLong(integer(conversation, "radiusDistance"));
                writeFlagRefs(reference(conversation, "flagRefList"));
                std::vector<std::pair<UObject*, std::vector<std::string>>> events;
                UObject* event = reference(conversation, "eventList");
                while (event) {
                    std::vector<std::string> labels;
                    UObject* next = reference(event, "nextEvent");
                    if (byte(event, "eventType") == EventChoice) {
                        for (UObject* choice = reference(event, "ChoiceList"); choice; choice = reference(choice, "nextChoice")) {
                            labels.push_back(text(choice, "choiceLabel"));
                            UObject* jump = next ? reference(next, "nextEvent") : nullptr;
                            if (flag(choice, "bDisplayAsSpeech") && jump && byte(next, "eventType") == EventSpeech && byte(jump, "eventType") == EventJump && text(next, "Label") == labels.back()) {
                                labels.back() = text(jump, "jumpLabel");
                                next = reference(jump, "nextEvent");
                            }
                        }
                    }
                    events.emplace_back(event, labels);
                    event = next;
                }
                out.writeLong(static_cast<int32_t>(events.size()));
                for (const auto& [entry, labels] : events) {
                    writeEvent(conversation, entry, labels);
                }
            }

            ExecContext& context;
            const ConFileSet& set;
            ConWriter out;
            std::string audioName;
            std::vector<UObject*> sounds;
            std::filesystem::path audioRoot;
            std::vector<std::string> actorNames;
            std::vector<std::pair<int32_t, int32_t>> actorReferences;
            std::unordered_map<std::string, UObject*> writtenAudio;
            int32_t choiceCount = 0;
            bool success = true;
        };

        bool parseImport(std::string_view text, std::string& file)
        {
            return parseCommand(text, "CONVERSATION") && parseCommand(text, "IMPORT") && parseString(text, "FILE=", file);
        }

        bool import(ExecContext& context, const std::string& command)
        {
            std::string_view text = command;
            if (!parseCommand(text, "CONVERSATION")) {
                return false;
            }
            std::string file;
            if (!parseImport(command, file)) {
                context.log("Bad CONVERSATION IMPORT");
                return true;
            }
            std::filesystem::path path = resolve(context.directory, file);
            std::vector<uint8_t> data;
            if (!readFile(path, data)) {
                context.log("CONIMPORT WARNING: Can't find file '" + path.string() + "' for import");
                return true;
            }
            ConImport(context, std::move(data)).run(path.string());
            return true;
        }

        bool exportSource(ExecContext& context, const std::string& command)
        {
            std::string file;
            if (!parseImport(command, file)) {
                return false;
            }
            static std::unordered_map<UObject*, ConFileSet> sets;
            auto found = sets.find(context.package);
            if (found != sets.end() && !current(found->second, context.package)) {
                sets.erase(found);
                found = sets.end();
            }
            if (found == sets.end()) {
                found = sets.emplace(context.package, partition(context.package)).first;
            }
            const ConFileSet& set = found->second;
            auto index = std::find_if(set.files.begin(), set.files.end(), [&](const std::string& name) {
                return iequals(name, file);
            });
            if (index == set.files.end() || !set.textPackage) {
                return false;
            }
            return ConExport(context, set).run(static_cast<size_t>(index - set.files.begin()));
        }

        bool serialize(PackageWriter& package, UObject* object, Writer& out)
        {
            if (extension::UConAudioList* list = cast<extension::UConAudioList>(object)) {
                out.index(static_cast<int32_t>(list->sounds.size()));
                for (UObject* sound : list->sounds) {
                    package.writeObject(out, sound);
                }
                return true;
            }
            if (extension::UConEventRandomLabel* event = cast<extension::UConEventRandomLabel>(object)) {
                out.index(static_cast<int32_t>(event->labels.size()));
                for (const std::string& label : event->labels) {
                    out.string(label);
                }
                return true;
            }
            return false;
        }
    }

    std::vector<std::string> conversationImportFiles(UObject* package)
    {
        return conversationFiles(package);
    }

    AssetHandler conversationAssets()
    {
        AssetHandler handler;
        handler.import = import;
        handler.exportSource = exportSource;
        handler.serialize = serialize;
        return handler;
    }
}
