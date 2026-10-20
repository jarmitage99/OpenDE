#include "extension/windows.h"

#include "core/objects.h"
#include "engine/props_consys.h"
#include "engine/props_deusex.h"
#include "engine/props_engine.h"
#include "engine/reader.h"
#include "engine/resources.h"
#include "engine/world.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <random>
#include <unordered_map>

namespace dx::extension {
    class UConAudioList : public UObject {
    public:
        void serialize(Reader& reader, Linker& source) override
        {
            int32_t count = reader.index();
            for (int32_t i = 0; i < count; i++) {
                sounds.push_back(source.readObject(reader));
            }
        }

        std::vector<UObject*> sounds;
    };

    class UConEventRandomLabel : public UObject {
    public:
        void serialize(Reader& reader, Linker&) override
        {
            int32_t count = reader.index();
            for (int32_t i = 0; i < count; i++) {
                labels.push_back(reader.string());
            }
        }

        std::vector<std::string> labels;
    };

    class UTextParser : public UObject {
    public:
        UObject* textObject = nullptr;
    };

    namespace {
        constexpr uint8_t ET_Speech = 0;
        constexpr uint8_t ET_TransferObject = 5;
        constexpr uint8_t ET_MoveCamera = 6;
        constexpr uint8_t ET_Animation = 7;
        constexpr uint8_t ET_Trade = 8;

        bool sameName(const std::string& a, const std::string& b)
        {
            if (a.empty() || a.size() != b.size()) {
                return false;
            }
            for (size_t i = 0; i < a.size(); i++) {
                if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) {
                    return false;
                }
            }
            return true;
        }

        std::string lowerText(const std::string& text)
        {
            std::string result = text;
            for (char& c : result) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            return result;
        }

        bool readU32(const std::vector<uint8_t>& bytes, size_t at, uint32_t& value)
        {
            if (at + 4 > bytes.size()) {
                return false;
            }
            std::memcpy(&value, bytes.data() + at, 4);
            return true;
        }

        float soundLength(UObject* object)
        {
            static const int32_t bitrates[16][5] = {
                { -1, -1, -1, -1, -1 },
                { 32, 32, 32, 32, 8 },
                { 64, 48, 40, 48, 16 },
                { 96, 56, 48, 56, 24 },
                { 128, 64, 56, 64, 32 },
                { 160, 80, 64, 80, 40 },
                { 192, 96, 80, 96, 48 },
                { 224, 112, 96, 112, 56 },
                { 256, 128, 112, 128, 64 },
                { 288, 160, 128, 144, 80 },
                { 320, 192, 160, 160, 96 },
                { 352, 224, 192, 176, 112 },
                { 384, 256, 224, 192, 128 },
                { 416, 320, 256, 224, 144 },
                { 448, 384, 320, 256, 160 },
                { -1, -1, -1, -1, -1 },
            };
            USound* sound = cast<USound>(object);
            if (!sound) {
                return 0.0f;
            }
            objects::ensureLoaded(sound);
            const std::vector<uint8_t>& bytes = sound->bytes;
            uint32_t header = 0;
            if (!readU32(bytes, 0, header)) {
                return 0.0f;
            }
            if (std::memcmp(bytes.data(), "RIFF", 4) == 0) {
                uint32_t byteRate = 0;
                uint32_t dataSize = 0;
                readU32(bytes, 0x1c, byteRate);
                readU32(bytes, 0x28, dataSize);
                return byteRate == 0 ? 0.0f : static_cast<float>(static_cast<double>(dataSize) / static_cast<int32_t>(byteRate));
            }
            if ((header & 0xff) != 0xff || (header & 0xe000) != 0xe000) {
                return 0.0f;
            }
            uint32_t version = header & 0x1800;
            uint32_t layer = header & 0x600;
            int32_t column = 0;
            if (version == 0x1800) {
                if (layer == 0x400) {
                    column = 1;
                } else if (layer == 0x200) {
                    column = 2;
                }
            } else if (version == 0x1000 || version == 0) {
                if (layer == 0x600) {
                    column = 3;
                } else if (layer == 0x400 || layer == 0x200) {
                    column = 4;
                }
            }
            int32_t bitsPerSecond = bitrates[(header >> 20) & 0xf][column] * 1000;
            return static_cast<float>(static_cast<double>(static_cast<int32_t>(bytes.size() * 8)) / bitsPerSecond);
        }

        void bindEvent(UObject* event, const std::unordered_map<std::string, UObject*>& actors, bool clear)
        {
            auto lookup = [&](const std::string& name) -> UObject* {
                if (clear) {
                    return nullptr;
                }
                auto it = actors.find(lowerText(name));
                return it == actors.end() ? nullptr : it->second;
            };
            switch (conevent::eventType(event)) {
            case ET_Speech:
                coneventspeech::speaker(event) = lookup(coneventspeech::speakerName(event));
                coneventspeech::speakingTo(event) = lookup(coneventspeech::speakingToName(event));
                break;
            case ET_TransferObject:
                coneventtransferobject::fromActor(event) = lookup(coneventtransferobject::fromName(event));
                coneventtransferobject::toActor(event) = lookup(coneventtransferobject::toName(event));
                break;
            case ET_MoveCamera:
                coneventmovecamera::cameraActor(event) = lookup(coneventmovecamera::cameraActorName(event));
                break;
            case ET_Animation:
                coneventanimation::eventOwner(event) = lookup(coneventanimation::eventOwnerName(event));
                break;
            case ET_Trade:
                coneventtrade::eventOwner(event) = lookup(coneventtrade::eventOwnerName(event));
                break;
            default:
                break;
            }
        }

        void addActor(std::unordered_map<std::string, UObject*>& actors, UObject* actor)
        {
            if (!actor || actor->destroyed || !actor->data || !actor->isA(Name("Actor"))) {
                return;
            }
            const std::string& bind = actor::BindName(actor);
            if (!bind.empty()) {
                actors[lowerText(bind)] = actor;
            }
        }

        void nativeBindEvents(UObject* object, Frame& stack, void*)
        {
            vm::get<UObject*>(stack);
            UObject* invokeActor = vm::get<UObject*>(stack);
            vm::finish(stack);
            if (!object) {
                return;
            }
            std::unordered_map<std::string, UObject*> actors;
            if (World::current) {
                for (UObject* actor : World::current->actors) {
                    addActor(actors, actor);
                }
            }
            addActor(actors, invokeActor);
            for (UObject* event = conversation::eventList(object); event; event = conevent::nextEvent(event)) {
                objects::ensureLoaded(event);
                bindEvent(event, actors, false);
            }
        }

        void nativeBindActorEvents(UObject* object, Frame& stack, void*)
        {
            UObject* actor = vm::get<UObject*>(stack);
            vm::finish(stack);
            if (!object || !actor || !actor->data) {
                return;
            }
            const std::string& bind = actor::BindName(actor);
            const std::string& bark = actor::BarkBindName(actor);
            bool firstPerson = conversation::bFirstPerson(object) != 0;
            auto matches = [&](const std::string& name, bool needFirstPerson) {
                if (sameName(name, bark)) {
                    return true;
                }
                return sameName(name, bind) && (!needFirstPerson || firstPerson);
            };
            for (UObject* event = conversation::eventList(object); event; event = conevent::nextEvent(event)) {
                objects::ensureLoaded(event);
                switch (conevent::eventType(event)) {
                case ET_Speech:
                    if (matches(coneventspeech::speakerName(event), false)) {
                        coneventspeech::speaker(event) = actor;
                    }
                    if (matches(coneventspeech::speakingToName(event), false)) {
                        coneventspeech::speakingTo(event) = actor;
                    }
                    break;
                case ET_TransferObject:
                    if (matches(coneventtransferobject::fromName(event), true)) {
                        coneventtransferobject::fromActor(event) = actor;
                    }
                    if (matches(coneventtransferobject::toName(event), true)) {
                        coneventtransferobject::toActor(event) = actor;
                    }
                    break;
                case ET_MoveCamera:
                    if (sameName(coneventmovecamera::cameraActorName(event), bind)) {
                        coneventmovecamera::cameraActor(event) = actor;
                    }
                    break;
                case ET_Animation:
                    if (matches(coneventanimation::eventOwnerName(event), true)) {
                        coneventanimation::eventOwner(event) = actor;
                    }
                    break;
                case ET_Trade:
                    if (sameName(coneventtrade::eventOwnerName(event), bind)) {
                        coneventtrade::eventOwner(event) = actor;
                    }
                    break;
                default:
                    break;
                }
            }
        }

        void clearBindings(UObject* conversation)
        {
            std::unordered_map<std::string, UObject*> none;
            for (UObject* event = conversation::eventList(conversation); event; event = conevent::nextEvent(event)) {
                objects::ensureLoaded(event);
                bindEvent(event, none, true);
            }
        }

        void nativeIndex2054(UObject* object, Frame& stack, void* result)
        {
            if (object && object->isA(Name("ConEvent"))) {
                UObject* sound = vm::get<UObject*>(stack);
                vm::finish(stack);
                vm::ret<float>(result, soundLength(sound));
                return;
            }
            vm::finish(stack);
            if (object && object->isA(Name("Conversation"))) {
                clearBindings(object);
            }
        }

        void nativeCreateConCamera(UObject*, Frame& stack, void* result)
        {
            vm::finish(stack);
            UClass* cls = extensionClass("ConSys.ConCamera");
            UObject* camera = cls ? objects::newObject(cls, objects::packageObject(Name("Transient")), Name(), RF_Transient) : nullptr;
            vm::ret<UObject*>(result, camera);
        }

        void nativeCreateFlagRef(UObject*, Frame& stack, void* result)
        {
            Name name = vm::get<Name>(stack);
            bool value = vm::getBool(stack);
            vm::finish(stack);
            UClass* cls = extensionClass("ConSys.ConFlagRef");
            UObject* flag = cls ? objects::newObject(cls, objects::packageObject(Name("Transient")), Name(), RF_Transient) : nullptr;
            if (flag) {
                conflagref::FlagName(flag) = name;
                conflagref::Value(flag) = value ? 1 : 0;
            }
            vm::ret<UObject*>(result, flag);
        }

        UObject* speechAudio(UObject* conversation, int32_t soundId)
        {
            if (!conversation || soundId < 0) {
                return nullptr;
            }
            const std::string& audioPackage = conversation::audioPackageName(conversation);
            if (audioPackage.empty()) {
                return nullptr;
            }
            Linker* linker = objects::loadPackage(Name("DeusExConAudio" + audioPackage));
            if (!linker) {
                return nullptr;
            }
            int32_t ref = linker->findExport(Name("ConAudioList_" + audioPackage), Name("ConAudioList"), "*");
            if (ref == 0) {
                return nullptr;
            }
            UConAudioList* list = cast<UConAudioList>(linker->object(ref));
            objects::ensureLoaded(list);
            if (!list || static_cast<size_t>(soundId) >= list->sounds.size()) {
                return nullptr;
            }
            UObject* sound = list->sounds[static_cast<size_t>(soundId)];
            objects::ensureLoaded(sound);
            return sound;
        }

        void nativeGetSpeechAudio(UObject* object, Frame& stack, void* result)
        {
            int32_t soundId = vm::get<int32_t>(stack);
            vm::finish(stack);
            vm::ret<UObject*>(result, speechAudio(object, soundId));
        }

        void nativeGetSpeechLength(UObject* object, Frame& stack, void* result)
        {
            int32_t soundId = vm::get<int32_t>(stack);
            vm::finish(stack);
            UObject* sound = speechAudio(object, soundId);
            vm::ret<float>(result, sound ? soundLength(sound) : 0.0f);
        }

        UConEventRandomLabel* labelSelf(UObject* object)
        {
            return cast<UConEventRandomLabel>(object);
        }

        void nativeGetLabelCount(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UConEventRandomLabel* event = labelSelf(object);
            vm::ret<int32_t>(result, event ? static_cast<int32_t>(event->labels.size()) : 0);
        }

        void nativeGetLabel(UObject* object, Frame& stack, void* result)
        {
            int32_t index = vm::get<int32_t>(stack);
            vm::finish(stack);
            UConEventRandomLabel* event = labelSelf(object);
            std::string label;
            if (event && index >= 0 && static_cast<size_t>(index) < event->labels.size()) {
                label = event->labels[static_cast<size_t>(index)];
            }
            vm::ret<std::string>(result, label);
        }

        void nativeGetRandomLabel(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UConEventRandomLabel* event = labelSelf(object);
            if (!event || event->labels.empty()) {
                vm::ret<std::string>(result, std::string());
                return;
            }
            static std::mt19937 random(12345u);
            int32_t count = static_cast<int32_t>(event->labels.size());
            int32_t& cycle = coneventrandomlabel::cycleIndex(event);
            int32_t index = 0;
            if (!coneventrandomlabel::bCycleEvents(event)) {
                index = static_cast<int32_t>(random() % static_cast<uint32_t>(count));
            } else if (coneventrandomlabel::bLabelsCycled(event) && coneventrandomlabel::bCycleOnce(event)) {
                index = count - 1;
            } else if (coneventrandomlabel::bLabelsCycled(event) && coneventrandomlabel::bCycleRandom(event)) {
                index = static_cast<int32_t>(random() % static_cast<uint32_t>(count));
            } else {
                index = cycle % count;
                cycle++;
                if (cycle >= count) {
                    coneventrandomlabel::bLabelsCycled(event) = 1;
                    cycle = 0;
                }
            }
            vm::ret<std::string>(result, event->labels[static_cast<size_t>(index)]);
        }

        constexpr uint8_t TT_Text = 0;
        constexpr uint8_t TT_File = 1;
        constexpr uint8_t TT_Email = 2;
        constexpr uint8_t TT_Note = 3;
        constexpr uint8_t TT_EndNote = 4;
        constexpr uint8_t TT_Goal = 5;
        constexpr uint8_t TT_EndGoal = 6;
        constexpr uint8_t TT_Comment = 7;
        constexpr uint8_t TT_EndComment = 8;
        constexpr uint8_t TT_PlayerName = 9;
        constexpr uint8_t TT_PlayerFirstName = 10;
        constexpr uint8_t TT_NewPage = 11;
        constexpr uint8_t TT_CenterText = 12;
        constexpr uint8_t TT_LeftJustify = 13;
        constexpr uint8_t TT_RightJustify = 14;
        constexpr uint8_t TT_DefaultColor = 15;
        constexpr uint8_t TT_TextColor = 16;
        constexpr uint8_t TT_RevertColor = 17;
        constexpr uint8_t TT_NewParagraph = 18;
        constexpr uint8_t TT_Bold = 19;
        constexpr uint8_t TT_EndBold = 20;
        constexpr uint8_t TT_Underline = 21;
        constexpr uint8_t TT_EndUnderline = 22;
        constexpr uint8_t TT_Italics = 23;
        constexpr uint8_t TT_EndItalics = 24;
        constexpr uint8_t TT_Graphic = 25;
        constexpr uint8_t TT_Font = 26;
        constexpr uint8_t TT_Label = 27;
        constexpr uint8_t TT_None = 30;

        UTextParser* parserSelf(UObject* object)
        {
            return cast<UTextParser>(object);
        }

        const std::string& parserText(UTextParser* parser)
        {
            static const std::string empty;
            if (!parser || !parser->textObject) {
                return empty;
            }
            return extstring::Text(parser->textObject);
        }

        std::vector<std::string> splitFields(const std::string& value)
        {
            std::vector<std::string> fields;
            size_t start = 0;
            while (true) {
                size_t comma = value.find(',', start);
                if (comma == std::string::npos) {
                    fields.push_back(value.substr(start));
                    break;
                }
                fields.push_back(value.substr(start, comma - start));
                start = comma + 1;
            }
            return fields;
        }

        bool parseColor(const std::string& value, Color& color)
        {
            std::vector<std::string> fields = splitFields(value);
            if (fields.size() < 3) {
                return false;
            }
            color.r = static_cast<uint8_t>(std::clamp(std::atoi(fields[0].c_str()), 0, 255));
            color.g = static_cast<uint8_t>(std::clamp(std::atoi(fields[1].c_str()), 0, 255));
            color.b = static_cast<uint8_t>(std::clamp(std::atoi(fields[2].c_str()), 0, 255));
            color.a = 255;
            return true;
        }

        bool processTag(UTextParser* parser, const std::string& tag)
        {
            std::string nameText = tag;
            std::string value;
            size_t equals = tag.find('=');
            if (equals != std::string::npos) {
                nameText = tag.substr(0, equals);
                value = tag.substr(equals + 1);
            }
            std::string upper = nameText;
            for (char& c : upper) {
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            }
            uint8_t& lastTag = deusextextparser::lastTag(parser);
            static const std::unordered_map<std::string, uint8_t> simple = {
                { "P", TT_NewParagraph },
                { "B", TT_Bold },
                { "/B", TT_EndBold },
                { "I", TT_Italics },
                { "/I", TT_EndItalics },
                { "U", TT_Underline },
                { "/U", TT_EndUnderline },
                { "JC", TT_CenterText },
                { "JL", TT_LeftJustify },
                { "JR", TT_RightJustify },
                { "NP", TT_NewPage },
                { "NOTE", TT_Note },
                { "/NOTE", TT_EndNote },
                { "GOAL", TT_Goal },
                { "/GOAL", TT_EndGoal },
                { "COMMENT", TT_Comment },
                { "/COMMENT", TT_EndComment },
                { "/C", TT_RevertColor },
                { "/COLOR", TT_RevertColor },
            };
            auto found = simple.find(upper);
            if (found != simple.end() && equals == std::string::npos) {
                lastTag = found->second;
                if (lastTag == TT_NewParagraph) {
                    deusextextparser::bParagraphStarted(parser) = 1;
                }
                return true;
            }
            if (upper == "PLAYERNAME") {
                lastTag = TT_PlayerName;
                deusextextparser::lastText(parser) = deusextextparser::PlayerName(parser);
                return true;
            }
            if (upper == "PLAYERFIRSTNAME") {
                lastTag = TT_PlayerFirstName;
                deusextextparser::lastText(parser) = deusextextparser::playerFirstName(parser);
                return true;
            }
            if (upper == "DC") {
                Color color {};
                if (parseColor(value, color)) {
                    deusextextparser::DefaultColor(parser) = color;
                    deusextextparser::lastColor(parser) = color;
                    lastTag = TT_DefaultColor;
                    return true;
                }
                return false;
            }
            if (upper == "C" || upper == "COLOR") {
                Color color {};
                if (parseColor(value, color)) {
                    deusextextparser::lastColor(parser) = color;
                    lastTag = TT_TextColor;
                    return true;
                }
                return false;
            }
            if (upper == "FILE") {
                std::vector<std::string> fields = splitFields(value);
                deusextextparser::lastFileName(parser) = fields.size() > 0 ? fields[0] : std::string();
                std::string description;
                for (size_t i = 1; i < fields.size(); i++) {
                    if (i > 1) {
                        description += ",";
                    }
                    description += fields[i];
                }
                deusextextparser::lastFileDescription(parser) = description;
                lastTag = TT_File;
                return true;
            }
            if (upper == "EMAIL") {
                std::vector<std::string> fields = splitFields(value);
                fields.resize(std::max<size_t>(fields.size(), 5));
                deusextextparser::lastEmailName(parser) = fields[0];
                deusextextparser::lastEmailSubject(parser) = fields[1];
                deusextextparser::lastEmailFrom(parser) = fields[2];
                deusextextparser::lastEmailTo(parser) = fields[3];
                deusextextparser::lastEmailCC(parser) = fields[4];
                lastTag = TT_Email;
                return true;
            }
            if (upper == "GRAPHIC" || upper == "FONT" || upper == "LABEL") {
                deusextextparser::lastName(parser) = Name(value);
                lastTag = upper == "GRAPHIC" ? TT_Graphic : (upper == "FONT" ? TT_Font : TT_Label);
                return true;
            }
            return false;
        }

        bool processText(UTextParser* parser)
        {
            const std::string& text = parserText(parser);
            int32_t& position = deusextextparser::textPos(parser);
            size_t pos = static_cast<size_t>(std::max(position, 0));
            uint8_t& lastTag = deusextextparser::lastTag(parser);
            std::string& lastText = deusextextparser::lastText(parser);
            lastTag = TT_None;
            lastText.clear();
            if (pos >= text.size()) {
                position = static_cast<int32_t>(text.size());
                return false;
            }
            std::string value;
            bool gotText = false;
            while (pos < text.size()) {
                char c = text[pos];
                if (c == '<') {
                    if (gotText) {
                        break;
                    }
                    size_t end = text.find('>', pos);
                    std::string tag = text.substr(pos + 1, end == std::string::npos ? std::string::npos : end - pos - 1);
                    pos = end == std::string::npos ? text.size() : end + 1;
                    bool paragraphStarted = deusextextparser::bParagraphStarted(parser) != 0;
                    if (!processTag(parser, tag)) {
                        lastTag = TT_None;
                    }
                    deusextextparser::tagEndPos(parser) = static_cast<int32_t>(pos);
                    if (lastTag == TT_NewParagraph && !paragraphStarted) {
                        continue;
                    }
                    position = static_cast<int32_t>(pos);
                    return true;
                }
                if (deusextextparser::bParagraphStarted(parser)) {
                    if (c == '\r' || c == '\n') {
                        c = ' ';
                    }
                    value.push_back(c);
                    lastTag = TT_Text;
                    gotText = true;
                }
                pos++;
            }
            lastText = value;
            position = static_cast<int32_t>(pos);
            return true;
        }

        void nativeOpenText(UObject* object, Frame& stack, void* result)
        {
            Name textName = vm::get<Name>(stack);
            std::string packageName;
            optional(stack, packageName);
            vm::finish(stack);
            UTextParser* parser = parserSelf(object);
            if (!parser) {
                vm::retBool(result, false);
                return;
            }
            parser->textObject = nullptr;
            deusextextparser::textPos(parser) = 0;
            deusextextparser::tagEndPos(parser) = 0;
            deusextextparser::bParagraphStarted(parser) = 0;
            if (packageName.empty()) {
                packageName = "DeusExText";
            }
            Linker* linker = objects::loadPackage(Name(packageName));
            if (linker && !textName.none()) {
                int32_t ref = linker->findExport(textName, Name("ExtString"), "*");
                if (ref != 0) {
                    parser->textObject = linker->object(ref);
                    objects::ensureLoaded(parser->textObject);
                }
            }
            deusextextparser::Text(parser) = parser->textObject ? 1 : 0;
            vm::retBool(result, parser->textObject != nullptr);
        }

        void nativeCloseText(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (UTextParser* parser = parserSelf(object)) {
                parser->textObject = nullptr;
                deusextextparser::Text(parser) = 0;
                deusextextparser::textPos(parser) = 0;
                deusextextparser::tagEndPos(parser) = 0;
            }
        }

        void nativeProcessText(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UTextParser* parser = parserSelf(object);
            vm::retBool(result, parser && parser->textObject && processText(parser));
        }

        void nativeIsEOF(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UTextParser* parser = parserSelf(object);
            if (!parser || !parser->textObject) {
                vm::retBool(result, true);
                return;
            }
            const std::string& text = parserText(parser);
            size_t pos = static_cast<size_t>(std::max(deusextextparser::textPos(parser), 0));
            while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
                pos++;
            }
            vm::retBool(result, pos >= text.size());
        }

        void nativeGetText(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UTextParser* parser = parserSelf(object);
            vm::ret<std::string>(result, parser ? deusextextparser::lastText(parser) : std::string());
        }

        void nativeGotoLabel(UObject* object, Frame& stack, void*)
        {
            std::string label = vm::get<std::string>(stack);
            vm::finish(stack);
            UTextParser* parser = parserSelf(object);
            if (!parser) {
                return;
            }
            const std::string& text = parserText(parser);
            std::string upperText = text;
            for (char& c : upperText) {
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            }
            std::string needle = "<LABEL=" + label + ">";
            for (char& c : needle) {
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            }
            size_t pos = upperText.find(needle);
            if (pos != std::string::npos) {
                deusextextparser::textPos(parser) = static_cast<int32_t>(pos + needle.size());
            }
        }

        void nativeGetTag(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UTextParser* parser = parserSelf(object);
            vm::ret<uint8_t>(result, parser ? deusextextparser::lastTag(parser) : TT_None);
        }

        void nativeGetName(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UTextParser* parser = parserSelf(object);
            vm::ret<Name>(result, parser ? deusextextparser::lastName(parser) : Name());
        }

        void nativeGetColor(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UTextParser* parser = parserSelf(object);
            Color color { 255, 255, 255, 255 };
            if (parser) {
                uint8_t tag = deusextextparser::lastTag(parser);
                if (tag == TT_TextColor || tag == TT_DefaultColor) {
                    color = deusextextparser::lastColor(parser);
                } else {
                    Color fallback = deusextextparser::DefaultColor(parser);
                    if (fallback.r || fallback.g || fallback.b) {
                        color = fallback;
                    }
                }
            }
            vm::ret<Color>(result, color);
        }

        void nativeGetEmailInfo(UObject* object, Frame& stack, void*)
        {
            std::string* name = vm::getRef<std::string>(stack);
            std::string* subject = vm::getRef<std::string>(stack);
            std::string* from = vm::getRef<std::string>(stack);
            std::string* to = vm::getRef<std::string>(stack);
            std::string* cc = vm::getRef<std::string>(stack);
            vm::finish(stack);
            UTextParser* parser = parserSelf(object);
            bool valid = parser && deusextextparser::lastTag(parser) == TT_Email;
            *name = valid ? deusextextparser::lastEmailName(parser) : std::string();
            *subject = valid ? deusextextparser::lastEmailSubject(parser) : std::string();
            *from = valid ? deusextextparser::lastEmailFrom(parser) : std::string();
            *to = valid ? deusextextparser::lastEmailTo(parser) : std::string();
            *cc = valid ? deusextextparser::lastEmailCC(parser) : std::string();
        }

        void nativeGetFileInfo(UObject* object, Frame& stack, void*)
        {
            std::string* name = vm::getRef<std::string>(stack);
            std::string* description = vm::getRef<std::string>(stack);
            vm::finish(stack);
            UTextParser* parser = parserSelf(object);
            bool valid = parser && deusextextparser::lastTag(parser) == TT_File;
            *name = valid ? deusextextparser::lastFileName(parser) : std::string();
            *description = valid ? deusextextparser::lastFileDescription(parser) : std::string();
        }

        void nativeSetPlayerName(UObject* object, Frame& stack, void*)
        {
            std::string name = vm::get<std::string>(stack);
            vm::finish(stack);
            if (UTextParser* parser = parserSelf(object)) {
                deusextextparser::PlayerName(parser) = name;
                size_t space = name.find(' ');
                deusextextparser::playerFirstName(parser) = space == std::string::npos ? name : name.substr(0, space);
            }
        }
    }

    void registerConSysFactories()
    {
        objects::registerFactory(Name("ConAudioList"), [] { return new UConAudioList(); });
        objects::registerFactory(Name("ConEventRandomLabel"), [] { return new UConEventRandomLabel(); });
        objects::registerFactory(Name("DeusExTextParser"), [] { return new UTextParser(); });
    }

    void registerConSysNatives()
    {
        vm::registerNative(2050, nativeCreateFlagRef);
        vm::registerNative(2051, nativeCreateConCamera);
        vm::registerNative(2052, nativeBindEvents);
        vm::registerNative(2053, nativeBindActorEvents);
        vm::registerNative(2054, nativeIndex2054);
        vm::registerNative(2055, nativeGetSpeechAudio);
        vm::registerNative(2056, nativeGetSpeechLength);
        vm::registerNative(2060, nativeGetLabelCount);
        vm::registerNative(2061, nativeGetLabel);
        vm::registerNative(2062, nativeGetRandomLabel);
    }

    void registerTextParserNatives()
    {
        vm::registerNative(2210, nativeOpenText);
        vm::registerNative(2211, nativeCloseText);
        vm::registerNative(2212, nativeProcessText);
        vm::registerNative(2213, nativeIsEOF);
        vm::registerNative(2214, nativeGetText);
        vm::registerNative(2215, nativeGotoLabel);
        vm::registerNative(2216, nativeGetTag);
        vm::registerNative(2217, nativeGetName);
        vm::registerNative(2218, nativeGetColor);
        vm::registerNative(2219, nativeGetEmailInfo);
        vm::registerNative(2220, nativeGetFileInfo);
        vm::registerNative(2221, nativeSetPlayerName);
    }
}
