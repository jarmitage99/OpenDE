#include "core/config.h"
#include "core/objects.h"
#include "core/vm.h"
#include "editor/script_workspace.h"
#include "engine/package.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {
    int32_t answer(dx::UObject* object)
    {
        dx::vm::Call call(object, dx::Name("Answer"));
        REQUIRE(call.valid());
        call.run();
        int32_t value = 0;
        std::memcpy(&value, call.returnValue(), sizeof(value));
        return value;
    }

    int32_t& intAt(dx::UObject* object, const char* name)
    {
        dx::UProperty* property = object->cls->findProperty(dx::Name(name));
        REQUIRE(property != nullptr);
        return *reinterpret_cast<int32_t*>(object->data + property->offset);
    }

    std::string source(const std::string& className, const std::string& members)
    {
        return "class " + className + " expands Object;\r\n\r\n" + members;
    }

    void start()
    {
        dx::objects::setEditorMode(true);
        dx::engine::initialize(DEUSEX_GAME_DATA);
        REQUIRE(dx::objects::objectClass()->scriptText != nullptr);
    }

    std::string readFile(const std::filesystem::path& path)
    {
        std::ifstream in(path, std::ios::binary);
        std::stringstream text;
        text << in.rdbuf();
        return text.str();
    }
}

TEST_CASE("editor recompiles a loaded class in place keeping defaults and instance values", "[.][editor-scripts][data]")
{
    start();
    dx::editor::ScriptWorkspace workspace;
    dx::UClass* base = workspace.newClass("EdTestScripts", "EdTestKeep", dx::objects::objectClass());
    REQUIRE(base != nullptr);
    REQUIRE(workspace.compile({ { base, source("EdTestKeep", "var int Kept;\r\n\r\nfunction int Answer()\r\n{\r\n\treturn 1;\r\n}\r\n") } }, false));
    dx::UObject* instance = dx::objects::newObject(base, dx::objects::packageObject(dx::Name("EdTestScripts")), dx::Name(), 0);
    REQUIRE(answer(instance) == 1);
    intAt(base->defaultObject, "Kept") = 7;
    intAt(instance, "Kept") = 42;
    size_t oldOffset = base->findProperty(dx::Name("Kept"))->offset;

    REQUIRE(workspace.compile({ { base, source("EdTestKeep", "var int Before;\r\nvar int Kept;\r\n\r\nfunction int Answer()\r\n{\r\n\treturn 2;\r\n}\r\n") } }, false));

    REQUIRE(base->findProperty(dx::Name("Kept"))->offset == oldOffset + 4);
    REQUIRE(answer(instance) == 2);
    REQUIRE(intAt(instance, "Kept") == 42);
    REQUIRE(intAt(instance, "Before") == 0);
    REQUIRE(intAt(base->defaultObject, "Kept") == 7);
    REQUIRE(workspace.changed().count(base) == 1);
}

TEST_CASE("editor recompiles subclasses when their parent changes", "[.][editor-scripts][data]")
{
    start();
    dx::editor::ScriptWorkspace workspace;
    dx::UClass* base = workspace.newClass("EdTestScripts", "EdTestParent", dx::objects::objectClass());
    REQUIRE(base != nullptr);
    REQUIRE(workspace.compile({ { base, source("EdTestParent", "var int First;\r\n\r\nfunction int Answer()\r\n{\r\n\treturn 10;\r\n}\r\n") } }, false));
    dx::UClass* child = workspace.newClass("EdTestScripts", "EdTestChild", base);
    REQUIRE(child != nullptr);
    std::string childText = "class EdTestChild expands EdTestParent;\r\n\r\nvar int Own;\r\n\r\nfunction int Answer()\r\n{\r\n\treturn Super.Answer() + Own;\r\n}\r\n";
    REQUIRE(workspace.compile({ { child, childText } }, false));
    dx::UObject* instance = dx::objects::newObject(child, dx::objects::packageObject(dx::Name("EdTestScripts")), dx::Name(), 0);
    intAt(instance, "Own") = 5;
    REQUIRE(answer(instance) == 15);

    REQUIRE(workspace.compile({ { base, source("EdTestParent", "var int Zero;\r\nvar int First;\r\n\r\nfunction int Answer()\r\n{\r\n\treturn 20;\r\n}\r\n") } }, false));

    REQUIRE(child->findProperty(dx::Name("Own"))->offset == base->size);
    REQUIRE(intAt(instance, "Own") == 5);
    REQUIRE(answer(instance) == 25);
}

TEST_CASE("a script error restores the classes and reports the line", "[.][editor-scripts][data]")
{
    start();
    dx::editor::ScriptWorkspace workspace;
    dx::UClass* base = workspace.newClass("EdTestScripts", "EdTestBroken", dx::objects::objectClass());
    REQUIRE(base != nullptr);
    REQUIRE(workspace.compile({ { base, source("EdTestBroken", "function int Answer()\r\n{\r\n\treturn 3;\r\n}\r\n") } }, false));
    dx::UObject* instance = dx::objects::newObject(base, dx::objects::packageObject(dx::Name("EdTestScripts")), dx::Name(), 0);

    REQUIRE_FALSE(workspace.compile({ { base, source("EdTestBroken", "function int Answer()\r\n{\r\n\treturn Missing;\r\n}\r\n") } }, false));

    bool located = false;
    for (const dx::editor::CompileMessage& message : workspace.messages()) {
        if (message.error && message.cls == base) {
            REQUIRE(message.line == 5);
            REQUIRE(message.text == "EdTestBroken(5) : Error, Bad or missing expression in 'Return'");
            located = true;
        }
    }
    REQUIRE(located);
    REQUIRE((base->classFlags & (dx::CLASS_Parsed | dx::CLASS_Compiled)) == (dx::CLASS_Parsed | dx::CLASS_Compiled));
    REQUIRE(answer(instance) == 3);
    REQUIRE(base->scriptText->text.find("return 3;") != std::string::npos);
}

TEST_CASE("editor exports changed scripts and saves a package ucc can read", "[.][editor-scripts][data]")
{
    start();
    dx::editor::ScriptWorkspace workspace;
    dx::UClass* base = workspace.newClass("EdTestSaved", "EdTestStored", dx::objects::objectClass());
    REQUIRE(base != nullptr);
    REQUIRE(workspace.compile({ { base, source("EdTestStored", "var int Stored;\r\n\r\nfunction int Answer()\r\n{\r\n\treturn 4;\r\n}\r\n") } }, false));
    intAt(base->defaultObject, "Stored") = 9;

    REQUIRE(workspace.exportScripts(true) == 1);
    std::filesystem::path exported = dx::config::userDirectory() / "EdTestSaved" / "Classes" / "EdTestStored.uc";
    std::string text = readFile(exported);
    REQUIRE(text.find("class EdTestStored expands Object;") != std::string::npos);
    REQUIRE(text.find("Stored=9") != std::string::npos);
    REQUIRE(workspace.changed().empty());

    REQUIRE(workspace.savePackage("EdTestSaved"));
    std::filesystem::path saved = dx::config::userDirectory() / "System" / "EdTestSaved.u";
    dx::Package package(saved);
    bool hasClass = false;
    for (size_t i = 0; i < package.exports().size(); i++) {
        if (package.classOf(static_cast<int32_t>(i + 1)) == "Class" && package.nameAt(package.exports()[i].name) == "EdTestStored") {
            hasClass = true;
        }
    }
    REQUIRE(hasClass);
    std::filesystem::path out = dx::config::userDirectory() / "reexport";
    std::string command = std::string("\"") + UCC_PATH + "\" batchexport EdTestSaved.u class uc \"" + out.string() + "\" > /dev/null";
    REQUIRE(std::system(command.c_str()) == 0);
    std::string reexported = readFile(out / "EdTestStored.uc");
    REQUIRE(reexported.find("return 4;") != std::string::npos);
    REQUIRE(reexported.find("Stored=9") != std::string::npos);
}

TEST_CASE("editor refuses to save packages holding non-script objects", "[.][editor-scripts][data]")
{
    start();
    dx::editor::ScriptWorkspace workspace;
    REQUIRE_FALSE(workspace.savePackage("Engine"));
    REQUIRE(workspace.messages().back().text.find("is not script content") != std::string::npos);
    REQUIRE_FALSE(std::filesystem::exists(dx::config::userDirectory() / "System" / "Engine.u"));
}
