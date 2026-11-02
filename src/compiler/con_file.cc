#include "compiler/con_file.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <set>

namespace dx::compiler {
    namespace {
        constexpr const char* Signature = "Deus Ex Conversation File";

        const char* typeNames[] = {
            "Speech",
            "Choice",
            "Set Flag",
            "Check Flag",
            "Check Object",
            "Transfer Object",
            "Move Camera",
            "Animation",
            "Trade",
            "Jump",
            "Random Label",
            "Trigger",
            "Add Goal",
            "Add Note",
            "Add Skill Points",
            "Add Credits",
            "Check Persona",
            "Comment",
            "End",
        };

        class Input {
        public:
            explicit Input(const std::vector<uint8_t>& source)
                : data(source)
            {
            }

            int32_t integer()
            {
                if (at + 4 > data.size()) {
                    failed = true;
                    at = data.size();
                    return 0;
                }
                uint32_t value = 0;
                for (int i = 0; i < 4; i++) {
                    value |= static_cast<uint32_t>(data[at + static_cast<size_t>(i)]) << (8 * i);
                }
                at += 4;
                return static_cast<int32_t>(value);
            }

            bool boolean()
            {
                return (integer() & 1) != 0;
            }

            float real()
            {
                int32_t bits = integer();
                float value = 0;
                std::memcpy(&value, &bits, sizeof(value));
                return value;
            }

            std::vector<uint8_t> bytes(int64_t count)
            {
                if (count < 0 || at + static_cast<size_t>(count) > data.size()) {
                    failed = true;
                    at = data.size();
                    return {};
                }
                std::vector<uint8_t> result(data.begin() + static_cast<std::ptrdiff_t>(at), data.begin() + static_cast<std::ptrdiff_t>(at + static_cast<size_t>(count)));
                at += static_cast<size_t>(count);
                return result;
            }

            std::array<uint8_t, 8> date()
            {
                std::vector<uint8_t> raw = bytes(8);
                std::array<uint8_t, 8> result {};
                std::copy(raw.begin(), raw.end(), result.begin());
                return result;
            }

            std::string string()
            {
                int32_t length = integer();
                if (length <= 0) {
                    return {};
                }
                std::vector<uint8_t> raw = bytes(length);
                std::string result(raw.begin(), raw.end());
                return result.substr(0, result.find('\0'));
            }

            std::string actor()
            {
                if (integer() > -1) {
                    return string();
                }
                return {};
            }

            const std::vector<uint8_t>& data;
            size_t at = 0;
            bool failed = false;
        };

        class Output {
        public:
            void integer(int32_t value)
            {
                for (int i = 0; i < 4; i++) {
                    data.push_back(static_cast<uint8_t>(static_cast<uint32_t>(value) >> (8 * i)));
                }
            }

            void boolean(bool value)
            {
                integer(value ? 1 : 0);
            }

            void real(float value)
            {
                int32_t bits = 0;
                std::memcpy(&bits, &value, sizeof(bits));
                integer(bits);
            }

            void string(const std::string& value)
            {
                integer(static_cast<int32_t>(value.size()));
                data.insert(data.end(), value.begin(), value.end());
            }

            void date(const std::array<uint8_t, 8>& value)
            {
                data.insert(data.end(), value.begin(), value.end());
            }

            std::vector<uint8_t> data;
        };

        std::vector<ConFlagRef> readFlags(Input& in)
        {
            std::vector<ConFlagRef> flags;
            int32_t count = in.integer();
            for (int32_t i = 0; i < count && !in.failed; i++) {
                ConFlagRef flag;
                flag.id = in.integer();
                flag.name = in.string();
                flag.value = in.boolean();
                flag.expiration = in.integer();
                flags.push_back(flag);
            }
            return flags;
        }

        void readEvent(Input& in, int32_t version, ConEvent& event)
        {
            switch (event.type) {
            case ConEventType::Speech:
                event.speaker = in.actor();
                event.speakingTo = in.actor();
                event.speech = in.string();
                event.audio = in.string();
                event.continued = in.boolean();
                event.bold = in.boolean();
                event.font = in.integer();
                break;
            case ConEventType::Choice: {
                event.choiceReserved = in.integer();
                event.clearScreen = in.boolean();
                int32_t count = in.integer();
                for (int32_t i = 0; i < count && !in.failed; i++) {
                    ConChoice choice;
                    choice.id = in.integer();
                    choice.text = in.string();
                    choice.displayAsSpeech = in.boolean();
                    choice.skillId = in.integer();
                    choice.hasSkill = choice.skillId > -1;
                    if (choice.hasSkill) {
                        choice.skill = in.string();
                        choice.skillLevel = in.integer();
                    }
                    choice.label = in.string();
                    choice.audio = in.string();
                    choice.flags = readFlags(in);
                    event.choices.push_back(choice);
                }
                break;
            }
            case ConEventType::SetFlag:
                event.flags = readFlags(in);
                break;
            case ConEventType::CheckFlag:
                event.flags = readFlags(in);
                event.setLabel = in.string();
                break;
            case ConEventType::CheckObject:
                event.objectId = in.integer();
                event.objectName = in.string();
                event.failLabel = in.string();
                break;
            case ConEventType::TransferObject:
                event.objectId = in.integer();
                event.objectName = in.string();
                event.transferCount = version > 0x1a ? in.integer() : 1;
                event.fromName = in.actor();
                event.toName = in.actor();
                event.failLabel = in.string();
                break;
            case ConEventType::MoveCamera:
                event.cameraType = in.integer();
                event.cameraPosition = in.integer();
                event.cameraTransition = in.integer();
                if (event.cameraType == 1) {
                    for (int32_t& value : event.cameraRotation) {
                        value = in.integer();
                    }
                    event.heightModifier = in.real();
                    event.centerModifier = in.real();
                    event.distanceMultiplier = in.real();
                } else if (event.cameraType == 2) {
                    for (float& value : event.cameraOffset) {
                        value = in.real();
                    }
                }
                break;
            case ConEventType::Animation:
                event.ownerName = in.actor();
                event.sequence = in.string();
                event.animMode = in.integer();
                event.animReserved[0] = in.integer();
                event.animReserved[1] = in.integer();
                break;
            case ConEventType::Trade:
                event.ownerName = in.actor();
                break;
            case ConEventType::Jump:
                event.jumpLabel = in.string();
                event.conId = in.integer();
                break;
            case ConEventType::RandomLabel: {
                int32_t count = in.integer();
                for (int32_t i = 0; i < count && !in.failed; i++) {
                    event.labels.push_back(in.string());
                }
                event.cycleEvents = in.boolean();
                event.cycleOnce = in.boolean();
                event.cycleRandom = in.boolean();
                break;
            }
            case ConEventType::Trigger:
                event.triggerTag = in.string();
                break;
            case ConEventType::AddGoal:
                event.goalName = in.string();
                event.goalCompleted = in.boolean();
                if (!event.goalCompleted) {
                    event.goalText = in.string();
                    event.primaryGoal = in.boolean();
                }
                break;
            case ConEventType::AddNote:
                event.noteText = in.string();
                break;
            case ConEventType::AddSkillPoints:
                event.points = in.integer();
                event.awardMessage = version > 0x1a ? in.string() : std::string();
                break;
            case ConEventType::AddCredits:
                event.credits = in.integer();
                break;
            case ConEventType::CheckPersona:
                event.personaType = in.integer();
                event.condition = in.integer();
                event.personaValue = in.integer();
                event.jumpLabel = in.string();
                break;
            case ConEventType::Comment: {
                std::vector<uint8_t> raw = in.bytes(in.integer());
                std::vector<uint16_t> units;
                for (size_t i = 0; i + 1 < raw.size(); i += 2) {
                    uint16_t unit = static_cast<uint16_t>(raw[i] | (raw[i + 1] << 8));
                    if (unit == 0) {
                        break;
                    }
                    units.push_back(unit);
                }
                event.comment = conUnitsToText(units);
                break;
            }
            case ConEventType::End:
                break;
            }
        }

        void readConversation(Input& in, int32_t version, ConConversation& conversation, std::string* error)
        {
            conversation.reserved = in.integer();
            conversation.id = in.integer();
            conversation.name = in.string();
            conversation.description = in.string();
            conversation.createdOn = in.date();
            conversation.createdBy = in.string();
            conversation.modifiedOn = in.date();
            conversation.modifiedBy = in.string();
            conversation.owner = in.actor();
            conversation.dataLink = in.boolean();
            conversation.notes = in.string();
            conversation.displayOnce = in.boolean();
            conversation.firstPerson = in.boolean();
            conversation.nonInteractive = in.boolean();
            conversation.randomCamera = in.boolean();
            conversation.canBeInterrupted = in.boolean();
            conversation.cannotBeInterrupted = version > 0x1b && in.boolean();
            conversation.invokeBump = in.boolean();
            conversation.invokeFrob = in.boolean();
            conversation.invokeSight = in.boolean();
            conversation.invokeRadius = in.boolean();
            conversation.radius = in.integer();
            conversation.flags = readFlags(in);
            int32_t count = in.integer();
            for (int32_t i = 0; i < count && !in.failed; i++) {
                ConEvent event;
                event.reserved = in.integer();
                event.id = in.integer();
                int32_t type = in.integer();
                event.label = in.string();
                if (type < 0 || type >= ConEventTypeCount) {
                    in.failed = true;
                    if (error) {
                        *error = "unknown event type " + std::to_string(type) + " in " + conversation.name;
                    }
                    return;
                }
                event.type = static_cast<ConEventType>(type);
                readEvent(in, version, event);
                conversation.events.push_back(event);
            }
        }

        int32_t actorId(const std::vector<ConActor>& actors, const std::string& name)
        {
            for (const ConActor& actor : actors) {
                if (actor.name == name) {
                    return actor.id;
                }
            }
            return -1;
        }

        void collectActor(std::vector<ConActor>& actors, const std::string& name)
        {
            if (name.empty() || actorId(actors, name) > -1) {
                return;
            }
            int32_t next = 0;
            for (const ConActor& actor : actors) {
                next = std::max(next, actor.id + 1);
            }
            actors.push_back({ next, name });
        }

        std::vector<ConActor> actorTable(const ConFile& file)
        {
            std::vector<ConActor> actors = file.actors;
            for (const ConConversation& conversation : file.conversations) {
                collectActor(actors, conversation.owner);
                for (const ConEvent& event : conversation.events) {
                    collectActor(actors, event.speaker);
                    collectActor(actors, event.speakingTo);
                    collectActor(actors, event.fromName);
                    collectActor(actors, event.toName);
                    collectActor(actors, event.ownerName);
                }
            }
            return actors;
        }

        class Writer {
        public:
            explicit Writer(const ConFile& source)
                : file(source)
                , actors(actorTable(source))
            {
            }

            std::vector<uint8_t> run()
            {
                out.data.insert(out.data.end(), Signature, Signature + std::strlen(Signature));
                out.data.push_back(0x1a);
                out.integer(file.version);
                out.date(file.createdOn);
                out.string(file.createdBy);
                out.date(file.modifiedOn);
                out.string(file.modifiedBy);
                out.string(file.audioPackage);
                out.string(file.notes);
                out.integer(static_cast<int32_t>(file.missions.size()));
                for (int32_t mission : file.missions) {
                    out.integer(mission);
                }
                Output table;
                table.integer(static_cast<int32_t>(actors.size()));
                for (const ConActor& actor : actors) {
                    table.integer(actor.id);
                    table.string(actor.name);
                }
                out.integer(static_cast<int32_t>(table.data.size()));
                out.data.insert(out.data.end(), table.data.begin(), table.data.end());
                out.integer(file.reserved);
                out.integer(static_cast<int32_t>(file.conversations.size()));
                for (const ConConversation& conversation : file.conversations) {
                    writeConversation(conversation);
                }
                return out.data;
            }

        private:
            void actor(const std::string& name)
            {
                int32_t id = name.empty() ? -1 : actorId(actors, name);
                out.integer(id);
                if (id > -1) {
                    out.string(name);
                }
            }

            void flags(const std::vector<ConFlagRef>& refs)
            {
                out.integer(static_cast<int32_t>(refs.size()));
                for (const ConFlagRef& flag : refs) {
                    out.integer(flag.id);
                    out.string(flag.name);
                    out.boolean(flag.value);
                    out.integer(flag.expiration);
                }
            }

            void event(const ConEvent& event)
            {
                out.integer(event.reserved);
                out.integer(event.id);
                out.integer(static_cast<int32_t>(event.type));
                out.string(event.label);
                switch (event.type) {
                case ConEventType::Speech:
                    actor(event.speaker);
                    actor(event.speakingTo);
                    out.string(event.speech);
                    out.string(event.audio);
                    out.boolean(event.continued);
                    out.boolean(event.bold);
                    out.integer(event.font);
                    break;
                case ConEventType::Choice:
                    out.integer(event.choiceReserved);
                    out.boolean(event.clearScreen);
                    out.integer(static_cast<int32_t>(event.choices.size()));
                    for (const ConChoice& choice : event.choices) {
                        out.integer(choice.id);
                        out.string(choice.text);
                        out.boolean(choice.displayAsSpeech);
                        if (choice.hasSkill) {
                            out.integer(std::max(choice.skillId, 0));
                            out.string(choice.skill);
                            out.integer(choice.skillLevel);
                        } else {
                            out.integer(-1);
                        }
                        out.string(choice.label);
                        out.string(choice.audio);
                        flags(choice.flags);
                    }
                    break;
                case ConEventType::SetFlag:
                    flags(event.flags);
                    break;
                case ConEventType::CheckFlag:
                    flags(event.flags);
                    out.string(event.setLabel);
                    break;
                case ConEventType::CheckObject:
                    out.integer(event.objectId);
                    out.string(event.objectName);
                    out.string(event.failLabel);
                    break;
                case ConEventType::TransferObject:
                    out.integer(event.objectId);
                    out.string(event.objectName);
                    if (file.version > 0x1a) {
                        out.integer(event.transferCount);
                    }
                    actor(event.fromName);
                    actor(event.toName);
                    out.string(event.failLabel);
                    break;
                case ConEventType::MoveCamera:
                    out.integer(event.cameraType);
                    out.integer(event.cameraPosition);
                    out.integer(event.cameraTransition);
                    if (event.cameraType == 1) {
                        for (int32_t value : event.cameraRotation) {
                            out.integer(value);
                        }
                        out.real(event.heightModifier);
                        out.real(event.centerModifier);
                        out.real(event.distanceMultiplier);
                    } else if (event.cameraType == 2) {
                        for (float value : event.cameraOffset) {
                            out.real(value);
                        }
                    }
                    break;
                case ConEventType::Animation:
                    actor(event.ownerName);
                    out.string(event.sequence);
                    out.integer(event.animMode);
                    out.integer(event.animReserved[0]);
                    out.integer(event.animReserved[1]);
                    break;
                case ConEventType::Trade:
                    actor(event.ownerName);
                    break;
                case ConEventType::Jump:
                    out.string(event.jumpLabel);
                    out.integer(event.conId);
                    break;
                case ConEventType::RandomLabel:
                    out.integer(static_cast<int32_t>(event.labels.size()));
                    for (const std::string& label : event.labels) {
                        out.string(label);
                    }
                    out.boolean(event.cycleEvents);
                    out.boolean(event.cycleOnce);
                    out.boolean(event.cycleRandom);
                    break;
                case ConEventType::Trigger:
                    out.string(event.triggerTag);
                    break;
                case ConEventType::AddGoal:
                    out.string(event.goalName);
                    out.boolean(event.goalCompleted);
                    if (!event.goalCompleted) {
                        out.string(event.goalText);
                        out.boolean(event.primaryGoal);
                    }
                    break;
                case ConEventType::AddNote:
                    out.string(event.noteText);
                    break;
                case ConEventType::AddSkillPoints:
                    out.integer(event.points);
                    if (file.version > 0x1a) {
                        out.string(event.awardMessage);
                    }
                    break;
                case ConEventType::AddCredits:
                    out.integer(event.credits);
                    break;
                case ConEventType::CheckPersona:
                    out.integer(event.personaType);
                    out.integer(event.condition);
                    out.integer(event.personaValue);
                    out.string(event.jumpLabel);
                    break;
                case ConEventType::Comment: {
                    std::vector<uint8_t> raw;
                    for (uint16_t unit : conTextToUnits(event.comment)) {
                        raw.push_back(static_cast<uint8_t>(unit));
                        raw.push_back(static_cast<uint8_t>(unit >> 8));
                    }
                    raw.push_back(0);
                    raw.push_back(0);
                    out.integer(static_cast<int32_t>(raw.size()));
                    out.data.insert(out.data.end(), raw.begin(), raw.end());
                    break;
                }
                case ConEventType::End:
                    break;
                }
            }

            void writeConversation(const ConConversation& conversation)
            {
                out.integer(conversation.reserved);
                out.integer(conversation.id);
                out.string(conversation.name);
                out.string(conversation.description);
                out.date(conversation.createdOn);
                out.string(conversation.createdBy);
                out.date(conversation.modifiedOn);
                out.string(conversation.modifiedBy);
                actor(conversation.owner);
                out.boolean(conversation.dataLink);
                out.string(conversation.notes);
                out.boolean(conversation.displayOnce);
                out.boolean(conversation.firstPerson);
                out.boolean(conversation.nonInteractive);
                out.boolean(conversation.randomCamera);
                out.boolean(conversation.canBeInterrupted);
                if (file.version > 0x1b) {
                    out.boolean(conversation.cannotBeInterrupted);
                }
                out.boolean(conversation.invokeBump);
                out.boolean(conversation.invokeFrob);
                out.boolean(conversation.invokeSight);
                out.boolean(conversation.invokeRadius);
                out.integer(conversation.radius);
                flags(conversation.flags);
                out.integer(static_cast<int32_t>(conversation.events.size()));
                for (const ConEvent& entry : conversation.events) {
                    event(entry);
                }
            }

            const ConFile& file;
            std::vector<ConActor> actors;
            Output out;
        };

        std::string folded(const std::string& value)
        {
            std::string result = value;
            for (char& c : result) {
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            }
            return result;
        }

        void addTarget(std::vector<std::string>& targets, const std::string& label)
        {
            if (!label.empty()) {
                targets.push_back(label);
            }
        }

        int32_t renameIn(std::string& value, const std::string& from, const std::string& to)
        {
            if (value.empty() || folded(value) != folded(from)) {
                return 0;
            }
            value = to;
            return 1;
        }

        void jumpTo(const ConFile& file, ConPreview& preview, const std::string& label)
        {
            preview.event = label.empty() ? -1 : findConLabel(file.conversations[preview.conversation], label);
        }

        bool personaPasses(const ConEvent& event, const ConPreview& preview)
        {
            int32_t value = event.personaType == 0 ? preview.credits : (event.personaType == 1 ? preview.health : preview.skillPoints);
            switch (event.condition) {
            case 0:
                return value < event.personaValue;
            case 1:
                return value <= event.personaValue;
            case 2:
                return value == event.personaValue;
            case 3:
                return value >= event.personaValue;
            default:
                return value > event.personaValue;
            }
        }
    }

    const char* conEventTypeName(ConEventType type)
    {
        int32_t index = static_cast<int32_t>(type);
        return index >= 0 && index < ConEventTypeCount ? typeNames[index] : "Unknown";
    }

    std::string conUnitsToText(const std::vector<uint16_t>& units)
    {
        bool wide = std::any_of(units.begin(), units.end(), [](uint16_t unit) {
            return unit > 0xff;
        });
        std::string result;
        for (uint16_t unit : units) {
            if (!wide || unit < 0x80) {
                result.push_back(static_cast<char>(unit));
            } else if (unit < 0x800) {
                result.push_back(static_cast<char>(0xc0 | (unit >> 6)));
                result.push_back(static_cast<char>(0x80 | (unit & 0x3f)));
            } else {
                result.push_back(static_cast<char>(0xe0 | (unit >> 12)));
                result.push_back(static_cast<char>(0x80 | ((unit >> 6) & 0x3f)));
                result.push_back(static_cast<char>(0x80 | (unit & 0x3f)));
            }
        }
        return result;
    }

    std::vector<uint16_t> conTextToUnits(const std::string& value)
    {
        std::vector<uint16_t> units;
        bool wide = false;
        for (size_t i = 0; i < value.size(); i++) {
            uint8_t lead = static_cast<uint8_t>(value[i]);
            size_t extra = lead < 0x80 ? 0 : ((lead & 0xe0) == 0xc0 ? 1 : ((lead & 0xf0) == 0xe0 ? 2 : 3));
            if (extra > 2 || i + extra >= value.size()) {
                wide = false;
                break;
            }
            uint32_t unit = extra == 0 ? lead : (extra == 1 ? lead & 0x1fu : lead & 0x0fu);
            bool continuation = true;
            for (size_t k = 1; k <= extra; k++) {
                uint8_t next = static_cast<uint8_t>(value[i + k]);
                continuation = continuation && (next & 0xc0) == 0x80;
                unit = (unit << 6) | (next & 0x3fu);
            }
            if (!continuation) {
                wide = false;
                break;
            }
            i += extra;
            wide = wide || unit > 0xff;
            units.push_back(static_cast<uint16_t>(unit));
        }
        if (wide) {
            return units;
        }
        units.clear();
        for (char c : value) {
            units.push_back(static_cast<uint8_t>(c));
        }
        return units;
    }

    bool readConFile(const std::vector<uint8_t>& data, ConFile& file, std::string* error)
    {
        file = ConFile();
        size_t length = std::strlen(Signature);
        if (data.size() < length + 1 || std::memcmp(data.data(), Signature, length) != 0) {
            if (error) {
                *error = "not a Deus Ex conversation file";
            }
            return false;
        }
        Input in(data);
        in.at = length + 1;
        file.version = in.integer();
        if (file.version < 0x1a) {
            if (error) {
                *error = "obsolete conversation file version " + std::to_string(file.version);
            }
            return false;
        }
        file.createdOn = in.date();
        file.createdBy = in.string();
        file.modifiedOn = in.date();
        file.modifiedBy = in.string();
        file.audioPackage = in.string();
        file.notes = in.string();
        int32_t missions = in.integer();
        for (int32_t i = 0; i < missions && !in.failed; i++) {
            file.missions.push_back(in.integer());
        }
        std::vector<uint8_t> table = in.bytes(in.integer());
        Input actors(table);
        int32_t actorCount = table.empty() ? 0 : actors.integer();
        for (int32_t i = 0; i < actorCount && !actors.failed; i++) {
            ConActor actor;
            actor.id = actors.integer();
            actor.name = actors.string();
            file.actors.push_back(actor);
        }
        file.reserved = in.integer();
        int32_t count = in.integer();
        for (int32_t i = 0; i < count && !in.failed; i++) {
            ConConversation conversation;
            readConversation(in, file.version, conversation, error);
            file.conversations.push_back(conversation);
        }
        if (in.failed || actors.failed) {
            if (error && error->empty()) {
                *error = "truncated conversation file";
            }
            return false;
        }
        return true;
    }

    std::vector<uint8_t> writeConFile(const ConFile& file)
    {
        return Writer(file).run();
    }

    bool loadConFile(const std::filesystem::path& path, ConFile& file, std::string* error)
    {
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            if (error) {
                *error = "cannot open " + path.string();
            }
            return false;
        }
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        return readConFile(data, file, error);
    }

    bool saveConFile(const std::filesystem::path& path, const ConFile& file, std::string* error)
    {
        std::vector<uint8_t> data = writeConFile(file);
        std::error_code ignored;
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path(), ignored);
        }
        std::ofstream out(path, std::ios::binary);
        out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        if (!out) {
            if (error) {
                *error = "cannot write " + path.string();
            }
            return false;
        }
        return true;
    }

    std::string conSpeechAudioPath(const ConConversation& conversation, size_t index)
    {
        if (index >= conversation.events.size()) {
            return {};
        }
        const std::string& speaker = conversation.events[index].speaker;
        int32_t count = 0;
        for (size_t i = 0; i <= index; i++) {
            if (conversation.events[i].type == ConEventType::Speech && conversation.events[i].speaker == speaker) {
                count++;
            }
        }
        std::string directory = conversation.dataLink ? "InfoLink\\" + conversation.name + "\\" : conversation.owner + "\\" + conversation.name + "\\";
        char number[16];
        std::snprintf(number, sizeof(number), "%02d", count);
        return directory + speaker + number + ".mp3";
    }

    std::string conChoiceAudioPath(const ConConversation& conversation, size_t index, size_t choice)
    {
        int32_t count = 0;
        for (size_t i = 0; i <= index && i < conversation.events.size(); i++) {
            if (conversation.events[i].type == ConEventType::Choice) {
                count++;
            }
        }
        char number[16];
        std::snprintf(number, sizeof(number), "%02d", count);
        return conversation.owner + "\\" + conversation.name + "\\Choice" + number + static_cast<char>('a' + choice) + ".mp3";
    }

    ConEvent makeConEvent(ConEventType type)
    {
        ConEvent event;
        event.type = type;
        if (type == ConEventType::Speech) {
            event.speaker = "JCDenton";
        }
        if (type == ConEventType::MoveCamera) {
            event.distanceMultiplier = 1.0f;
        }
        if (type == ConEventType::TransferObject) {
            event.toName = "JCDenton";
        }
        return event;
    }

    ConConversation& addConversation(ConFile& file, const std::string& name, const std::string& owner)
    {
        ConConversation conversation;
        for (const ConConversation& existing : file.conversations) {
            conversation.id = std::max(conversation.id, existing.id + 1);
        }
        conversation.name = name;
        conversation.owner = owner;
        conversation.events.push_back(makeConEvent(ConEventType::End));
        file.conversations.push_back(conversation);
        return file.conversations.back();
    }

    void insertConEvent(ConConversation& conversation, size_t index, const ConEvent& event)
    {
        ConEvent added = event;
        for (const ConEvent& existing : conversation.events) {
            added.id = std::max(added.id, existing.id + 1);
        }
        index = std::min(index, conversation.events.size());
        conversation.events.insert(conversation.events.begin() + static_cast<std::ptrdiff_t>(index), added);
    }

    void removeConEvent(ConConversation& conversation, size_t index)
    {
        if (index < conversation.events.size()) {
            conversation.events.erase(conversation.events.begin() + static_cast<std::ptrdiff_t>(index));
        }
    }

    bool moveConEvent(ConConversation& conversation, size_t from, size_t to)
    {
        if (from >= conversation.events.size() || to >= conversation.events.size() || from == to) {
            return false;
        }
        ConEvent event = conversation.events[from];
        conversation.events.erase(conversation.events.begin() + static_cast<std::ptrdiff_t>(from));
        conversation.events.insert(conversation.events.begin() + static_cast<std::ptrdiff_t>(to), event);
        return true;
    }

    int32_t renameConLabel(ConConversation& conversation, const std::string& from, const std::string& to)
    {
        int32_t count = 0;
        for (ConEvent& event : conversation.events) {
            count += renameIn(event.label, from, to);
            count += renameIn(event.setLabel, from, to);
            count += renameIn(event.failLabel, from, to);
            count += renameIn(event.jumpLabel, from, to);
            for (std::string& label : event.labels) {
                count += renameIn(label, from, to);
            }
            for (ConChoice& choice : event.choices) {
                count += renameIn(choice.label, from, to);
            }
        }
        return count;
    }

    std::vector<std::string> conLabelTargets(const ConEvent& event)
    {
        std::vector<std::string> targets;
        switch (event.type) {
        case ConEventType::Choice:
            for (const ConChoice& choice : event.choices) {
                addTarget(targets, choice.label);
            }
            break;
        case ConEventType::CheckFlag:
            addTarget(targets, event.setLabel);
            break;
        case ConEventType::CheckObject:
        case ConEventType::TransferObject:
            addTarget(targets, event.failLabel);
            break;
        case ConEventType::Jump:
        case ConEventType::CheckPersona:
            addTarget(targets, event.jumpLabel);
            break;
        case ConEventType::RandomLabel:
            for (const std::string& label : event.labels) {
                addTarget(targets, label);
            }
            break;
        case ConEventType::Speech:
        case ConEventType::SetFlag:
        case ConEventType::MoveCamera:
        case ConEventType::Animation:
        case ConEventType::Trade:
        case ConEventType::Trigger:
        case ConEventType::AddGoal:
        case ConEventType::AddNote:
        case ConEventType::AddSkillPoints:
        case ConEventType::AddCredits:
        case ConEventType::Comment:
        case ConEventType::End:
            break;
        }
        return targets;
    }

    int32_t findConLabel(const ConConversation& conversation, const std::string& label)
    {
        for (size_t i = 0; i < conversation.events.size(); i++) {
            if (!label.empty() && folded(conversation.events[i].label) == folded(label)) {
                return static_cast<int32_t>(i);
            }
        }
        return -1;
    }

    std::vector<std::string> checkConFile(const ConFile& file)
    {
        std::vector<std::string> issues;
        std::set<std::string> names;
        std::set<int32_t> ids;
        for (const ConConversation& conversation : file.conversations) {
            if (conversation.name.empty()) {
                issues.push_back("conversation " + std::to_string(conversation.id) + " has no name");
            } else if (!names.insert(conversation.name).second) {
                issues.push_back(conversation.name + ": duplicate conversation name");
            }
            if (!ids.insert(conversation.id).second) {
                issues.push_back(conversation.name + ": duplicate conversation id " + std::to_string(conversation.id));
            }
            if (conversation.owner.empty() && !conversation.dataLink) {
                issues.push_back(conversation.name + ": no owner");
            }
            std::set<std::string> labels;
            for (size_t i = 0; i < conversation.events.size(); i++) {
                const std::string& label = conversation.events[i].label;
                if (!label.empty() && !labels.insert(folded(label)).second) {
                    issues.push_back(conversation.name + ": event " + std::to_string(i) + " repeats label " + label);
                }
            }
            for (size_t i = 0; i < conversation.events.size(); i++) {
                const ConEvent& event = conversation.events[i];
                if (event.type == ConEventType::Speech && event.speaker.empty()) {
                    issues.push_back(conversation.name + ": event " + std::to_string(i) + " has no speaker");
                }
                if (event.type == ConEventType::Choice && event.choices.empty()) {
                    issues.push_back(conversation.name + ": event " + std::to_string(i) + " has no choices");
                }
                if (event.type == ConEventType::Jump && event.conId >= 0 && event.conId != conversation.id) {
                    auto target = std::find_if(file.conversations.begin(), file.conversations.end(), [&](const ConConversation& other) {
                        return other.id == event.conId;
                    });
                    if (target == file.conversations.end()) {
                        issues.push_back(conversation.name + ": event " + std::to_string(i) + " jumps to missing conversation " + std::to_string(event.conId));
                    } else if (!event.jumpLabel.empty() && findConLabel(*target, event.jumpLabel) < 0) {
                        issues.push_back(conversation.name + ": event " + std::to_string(i) + " jumps to missing label " + event.jumpLabel + " in " + target->name);
                    }
                    continue;
                }
                for (const std::string& target : conLabelTargets(event)) {
                    if (labels.count(folded(target)) == 0) {
                        issues.push_back(conversation.name + ": event " + std::to_string(i) + " jumps to missing label " + target);
                    }
                }
            }
        }
        return issues;
    }

    bool conChoiceAvailable(const ConChoice& choice, const ConPreview& preview)
    {
        for (const ConFlagRef& flag : choice.flags) {
            auto found = preview.flags.find(flag.name);
            bool value = found != preview.flags.end() && found->second;
            if (value != flag.value) {
                return false;
            }
        }
        return true;
    }

    void startConPreview(const ConFile& file, size_t conversation, ConPreview& preview)
    {
        preview.conversation = conversation;
        preview.event = conversation < file.conversations.size() && !file.conversations[conversation].events.empty() ? 0 : -1;
    }

    bool advanceConPreview(const ConFile& file, ConPreview& preview, int32_t choice)
    {
        if (preview.conversation >= file.conversations.size() || preview.event < 0) {
            return false;
        }
        const ConConversation& conversation = file.conversations[preview.conversation];
        size_t index = static_cast<size_t>(preview.event);
        if (index >= conversation.events.size()) {
            preview.event = -1;
            return false;
        }
        const ConEvent& event = conversation.events[index];
        preview.event = index + 1 < conversation.events.size() ? static_cast<int32_t>(index + 1) : -1;
        switch (event.type) {
        case ConEventType::Choice:
            if (choice < 0 || static_cast<size_t>(choice) >= event.choices.size() || !conChoiceAvailable(event.choices[static_cast<size_t>(choice)], preview)) {
                preview.event = static_cast<int32_t>(index);
                return false;
            }
            jumpTo(file, preview, event.choices[static_cast<size_t>(choice)].label);
            break;
        case ConEventType::SetFlag:
            for (const ConFlagRef& flag : event.flags) {
                preview.flags[flag.name] = flag.value;
            }
            break;
        case ConEventType::CheckFlag: {
            bool match = std::all_of(event.flags.begin(), event.flags.end(), [&](const ConFlagRef& flag) {
                auto found = preview.flags.find(flag.name);
                return (found != preview.flags.end() && found->second) == flag.value;
            });
            if (match) {
                jumpTo(file, preview, event.setLabel);
            }
            break;
        }
        case ConEventType::CheckObject:
            if (preview.inventory[event.objectName] <= 0) {
                jumpTo(file, preview, event.failLabel);
            }
            break;
        case ConEventType::TransferObject:
            if (event.toName == "JCDenton") {
                preview.inventory[event.objectName] += event.transferCount;
            } else if (event.fromName == "JCDenton") {
                preview.inventory[event.objectName] -= event.transferCount;
            }
            break;
        case ConEventType::Jump:
            if (event.conId >= 0 && event.conId != conversation.id) {
                for (size_t i = 0; i < file.conversations.size(); i++) {
                    if (file.conversations[i].id == event.conId) {
                        preview.conversation = i;
                        jumpTo(file, preview, event.jumpLabel);
                        return true;
                    }
                }
                preview.event = -1;
            } else if (!event.jumpLabel.empty()) {
                jumpTo(file, preview, event.jumpLabel);
            }
            break;
        case ConEventType::RandomLabel:
            if (!event.labels.empty()) {
                size_t& cycle = preview.cycles[{ preview.conversation, index }];
                jumpTo(file, preview, event.labels[cycle % event.labels.size()]);
                cycle++;
            }
            break;
        case ConEventType::AddSkillPoints:
            preview.skillPoints += event.points;
            break;
        case ConEventType::AddCredits:
            preview.credits += event.credits;
            break;
        case ConEventType::CheckPersona:
            if (personaPasses(event, preview)) {
                jumpTo(file, preview, event.jumpLabel);
            }
            break;
        case ConEventType::End:
            preview.event = -1;
            break;
        case ConEventType::Speech:
        case ConEventType::MoveCamera:
        case ConEventType::Animation:
        case ConEventType::Trade:
        case ConEventType::Trigger:
        case ConEventType::AddGoal:
        case ConEventType::AddNote:
        case ConEventType::Comment:
            break;
        }
        return true;
    }
}
