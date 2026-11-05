#include "compiler/con_file.h"
#include "compiler/script_package.h"
#include "core/config.h"
#include "core/objects.h"
#include "core/vm.h"
#include "deusex/deusex.h"
#include "engine/package.h"
#include "engine/resources.h"
#include "engine/world.h"
#include "extension/consys.h"

#include <catch_amalgamated.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {
    using dx::compiler::ConEvent;
    using dx::compiler::ConEventType;
    using dx::compiler::ConFile;

    std::filesystem::path scratch(const std::string& name)
    {
        std::filesystem::path directory = dx::config::userDirectory() / "conedit-test" / name;
        std::filesystem::remove_all(directory);
        std::filesystem::create_directories(directory);
        return directory;
    }

    std::vector<uint8_t> readBytes(const std::filesystem::path& file)
    {
        std::ifstream in(file, std::ios::binary);
        return std::vector<uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }

    template <typename T>
    T& field(dx::UObject* object, const char* name)
    {
        uint8_t* address = object->propertyAddress(dx::Name(name));
        REQUIRE(address != nullptr);
        return *reinterpret_cast<T*>(address);
    }

    std::string text(dx::UObject* object, const char* name)
    {
        return field<std::string>(object, name);
    }

    std::string nameText(dx::UObject* object, const char* name)
    {
        dx::Name value = dx::Name::fromIndex(field<uint32_t>(object, name));
        return value.none() ? std::string() : value.str();
    }

    bool flag(dx::UObject* object, const char* name)
    {
        return field<uint32_t>(object, name) != 0;
    }

    dx::UObject* reference(dx::UObject* object, const char* name)
    {
        return field<dx::UObject*>(object, name);
    }

    dx::UObject* findConversation(dx::UObject* package, const std::string& conName)
    {
        dx::UClass* cls = dx::objects::findClass("ConSys.Conversation");
        for (dx::UObject* object : dx::compiler::packageContents(package)) {
            if (object->cls == cls && nameText(object, "conName") == conName) {
                return object;
            }
        }
        return nullptr;
    }

    std::vector<dx::UObject*> events(dx::UObject* conversation)
    {
        std::vector<dx::UObject*> result;
        for (dx::UObject* event = reference(conversation, "eventList"); event; event = reference(event, "nextEvent")) {
            result.push_back(event);
        }
        return result;
    }

    std::vector<dx::UObject*> chain(dx::UObject* head, const char* next)
    {
        std::vector<dx::UObject*> result;
        for (dx::UObject* item = head; item; item = reference(item, next)) {
            result.push_back(item);
        }
        return result;
    }

    ConEvent labelled(ConEventType type, const std::string& label)
    {
        ConEvent event = dx::compiler::makeConEvent(type);
        event.label = label;
        return event;
    }

    ConEvent speech(const std::string& speaker, const std::string& line, const std::string& label)
    {
        ConEvent event = labelled(ConEventType::Speech, label);
        event.speaker = speaker;
        event.speakingTo = "JCDenton";
        event.speech = line;
        return event;
    }

    dx::compiler::ConFlagRef flagRef(const std::string& name, bool value)
    {
        dx::compiler::ConFlagRef ref;
        ref.name = name;
        ref.value = value;
        return ref;
    }

    dx::compiler::ConChoice choice(const std::string& line, const std::string& label, bool asSpeech)
    {
        dx::compiler::ConChoice result;
        result.text = line;
        result.label = label;
        result.displayAsSpeech = asSpeech;
        return result;
    }
}

TEST_CASE("conversation labels rename across every reference ignoring case", "[conedit]")
{
    ConFile file;
    dx::compiler::ConConversation& conversation = dx::compiler::addConversation(file, "Talk", "Paul");
    ConEvent check = labelled(ConEventType::CheckFlag, "");
    check.setLabel = "bye";
    ConEvent pick = labelled(ConEventType::Choice, "");
    pick.choices = { choice("Leave", "Bye", false), choice("Stay", "Stay", false) };
    ConEvent jump = labelled(ConEventType::Jump, "");
    jump.jumpLabel = "BYE";
    dx::compiler::insertConEvent(conversation, 0, check);
    dx::compiler::insertConEvent(conversation, 1, pick);
    dx::compiler::insertConEvent(conversation, 2, jump);
    dx::compiler::insertConEvent(conversation, 3, speech("Paul", "Stay then.", "Stay"));
    conversation.events.back().label = "Bye";

    CHECK(dx::compiler::renameConLabel(conversation, "Bye", "Farewell") == 4);
    CHECK(conversation.events[0].setLabel == "Farewell");
    CHECK(conversation.events[1].choices[0].label == "Farewell");
    CHECK(conversation.events[1].choices[1].label == "Stay");
    CHECK(conversation.events[2].jumpLabel == "Farewell");
    CHECK(conversation.events[4].label == "Farewell");
    CHECK(dx::compiler::findConLabel(conversation, "farewell") == 4);
    CHECK(dx::compiler::checkConFile(file).empty());
}

TEST_CASE("conversation events insert with fresh ids and reorder in place", "[conedit]")
{
    ConFile file;
    dx::compiler::addConversation(file, "Talk", "Paul");
    dx::compiler::addConversation(file, "Other", "Paul");
    CHECK(file.conversations[1].id == 1);
    dx::compiler::ConConversation& conversation = file.conversations[0];
    REQUIRE(conversation.events.size() == 1);
    CHECK(conversation.events[0].type == ConEventType::End);
    dx::compiler::insertConEvent(conversation, 0, speech("Paul", "One", "A"));
    dx::compiler::insertConEvent(conversation, 1, speech("Paul", "Two", "B"));
    REQUIRE(conversation.events.size() == 3);
    CHECK(conversation.events[0].id == 1);
    CHECK(conversation.events[1].id == 2);
    CHECK(dx::compiler::moveConEvent(conversation, 1, 0));
    CHECK(conversation.events[0].speech == "Two");
    CHECK(conversation.events[1].speech == "One");
    CHECK_FALSE(dx::compiler::moveConEvent(conversation, 3, 0));
    dx::compiler::removeConEvent(conversation, 0);
    REQUIRE(conversation.events.size() == 2);
    CHECK(conversation.events[0].speech == "One");
}

TEST_CASE("conversation checks report missing and repeated labels and unknown conversations", "[conedit]")
{
    ConFile file;
    dx::compiler::ConConversation& conversation = dx::compiler::addConversation(file, "Talk", "Paul");
    ConEvent jump = labelled(ConEventType::Jump, "Twice");
    jump.jumpLabel = "Nowhere";
    jump.conId = 0;
    ConEvent away = labelled(ConEventType::Jump, "twice");
    away.jumpLabel = "Start";
    away.conId = 7;
    dx::compiler::insertConEvent(conversation, 0, jump);
    dx::compiler::insertConEvent(conversation, 1, away);
    std::vector<std::string> issues = dx::compiler::checkConFile(file);
    REQUIRE(issues.size() == 3);
    CHECK(issues[0] == "Talk: event 1 repeats label twice");
    CHECK(issues[1] == "Talk: event 0 jumps to missing label Nowhere");
    CHECK(issues[2] == "Talk: event 1 jumps to missing conversation 7");
}

TEST_CASE("conversation preview follows flags, choices, persona checks and jumps", "[conedit]")
{
    ConFile file;
    dx::compiler::ConConversation& conversation = dx::compiler::addConversation(file, "Talk", "Paul");
    conversation.events.clear();
    ConEvent check = labelled(ConEventType::CheckFlag, "");
    check.flags = { flagRef("Met", true) };
    check.setLabel = "Again";
    ConEvent set = labelled(ConEventType::SetFlag, "");
    set.flags = { flagRef("Met", true) };
    ConEvent pick = labelled(ConEventType::Choice, "");
    pick.choices = { choice("Yes", "Yes", true), choice("No", "No", true) };
    pick.choices[1].flags = { flagRef("Met", false) };
    ConEvent pay = labelled(ConEventType::AddCredits, "Yes");
    pay.credits = 30;
    ConEvent rich = labelled(ConEventType::CheckPersona, "");
    rich.personaType = 0;
    rich.condition = 3;
    rich.personaValue = 30;
    rich.jumpLabel = "Rich";
    conversation.events = { check, speech("Paul", "Hello", ""), set, pick, pay, rich, labelled(ConEventType::End, ""), speech("Paul", "Rich", "Rich"), labelled(ConEventType::End, ""), speech("Paul", "Again", "Again"), labelled(ConEventType::End, "") };

    dx::compiler::ConPreview preview;
    dx::compiler::startConPreview(file, 0, preview);
    std::vector<int32_t> visited = { preview.event };
    while (preview.event >= 0 && conversation.events[static_cast<size_t>(preview.event)].type != ConEventType::Choice) {
        REQUIRE(dx::compiler::advanceConPreview(file, preview, -1));
        visited.push_back(preview.event);
    }
    CHECK(visited == std::vector<int32_t> { 0, 1, 2, 3 });
    CHECK(preview.flags["Met"]);
    CHECK_FALSE(dx::compiler::conChoiceAvailable(conversation.events[3].choices[1], preview));
    CHECK_FALSE(dx::compiler::advanceConPreview(file, preview, 1));
    CHECK(preview.event == 3);
    REQUIRE(dx::compiler::advanceConPreview(file, preview, 0));
    CHECK(preview.event == 4);
    REQUIRE(dx::compiler::advanceConPreview(file, preview, -1));
    CHECK(preview.credits == 30);
    REQUIRE(dx::compiler::advanceConPreview(file, preview, -1));
    CHECK(preview.event == 7);
    REQUIRE(dx::compiler::advanceConPreview(file, preview, -1));
    REQUIRE(dx::compiler::advanceConPreview(file, preview, -1));
    CHECK(preview.event == -1);

    dx::compiler::startConPreview(file, 0, preview);
    REQUIRE(dx::compiler::advanceConPreview(file, preview, -1));
    CHECK(preview.event == 9);
}

TEST_CASE("conversation audio paths follow the importer's speaker and choice numbering", "[conedit]")
{
    ConFile file;
    dx::compiler::ConConversation& conversation = dx::compiler::addConversation(file, "MeetPaul", "PaulDenton");
    conversation.events = { speech("PaulDenton", "One", ""), speech("JCDenton", "Two", ""), labelled(ConEventType::Choice, ""), speech("PaulDenton", "Three", ""), labelled(ConEventType::Choice, "") };
    CHECK(dx::compiler::conSpeechAudioPath(conversation, 0) == "PaulDenton\\MeetPaul\\PaulDenton01.mp3");
    CHECK(dx::compiler::conSpeechAudioPath(conversation, 3) == "PaulDenton\\MeetPaul\\PaulDenton02.mp3");
    CHECK(dx::compiler::conSpeechAudioPath(conversation, 1) == "PaulDenton\\MeetPaul\\JCDenton01.mp3");
    CHECK(dx::compiler::conChoiceAudioPath(conversation, 4, 1) == "PaulDenton\\MeetPaul\\Choice02b.mp3");
    conversation.dataLink = true;
    CHECK(dx::compiler::conSpeechAudioPath(conversation, 0) == "InfoLink\\MeetPaul\\PaulDenton01.mp3");
}

TEST_CASE("conversation files keep comments in UTF-16 and reject truncated data", "[conedit]")
{
    ConFile file;
    dx::compiler::ConConversation& conversation = dx::compiler::addConversation(file, "Talk", "Paul");
    ConEvent comment = labelled(ConEventType::Comment, "");
    comment.comment = "caf\xc3\xa9 \xe2\x82\xac";
    dx::compiler::insertConEvent(conversation, 0, comment);
    std::vector<uint8_t> bytes = dx::compiler::writeConFile(file);
    const std::vector<uint8_t> units = { 'c', 0, 'a', 0, 'f', 0, 0xe9, 0, ' ', 0, 0xac, 0x20, 0, 0 };
    CHECK(std::search(bytes.begin(), bytes.end(), units.begin(), units.end()) != bytes.end());
    ConFile loaded;
    std::string error;
    REQUIRE(dx::compiler::readConFile(bytes, loaded, &error));
    REQUIRE(loaded.conversations.size() == 1);
    CHECK(loaded.conversations[0].events[0].comment == comment.comment);
    CHECK(loaded.actors.size() == 1);
    CHECK(loaded.actors[0].name == "Paul");
    bytes.resize(bytes.size() - 6);
    CHECK_FALSE(dx::compiler::readConFile(bytes, loaded, &error));
    CHECK(error == "truncated conversation file");
}

TEST_CASE("shipped conversations exported to .con round trip byte for byte through the editor model", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::setEditorMode(true);
    dx::objects::unloadPackage(dx::Name("DeusExConText"));
    std::filesystem::path root = scratch("roundtrip") / "DeusExConversations";
    std::vector<std::string> files = dx::compiler::packageConFiles("DeusExConversations");
    REQUIRE(files.size() == 35);
    CHECK(files.front() == "Mission0.Con");
    dx::Linker* textLinker = dx::objects::loadPackage(dx::Name("DeusExConText"));
    REQUIRE(textLinker != nullptr);
    std::map<int32_t, int32_t> types;
    for (const char* name : { "Intro.Con", "Mission0.Con", "Mission0_Infolink.Con", "NYC_Clinic_Shared.Con", "Mission2_Infolink.Con" }) {
        INFO(name);
        std::vector<std::string> log;
        REQUIRE(dx::compiler::exportPackageConFile("DeusExConversations", name, root, log));
        std::vector<uint8_t> original = readBytes(root / name);
        ConFile file;
        std::string error;
        REQUIRE(dx::compiler::readConFile(original, file, &error));
        REQUIRE_FALSE(file.conversations.empty());
        CHECK(dx::compiler::writeConFile(file) == original);
        for (const dx::compiler::ConConversation& conversation : file.conversations) {
            dx::UClass* cls = dx::objects::findClass("ConSys.Conversation");
            int32_t matches = 0;
            for (dx::UObject* shipped : dx::compiler::packageContents(textLinker->packageObject)) {
                if (shipped->cls != cls || nameText(shipped, "conName") != conversation.name) {
                    continue;
                }
                dx::UObject* first = reference(shipped, "eventList");
                bool same = text(shipped, "conOwnerName") == conversation.owner && text(shipped, "Description") == conversation.description && first && field<uint8_t>(first, "eventType") == static_cast<uint8_t>(conversation.events[0].type);
                matches += same ? 1 : 0;
            }
            INFO(conversation.name);
            CHECK(matches >= 1);
            for (const ConEvent& event : conversation.events) {
                types[static_cast<int32_t>(event.type)]++;
            }
        }
    }
    for (int32_t type : { 0, 1, 2, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18 }) {
        INFO(type);
        CHECK(types[type] > 0);
    }
    ConFile intro;
    REQUIRE(dx::compiler::loadConFile(root / "Intro.Con", intro, nullptr));
    CHECK(intro.audioPackage == "Intro");
    REQUIRE(intro.conversations.size() == 1);
    CHECK(intro.conversations[0].name == "Intro");
    dx::objects::unloadPackage(dx::Name("DeusExConText"));
    dx::objects::setEditorMode(false);
}

TEST_CASE("an edited shipped conversation recompiles into Conversation objects with the edits", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::setEditorMode(true);
    std::filesystem::path root = scratch("edit");
    std::vector<std::string> log;
    REQUIRE(dx::compiler::exportPackageConFile("DeusExConversations", "Mission0.Con", root / "DeusExConversations", log));
    ConFile file;
    REQUIRE(dx::compiler::loadConFile(root / "DeusExConversations" / "Mission0.Con", file, nullptr));
    size_t conversationIndex = 0;
    size_t speechIndex = 0;
    bool found = false;
    for (size_t c = 0; c < file.conversations.size() && !found; c++) {
        for (size_t e = 0; e < file.conversations[c].events.size() && !found; e++) {
            if (file.conversations[c].events[e].type == ConEventType::Speech) {
                conversationIndex = c;
                speechIndex = e;
                found = true;
            }
        }
    }
    REQUIRE(found);
    dx::compiler::ConConversation& conversation = file.conversations[conversationIndex];
    conversation.events[speechIndex].speech = "This line was edited in the conversation editor.";
    ConEvent pick = labelled(ConEventType::Choice, "");
    pick.choices = { choice("Tell me more.", "EditMore", true), choice("Goodbye.", "EditBye", false) };
    pick.choices[1].flags = { flagRef("ConEditUniqueFlagQ", true) };
    pick.choices[1].flags[0].expiration = 4;
    dx::compiler::insertConEvent(conversation, speechIndex + 1, pick);
    dx::compiler::insertConEvent(conversation, speechIndex + 2, speech(conversation.owner, "More then.", "EditMore"));
    dx::compiler::insertConEvent(conversation, speechIndex + 3, labelled(ConEventType::End, "EditBye"));
    std::filesystem::path mod = root / "ConEditMod";
    REQUIRE(dx::compiler::saveConFile(mod / "Edited.con", file, nullptr));

    log.clear();
    CHECK(dx::compiler::compileConFile(mod, "Edited.con", "ConEditMod", log) == static_cast<int32_t>(file.conversations.size()));
    CHECK(std::filesystem::exists(root / "System" / "ConEditModText.u"));
    dx::UObject* package = dx::objects::packageObject(dx::Name("ConEditModText"));
    dx::UObject* compiled = findConversation(package, conversation.name);
    REQUIRE(compiled != nullptr);
    std::vector<dx::UObject*> list = events(compiled);
    REQUIRE(list.size() > speechIndex + 6);
    dx::UObject* line = reference(list[speechIndex], "ConSpeech");
    REQUIRE(line != nullptr);
    CHECK(text(line, "speech") == "This line was edited in the conversation editor.");
    dx::UObject* choiceEvent = list[speechIndex + 1];
    CHECK(choiceEvent->cls->name.str() == "ConEventChoice");
    std::vector<dx::UObject*> choices = chain(reference(choiceEvent, "ChoiceList"), "nextChoice");
    REQUIRE(choices.size() == 2);
    CHECK(text(choices[0], "choiceText") == "Tell me more.");
    CHECK(text(choices[0], "choiceLabel") == "ChoiceSpeechLabel_0");
    CHECK(text(choices[1], "choiceText") == "Goodbye.");
    CHECK(text(choices[1], "choiceLabel") == "EditBye");
    dx::UObject* choiceFlag = reference(choices[1], "flagRef");
    REQUIRE(choiceFlag != nullptr);
    CHECK(nameText(choiceFlag, "FlagName") == "ConEditUniqueFlagQ");
    CHECK(field<int32_t>(choiceFlag, "expiration") == 4);
    dx::UObject* spoken = list[speechIndex + 2];
    CHECK(text(spoken, "Label") == "ChoiceSpeechLabel_0");
    CHECK(text(spoken, "speakerName") == "JCDenton");
    CHECK(text(reference(spoken, "ConSpeech"), "speech") == "Tell me more.");
    CHECK(text(list[speechIndex + 3], "jumpLabel") == "EditMore");
    CHECK(text(list[speechIndex + 4], "Label") == "EditMore");
    CHECK(text(reference(list[speechIndex + 4], "ConSpeech"), "speech") == "More then.");
    CHECK(list[speechIndex + 5]->cls->name.str() == "ConEventEnd");
    CHECK(text(list[speechIndex + 5], "Label") == "EditBye");
    dx::objects::setEditorMode(false);
}

TEST_CASE("ucc make compiles an authored .con with every event type into a conversation package", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    const char* xdg = std::getenv("XDG_DATA_HOME");
    REQUIRE(xdg != nullptr);
    std::filesystem::path home = std::filesystem::path(xdg) / "conedit-make";
    std::filesystem::remove_all(home);
    std::filesystem::path user = home / "deusex";
    std::filesystem::create_directories(user / "ModCons" / "Classes");
    {
        std::ifstream in(std::filesystem::path(DEUSEX_GAME_DATA) / "System" / "DeusEx.ini", std::ios::binary);
        std::stringstream ini;
        ini << in.rdbuf();
        std::string content = ini.str();
        size_t last = content.rfind("EditPackages=");
        REQUIRE(last != std::string::npos);
        content.insert(content.find('\n', last) + 1, "EditPackages=ModCons\r\n");
        std::ofstream out(user / "DeusEx.ini", std::ios::binary);
        out << content;
    }
    {
        std::ofstream out(user / "ModCons" / "Classes" / "ModCons.uc", std::ios::binary);
        out << "class ModCons expands Object\r\n\tabstract;\r\n\r\n#exec CONVERSATION IMPORT FILE=\"Conversations\\ModCons.con\"\r\n\r\ndefaultproperties\r\n{\r\n}\r\n";
    }

    ConFile file;
    file.audioPackage = "Mission90";
    file.missions = { 90 };
    dx::compiler::ConConversation& conversation = dx::compiler::addConversation(file, "ModGreeting", "ModGuy");
    conversation.description = "A greeting authored in the editor";
    conversation.invokeRadius = true;
    conversation.radius = 320;
    conversation.flags = { flagRef("ModReady", true) };
    conversation.events.clear();
    ConEvent pick = labelled(ConEventType::Choice, "");
    pick.clearScreen = true;
    pick.choices = { choice("Trade.", "Done", false), choice("Hack it.", "Done", false) };
    pick.choices[1].hasSkill = true;
    pick.choices[1].skill = "SkillComputer";
    pick.choices[1].skillLevel = 2;
    ConEvent set = labelled(ConEventType::SetFlag, "");
    set.flags = { flagRef("ModMet", true) };
    set.flags[0].expiration = 3;
    ConEvent check = labelled(ConEventType::CheckFlag, "");
    check.flags = { flagRef("ModMet", false) };
    check.setLabel = "Done";
    ConEvent has = labelled(ConEventType::CheckObject, "");
    has.objectName = "MedKit";
    has.failLabel = "Done";
    ConEvent give = labelled(ConEventType::TransferObject, "");
    give.objectName = "MedKit";
    give.transferCount = 2;
    give.fromName = "ModGuy";
    give.toName = "JCDenton";
    give.failLabel = "Done";
    ConEvent camera = labelled(ConEventType::MoveCamera, "");
    camera.cameraType = 1;
    camera.cameraPosition = 4;
    camera.cameraTransition = 1;
    camera.cameraRotation = { 100, 200, 300 };
    camera.heightModifier = 0.5f;
    camera.centerModifier = 0.25f;
    camera.distanceMultiplier = 2.0f;
    ConEvent anim = labelled(ConEventType::Animation, "");
    anim.ownerName = "ModGuy";
    anim.sequence = "Wave";
    anim.animMode = 1;
    ConEvent trade = labelled(ConEventType::Trade, "");
    trade.ownerName = "ModGuy";
    ConEvent jump = labelled(ConEventType::Jump, "");
    jump.jumpLabel = "Done";
    jump.conId = conversation.id;
    ConEvent random = labelled(ConEventType::RandomLabel, "");
    random.labels = { "Start", "Done" };
    random.cycleEvents = true;
    random.cycleOnce = true;
    ConEvent trigger = labelled(ConEventType::Trigger, "");
    trigger.triggerTag = "ModTrigger";
    ConEvent goal = labelled(ConEventType::AddGoal, "");
    goal.goalName = "ModGoal";
    goal.goalText = "Find the mod guy.";
    goal.primaryGoal = true;
    ConEvent finished = labelled(ConEventType::AddGoal, "");
    finished.goalName = "ModGoal";
    finished.goalCompleted = true;
    ConEvent note = labelled(ConEventType::AddNote, "");
    note.noteText = "The mod guy likes medkits.";
    ConEvent points = labelled(ConEventType::AddSkillPoints, "");
    points.points = 150;
    points.awardMessage = "Met the mod guy";
    ConEvent credits = labelled(ConEventType::AddCredits, "");
    credits.credits = 75;
    ConEvent persona = labelled(ConEventType::CheckPersona, "");
    persona.personaType = 2;
    persona.condition = 3;
    persona.personaValue = 40;
    persona.jumpLabel = "Done";
    ConEvent comment = labelled(ConEventType::Comment, "");
    comment.comment = "Authored for the ucc make test";
    conversation.events = { speech("ModGuy", "Hello there.", "Start"), pick, set, check, has, give, camera, anim, trade, jump, random, trigger, goal, finished, note, points, credits, persona, comment, labelled(ConEventType::End, "Done") };
    REQUIRE(dx::compiler::checkConFile(file).empty());
    REQUIRE(dx::compiler::saveConFile(user / "ModCons" / "Conversations" / "ModCons.con", file, nullptr));

    const std::vector<uint8_t> voice = { 0xff, 0xfb, 0x90, 0x44, 1, 2, 3, 4, 5, 6, 7, 8 };
    {
        std::filesystem::path audio = user / "ModCons" / "Audio" / "Mission90" / "ModGuy" / "ModGreeting";
        std::filesystem::create_directories(audio);
        std::ofstream out(audio / "ModGuy01.mp3", std::ios::binary);
        out.write(reinterpret_cast<const char*>(voice.data()), static_cast<std::streamsize>(voice.size()));
    }

    REQUIRE(std::system(("XDG_DATA_HOME=\"" + home.string() + "\" \"" UCC_PATH "\" make > \"" + (home / "make.log").string() + "\" 2>&1").c_str()) == 0);
    std::filesystem::path built = user / "System" / "ModConsText.u";
    REQUIRE(std::filesystem::exists(built));
    REQUIRE(std::filesystem::exists(user / "System" / "ModCons.u"));

    dx::Linker* linker = dx::objects::loadPackageFile(dx::Name("ModConsText"), built, false);
    REQUIRE(linker != nullptr);
    for (size_t i = 0; i < linker->file().exports().size(); i++) {
        dx::objects::ensureLoaded(linker->object(static_cast<int32_t>(i + 1)));
    }
    dx::objects::flushLoads();
    dx::UObject* compiled = findConversation(linker->packageObject, "ModGreeting");
    REQUIRE(compiled != nullptr);
    CHECK(text(compiled, "conOwnerName") == "ModGuy");
    CHECK(text(compiled, "Description") == "A greeting authored in the editor");
    CHECK(text(compiled, "audioPackageName") == "Mission90");
    CHECK(flag(compiled, "bInvokeRadius"));
    CHECK(field<int32_t>(compiled, "radiusDistance") == 320);
    REQUIRE(reference(compiled, "flagRefList") != nullptr);
    CHECK(nameText(reference(compiled, "flagRefList"), "FlagName") == "ModReady");

    std::vector<dx::UObject*> list = events(compiled);
    REQUIRE(list.size() == conversation.events.size());
    const char* classes[] = { "ConEventSpeech", "ConEventChoice", "ConEventSetFlag", "ConEventCheckFlag", "ConEventCheckObject", "ConEventTransferObject", "ConEventMoveCamera", "ConEventAnimation", "ConEventTrade", "ConEventJump", "ConEventRandomLabel", "ConEventTrigger", "ConEventAddGoal", "ConEventAddGoal", "ConEventAddNote", "ConEventAddSkillPoints", "ConEventAddCredits", "ConEventCheckPersona", "ConEventComment", "ConEventEnd" };
    for (size_t i = 0; i < list.size(); i++) {
        INFO(i);
        CHECK(list[i]->cls->name.str() == classes[i]);
    }
    CHECK(text(list[0], "Label") == "Start");
    CHECK(text(list[0], "speakerName") == "ModGuy");
    CHECK(text(list[0], "speakingToName") == "JCDenton");
    CHECK(text(reference(list[0], "ConSpeech"), "speech") == "Hello there.");
    CHECK(flag(list[1], "bClearScreen"));
    std::vector<dx::UObject*> choices = chain(reference(list[1], "ChoiceList"), "nextChoice");
    REQUIRE(choices.size() == 2);
    CHECK(text(choices[1], "choiceText") == "Hack it.");
    CHECK(field<int32_t>(choices[1], "skillLevelNeeded") == 2);
    CHECK(text(choices[1], "choiceLabel") == "Done");
    dx::UObject* setRef = reference(list[2], "flagRef");
    REQUIRE(setRef != nullptr);
    CHECK(nameText(setRef, "FlagName") == "ModMet");
    CHECK(flag(setRef, "Value"));
    CHECK(field<int32_t>(setRef, "expiration") == 3);
    CHECK_FALSE(flag(reference(list[3], "flagRef"), "Value"));
    CHECK(text(list[3], "setLabel") == "Done");
    CHECK(text(list[4], "ObjectName") == "MedKit");
    CHECK(text(list[4], "failLabel") == "Done");
    CHECK(field<int32_t>(list[5], "transferCount") == 2);
    CHECK(text(list[5], "fromName") == "ModGuy");
    CHECK(text(list[5], "toName") == "JCDenton");
    CHECK(field<uint8_t>(list[6], "cameraType") == 1);
    CHECK(field<uint8_t>(list[6], "cameraPosition") == 4);
    CHECK(field<uint8_t>(list[6], "cameraTransition") == 1);
    CHECK(field<float>(list[6], "heightModifier") == 0.5f);
    CHECK(field<float>(list[6], "distanceMultiplier") == 2.0f);
    CHECK(text(list[7], "eventOwnerName") == "ModGuy");
    CHECK(nameText(list[7], "Sequence") == "Wave");
    CHECK(flag(list[7], "bFinishAnim"));
    CHECK_FALSE(flag(list[7], "bLoopAnim"));
    CHECK(text(list[8], "eventOwnerName") == "ModGuy");
    CHECK(text(list[9], "jumpLabel") == "Done");
    CHECK(reference(list[9], "jumpCon") == compiled);
    auto* labels = dx::cast<dx::extension::UConEventRandomLabel>(list[10]);
    REQUIRE(labels != nullptr);
    CHECK(labels->labels == std::vector<std::string> { "Start", "Done" });
    CHECK(flag(list[10], "bCycleEvents"));
    CHECK(flag(list[10], "bCycleOnce"));
    CHECK_FALSE(flag(list[10], "bCycleRandom"));
    CHECK(nameText(list[11], "triggerTag") == "ModTrigger");
    CHECK(nameText(list[12], "goalName") == "ModGoal");
    CHECK(text(list[12], "goalText") == "Find the mod guy.");
    CHECK(flag(list[12], "bPrimaryGoal"));
    CHECK(flag(list[13], "bGoalCompleted"));
    CHECK(text(list[14], "noteText") == "The mod guy likes medkits.");
    CHECK(field<int32_t>(list[15], "pointsToAdd") == 150);
    CHECK(text(list[15], "awardMessage") == "Met the mod guy");
    CHECK(field<int32_t>(list[16], "creditsToAdd") == 75);
    CHECK(field<uint8_t>(list[17], "personaType") == 2);
    CHECK(field<uint8_t>(list[17], "condition") == 3);
    CHECK(field<int32_t>(list[17], "Value") == 40);
    CHECK(text(list[17], "jumpLabel") == "Done");
    CHECK(text(list[18], "commentText") == "Authored for the ucc make test");
    CHECK(text(list[19], "Label") == "Done");

    REQUIRE(dx::objects::loadPackageFile(dx::Name("ModConsAudioMission90"), user / "System" / "ModConsAudioMission90.u", false) != nullptr);
    dx::vm::Call call(compiled, dx::Name("GetSpeechAudio"));
    call.i(field<int32_t>(reference(list[0], "ConSpeech"), "soundID")).run();
    auto* sound = dx::cast<dx::USound>(*reinterpret_cast<dx::UObject**>(call.returnValue()));
    REQUIRE(sound != nullptr);
    CHECK(sound->bytes == voice);

    dx::World world;
    dx::UObject* level = dx::objects::packageObject(dx::Name("ConEditLevel"));
    dx::UObject* info = dx::objects::newObject(dx::objects::findClass("DeusEx.DeusExLevelInfo"), level, dx::Name(), 0);
    field<std::string>(info, "ConversationPackage") = "ModCons";
    field<int32_t>(info, "missionNumber") = 90;
    world.actors.push_back(info);
    dx::deusex::levelLoaded(world);
    dx::UObject* pawn = dx::objects::newObject(dx::objects::findClass("DeusEx.PaulDenton"), level, dx::Name(), 0);
    field<std::string>(pawn, "BindName") = "ModGuy";
    dx::vm::Call(pawn, dx::Name("ConBindEvents")).run();
    dx::UObject* bound = reference(pawn, "ConListItems");
    REQUIRE(bound != nullptr);
    CHECK(reference(bound, "con") == compiled);
    CHECK(reference(bound, "Next") == nullptr);
    std::filesystem::remove_all(home);
}

TEST_CASE("a conversation shared through BarkBindName counts every owner so the invoker is rebound", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::World world;
    dx::UObject* level = dx::objects::packageObject(dx::Name("ConBindLevel"));
    dx::UObject* info = dx::objects::newObject(dx::objects::findClass("DeusEx.DeusExLevelInfo"), level, dx::Name(), 0);
    field<std::string>(info, "ConversationPackage") = "DeusExConversations";
    field<int32_t>(info, "missionNumber") = 1;
    world.actors.push_back(info);
    dx::deusex::levelLoaded(world);
    dx::UClass* troopClass = dx::objects::findClass("DeusEx.UNATCOTroop");
    dx::UObject* generic = dx::objects::newObject(troopClass, level, dx::Name(), 0);
    field<std::string>(generic, "BindName") = "UNATCOTroop";
    dx::UObject* collins = dx::objects::newObject(troopClass, level, dx::Name(), 0);
    field<std::string>(collins, "BindName") = "CorporalCollins";
    field<std::string>(collins, "BarkBindName") = "UNATCOTroop";
    world.actors.push_back(generic);
    world.actors.push_back(collins);
    dx::vm::Call(generic, dx::Name("ConBindEvents")).run();
    dx::vm::Call(collins, dx::Name("ConBindEvents")).run();
    dx::UObject* shared = nullptr;
    dx::UObject* own = nullptr;
    for (dx::UObject* item = reference(collins, "ConListItems"); item; item = reference(item, "Next")) {
        dx::UObject* con = reference(item, "con");
        if (nameText(con, "conName") == "UNATCOTroopInitialBarks") {
            shared = con;
        }
        if (nameText(con, "conName") == "CollinsDockBarks") {
            own = con;
        }
    }
    REQUIRE(shared != nullptr);
    REQUIRE(own != nullptr);
    CHECK(field<int32_t>(own, "ownerRefCount") == 1);
    CHECK(field<int32_t>(shared, "ownerRefCount") == 2);
    dx::vm::Call bind(shared, dx::Name("BindEvents"));
    bind.skip();
    bind.obj(collins);
    bind.run();
    if (field<int32_t>(shared, "ownerRefCount") > 1) {
        dx::vm::Call(shared, dx::Name("BindActorEvents")).obj(collins).run();
    }
    dx::UObject* speech = reference(shared, "eventList");
    while (speech && !speech->isA(dx::Name("ConEventSpeech"))) {
        speech = reference(speech, "nextEvent");
    }
    REQUIRE(speech != nullptr);
    CHECK(reference(speech, "speaker") == collins);
}
