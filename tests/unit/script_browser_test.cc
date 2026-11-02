#include "core/config.h"
#include "core/objects.h"
#include "core/vm.h"
#include "editor/script_workspace.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {
    void begin()
    {
        dx::objects::setEditorMode(true);
        dx::engine::initialize(DEUSEX_GAME_DATA);
        REQUIRE(dx::objects::objectClass()->scriptText != nullptr);
    }

    int32_t callAnswer(dx::UObject* object)
    {
        dx::vm::Call call(object, dx::Name("Answer"));
        REQUIRE(call.valid());
        call.run();
        int32_t value = 0;
        std::memcpy(&value, call.returnValue(), sizeof(value));
        return value;
    }

    int32_t intValue(dx::UObject* object, const char* name)
    {
        dx::UProperty* property = object->cls->findProperty(dx::Name(name));
        REQUIRE(property != nullptr);
        return *reinterpret_cast<int32_t*>(object->data + property->offset);
    }

    void writeText(const std::filesystem::path& path, const std::string& text)
    {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream out(path, std::ios::binary);
        out << text;
    }

    std::string readText(const std::filesystem::path& path)
    {
        std::ifstream in(path, std::ios::binary);
        std::stringstream text;
        text << in.rdbuf();
        return text.str();
    }

    std::string loadable(int answer, int loaded)
    {
        return "class EdTestLoad expands Object;\r\n\r\nvar int Loaded;\r\n\r\nfunction int Answer()\r\n{\r\n\treturn " + std::to_string(answer) + ";\r\n}\r\n\r\ndefaultproperties\r\n{\r\n     Loaded=" + std::to_string(loaded) + "\r\n}\r\n";
    }

    bool hasLiveClass(const std::string& name)
    {
        for (dx::UObject* object : dx::objects::all()) {
            dx::UClass* cls = dx::cast<dx::UClass>(object);
            if (cls && !cls->destroyed && cls->name.str() == name) {
                return true;
            }
        }
        return false;
    }
}

TEST_CASE("class browser loads a class from a .uc file and reloads it in place", "[.][editor-scripts][data]")
{
    begin();
    dx::editor::ScriptWorkspace workspace;
    std::filesystem::path file = dx::config::userDirectory() / "load-test" / "EdTestLoaded" / "Classes" / "EdTestLoad.uc";
    writeText(file, loadable(6, 11));
    dx::UClass* cls = workspace.loadClass(file);
    REQUIRE(cls != nullptr);
    CHECK(cls->outer->name.str() == "EdTestLoaded");
    CHECK(intValue(cls->defaultObject, "Loaded") == 11);
    dx::UObject* instance = dx::objects::newObject(cls, cls->outer, dx::Name(), 0);
    CHECK(callAnswer(instance) == 6);
    CHECK(workspace.changed().count(cls) == 1);

    writeText(file, loadable(8, 12));
    REQUIRE(workspace.loadClass(file) == cls);
    CHECK(callAnswer(instance) == 8);
    CHECK(intValue(cls->defaultObject, "Loaded") == 12);
}

TEST_CASE("loading a class whose parent does not exist reports it and creates nothing", "[.][editor-scripts][data]")
{
    begin();
    dx::editor::ScriptWorkspace workspace;
    std::filesystem::path file = dx::config::userDirectory() / "load-test" / "EdTestLoaded" / "Classes" / "EdTestOrphan.uc";
    writeText(file, "class EdTestOrphan expands EdTestNoSuchParent;\r\n\r\nvar int Value;\r\n");
    CHECK(workspace.loadClass(file) == nullptr);
    bool reported = false;
    for (const dx::editor::CompileMessage& message : workspace.messages()) {
        if (message.error && message.text == "Error, superclass EdTestNoSuchParent of class EdTestOrphan not found") {
            reported = true;
        }
    }
    CHECK(reported);
    CHECK_FALSE(hasLiveClass("EdTestOrphan"));
    CHECK_FALSE(hasLiveClass("EdTestNoSuchParent"));
}

TEST_CASE("class defaults are edited and exported with a single class", "[.][editor-scripts][data]")
{
    begin();
    dx::editor::ScriptWorkspace workspace;
    dx::UClass* cls = workspace.newClass("EdTestDefaults", "EdTestCounted", dx::objects::objectClass());
    REQUIRE(cls != nullptr);
    REQUIRE(workspace.compile({ { cls, "class EdTestCounted expands Object;\r\n\r\nvar int Count;\r\nvar int Totals[3];\r\n" } }, false));
    REQUIRE(workspace.exportClass(cls));
    CHECK(workspace.changed().count(cls) == 0);

    CHECK(workspace.setDefault(cls, "Count", "5"));
    CHECK(workspace.setDefault(cls, "Totals[2]", "9"));
    CHECK(intValue(cls->defaultObject, "Count") == 5);
    CHECK(workspace.changed().count(cls) == 1);
    CHECK_FALSE(workspace.setDefault(cls, "Missing", "1"));
    CHECK(workspace.messages().back().text == "Error, EdTestCounted has no property Missing");
    CHECK_FALSE(workspace.setDefault(cls, "Totals[3]", "1"));

    REQUIRE(workspace.exportClass(cls));
    std::string text = readText(dx::config::userDirectory() / "EdTestDefaults" / "Classes" / "EdTestCounted.uc");
    CHECK(text.find("class EdTestCounted expands Object;") != std::string::npos);
    CHECK(text.find("     Count=5\r\n") != std::string::npos);
    CHECK(text.find("     Totals(2)=9\r\n") != std::string::npos);
    CHECK(workspace.changed().count(cls) == 0);
}

TEST_CASE("deleting a class is refused while it has subclasses or instances", "[.][editor-scripts][data]")
{
    begin();
    dx::editor::ScriptWorkspace workspace;
    dx::UClass* parent = workspace.newClass("EdTestDelete", "EdTestDoomedParent", dx::objects::objectClass());
    REQUIRE(parent != nullptr);
    dx::UClass* child = workspace.newClass("EdTestDelete", "EdTestDoomedChild", parent);
    REQUIRE(child != nullptr);

    CHECK_FALSE(workspace.deleteClass(parent));
    CHECK(workspace.messages().back().text == "Error, cannot delete EdTestDoomedParent: it has subclass EdTestDoomedChild");
    dx::UObject* instance = dx::objects::newObject(child, child->outer, dx::Name("EdTestDoomedInstance"), 0);
    CHECK_FALSE(workspace.deleteClass(child));
    CHECK(workspace.messages().back().text == "Error, cannot delete EdTestDoomedChild: it is used by EdTestDelete.EdTestDoomedInstance");
    CHECK_FALSE(workspace.deleteClass(dx::objects::findClass("Engine.Actor")));

    instance->destroyed = true;
    REQUIRE(workspace.deleteClass(child));
    CHECK(child->destroyed);
    REQUIRE(workspace.deleteClass(parent));
    CHECK_FALSE(hasLiveClass("EdTestDoomedParent"));
    CHECK_FALSE(hasLiveClass("EdTestDoomedChild"));
}
