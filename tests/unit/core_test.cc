#include "core/config.h"
#include "core/objects.h"
#include "core/url.h"
#include "core/vm.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <string>
#include <vector>

namespace {
    void setEnvironment(const char* name, const char* value)
    {
#ifdef _WIN32
        _putenv_s(name, value);
#else
        if (*value) {
            setenv(name, value, 1);
        } else {
            unsetenv(name);
        }
#endif
    }

    void initEngine()
    {
        static bool initialized = false;
        if (!initialized) {
            dx::engine::initialize(DEUSEX_GAME_DATA);
            initialized = true;
        }
    }

    dx::UClass* testInfoClass()
    {
        dx::UClass* cls = dx::objects::findClass("Engine.TestInfo");
        dx::objects::flushLoads();
        REQUIRE(cls != nullptr);
        return cls;
    }

    dx::UFunction* coreFunction(const char* name)
    {
        dx::UFunction* function = dx::objects::objectClass()->findFunction(dx::Name(name));
        REQUIRE(function != nullptr);
        return function;
    }

    struct Bytes {
        std::vector<uint8_t> data;

        Bytes& op(std::initializer_list<uint8_t> values)
        {
            data.insert(data.end(), values);
            return *this;
        }

        Bytes& u16(uint16_t value)
        {
            data.push_back(static_cast<uint8_t>(value & 0xff));
            data.push_back(static_cast<uint8_t>(value >> 8));
            return *this;
        }

        Bytes& u32(uint32_t value)
        {
            for (int i = 0; i < 4; i++) {
                data.push_back(static_cast<uint8_t>(value >> (8 * i)));
            }
            return *this;
        }

        Bytes& f32(float value)
        {
            uint32_t bits;
            std::memcpy(&bits, &value, 4);
            return u32(bits);
        }

        Bytes& str(const char* text)
        {
            data.insert(data.end(), text, text + std::strlen(text) + 1);
            return *this;
        }

        Bytes& object(const dx::UObject* value)
        {
            return u32(value->handle);
        }

        Bytes& name(const char* text)
        {
            return u32(dx::Name(text).index());
        }
    };

    dx::UFunction* makeFunction(dx::UClass* owner, const char* name, const Bytes& code, uint32_t flags = 0, dx::UField* params = nullptr)
    {
        dx::UFunction* function = new dx::UFunction();
        function->name = dx::Name(name);
        function->outer = owner;
        function->functionFlags = flags;
        function->children = params;
        function->script = code.data;
        dx::objects::registerObject(function);
        function->link();
        return function;
    }

    dx::UProperty* makeParam(const char* name, uint32_t flags)
    {
        dx::UProperty* property = new dx::UProperty(dx::PropType::Int);
        property->name = dx::Name(name);
        property->propertyFlags = flags;
        dx::objects::registerObject(property);
        return property;
    }

    dx::UState* makeState(dx::UClass* owner, const char* name, const Bytes& code, uint16_t labelTable)
    {
        dx::UState* state = new dx::UState();
        state->name = dx::Name(name);
        state->outer = owner;
        state->script = code.data;
        state->labelTableOffset = labelTable;
        state->ignoreMask = ~0ull;
        dx::objects::registerObject(state);
        owner->states[state->name] = state;
        return state;
    }

    int32_t& xnum(dx::UObject* object)
    {
        uint8_t* address = object->propertyAddress(dx::Name("xnum"));
        REQUIRE(address != nullptr);
        return *reinterpret_cast<int32_t*>(address);
    }

    dx::UProperty* xnumProperty(dx::UClass* cls)
    {
        dx::UProperty* property = cls->findProperty(dx::Name("xnum"));
        REQUIRE(property != nullptr);
        return property;
    }

    Bytes incrementXnum(dx::UClass* cls)
    {
        Bytes code;
        code.op({ 0xA3, 0x01 }).object(xnumProperty(cls)).op({ 0x16 });
        return code;
    }

    template <typename T>
    T evaluate(dx::UObject* self, const Bytes& code)
    {
        dx::UStruct node;
        node.script = code.data;
        dx::Frame frame;
        frame.object = self;
        frame.node = &node;
        frame.code = node.script.data();
        T value {};
        frame.step(self, &value);
        return value;
    }
}

TEST_CASE("vector to string conversion prints six decimals like the original", "[data]")
{
    initEngine();
    Bytes code;
    code.op({ 0x58, 0x23 }).f32(1.5f).f32(-2.0f).f32(0.25f);
    CHECK(evaluate<std::string>(dx::objects::objectClass()->defaultObject, code) == "1.500000,-2.000000,0.250000");
}

TEST_CASE("string switch matches case labels case insensitively", "[data]")
{
    initEngine();
    Bytes code;
    code.op({ 0x05, 0x00, 0x1F }).str("abc");
    code.op({ 0x0A }).u16(20).op({ 0x1F }).str("ABC");
    code.op({ 0x0B, 0x0B, 0x0B, 0x0B, 0x0B });
    code.op({ 0x0A }).u16(0xffff);
    dx::UStruct node;
    node.script = code.data;
    dx::Frame frame;
    frame.object = dx::objects::objectClass()->defaultObject;
    frame.node = &node;
    frame.code = node.script.data();
    frame.step(frame.object, nullptr);
    CHECK(frame.code - node.script.data() == 15);
}

TEST_CASE("Mid clamps the start index as unsigned like FString Mid", "[data]")
{
    initEngine();
    {
        dx::vm::Call call(dx::objects::objectClass()->defaultObject, coreFunction("Mid"));
        call.s("abcdef").i(-1).i(3).run();
        CHECK(*reinterpret_cast<std::string*>(call.returnValue()) == "");
    }
    {
        dx::vm::Call call(dx::objects::objectClass()->defaultObject, coreFunction("Mid"));
        call.s("abcdef").i(4).i(10).run();
        CHECK(*reinterpret_cast<std::string*>(call.returnValue()) == "ef");
    }
}

TEST_CASE("MirrorVectorByNormal normalizes the normal first", "[data]")
{
    initEngine();
    dx::vm::Call call(dx::objects::objectClass()->defaultObject, coreFunction("MirrorVectorByNormal"));
    call.v({ 1, -1, 0 }).v({ 0, 2, 0 }).run();
    dx::Vector value = *reinterpret_cast<dx::Vector*>(call.returnValue());
    CHECK(value.x == 1.0f);
    CHECK(value.y == 1.0f);
    CHECK(value.z == 0.0f);
}

TEST_CASE("script calls to a disabled probe function are skipped", "[data]")
{
    initEngine();
    dx::UClass* cls = testInfoClass();
    dx::UObject* object = dx::objects::newObject(cls, nullptr, dx::Name(), 0);
    Bytes timerCode = incrementXnum(cls);
    timerCode.op({ 0x04, 0x0B });
    dx::UFunction* timer = makeFunction(cls, "Timer", timerCode);
    Bytes callerCode;
    callerCode.op({ 0x1C }).object(timer).op({ 0x16, 0x04, 0x0B });
    dx::UFunction* caller = makeFunction(cls, "CoreProbeCaller", callerCode);
    xnum(object) = 10;
    dx::vm::disable(object, dx::Name("Timer"));
    {
        dx::vm::Call call(object, caller);
        call.run();
    }
    CHECK(xnum(object) == 10);
    {
        dx::vm::Call call(object, timer);
        call.run();
    }
    CHECK(xnum(object) == 10);
    dx::vm::enable(object, dx::Name("Timer"));
    {
        dx::vm::Call call(object, caller);
        call.run();
    }
    CHECK(xnum(object) == 11);
}

TEST_CASE("singular functions do not reenter on the same object", "[data]")
{
    initEngine();
    dx::UClass* cls = testInfoClass();
    dx::UObject* object = dx::objects::newObject(cls, nullptr, dx::Name(), 0);
    dx::UFunction* recurse = makeFunction(cls, "CoreSingular", Bytes(), dx::FUNC_Singular);
    Bytes code = incrementXnum(cls);
    code.op({ 0x1C }).object(recurse).op({ 0x16, 0x04, 0x0B });
    recurse->script = code.data;
    std::vector<std::string> criticals;
    dx::vm::CriticalHandler previous = dx::vm::setCriticalHandler([&criticals](const std::string& message) {
        criticals.push_back(message);
    });
    xnum(object) = 0;
    {
        dx::vm::Call call(object, recurse);
        call.run();
    }
    CHECK(xnum(object) == 1);
    CHECK(criticals.empty());
    {
        dx::vm::Call call(object, recurse);
        call.run();
    }
    CHECK(xnum(object) == 2);
    dx::vm::setCriticalHandler(previous);
}

TEST_CASE("out parameters are copied back in reverse order", "[data]")
{
    initEngine();
    dx::UClass* cls = testInfoClass();
    dx::UObject* object = dx::objects::newObject(cls, nullptr, dx::Name(), 0);
    dx::UProperty* a = makeParam("A", dx::CPF_Parm | dx::CPF_OutParm);
    dx::UProperty* b = makeParam("B", dx::CPF_Parm | dx::CPF_OutParm);
    a->next = b;
    Bytes code;
    code.op({ 0x0F, 0x00 }).object(a).op({ 0x26 });
    code.op({ 0x0F, 0x00 }).object(b).op({ 0x2C, 0x02 });
    code.op({ 0x04, 0x0B });
    dx::UFunction* setBoth = makeFunction(cls, "CoreSetBoth", code, 0, a);
    Bytes callerCode;
    callerCode.op({ 0x1C }).object(setBoth).op({ 0x01 }).object(xnumProperty(cls)).op({ 0x01 }).object(xnumProperty(cls)).op({ 0x16, 0x04, 0x0B });
    dx::UFunction* caller = makeFunction(cls, "CoreSetBothCaller", callerCode);
    xnum(object) = 0;
    dx::vm::Call call(object, caller);
    call.run();
    CHECK(xnum(object) == 1);
}

TEST_CASE("GotoState from EndState preempts the outer GotoState", "[data]")
{
    initEngine();
    dx::UClass* cls = testInfoClass();
    dx::UObject* object = dx::objects::newObject(cls, nullptr, dx::Name(), 0);
    Bytes begin;
    begin.op({ 0x08 }).name("Begin").u32(0).u32(0).u32(0);
    dx::UState* first = makeState(cls, "CoreFirst", begin, 1);
    makeState(cls, "CoreSecond", begin, 1);
    makeState(cls, "CoreThird", begin, 1);
    Bytes endCode;
    endCode.op({ 0x71, 0x21 }).name("CoreSecond").op({ 0x16, 0x04, 0x0B });
    first->functions[dx::Name("EndState")] = makeFunction(cls, "EndState", endCode);
    std::vector<std::string> criticals;
    dx::vm::CriticalHandler previous = dx::vm::setCriticalHandler([&criticals](const std::string& message) {
        criticals.push_back(message);
    });
    REQUIRE(dx::vm::gotoState(object, dx::Name("CoreFirst")));
    dx::vm::gotoState(object, dx::Name("CoreThird"));
    dx::vm::setCriticalHandler(previous);
    CHECK(criticals.empty());
    CHECK(dx::vm::stateName(object) == dx::Name("CoreSecond"));
}

TEST_CASE("state code stops after four state changes in one update", "[data]")
{
    initEngine();
    dx::UClass* cls = testInfoClass();
    dx::UObject* object = dx::objects::newObject(cls, nullptr, dx::Name(), 0);
    Bytes ping;
    ping.op({ 0x71, 0x21 }).name("CorePong").op({ 0x16 }).name("Begin").u32(0).u32(0).u32(0);
    Bytes pong;
    pong.op({ 0x71, 0x21 }).name("CorePing").op({ 0x16 }).name("Begin").u32(0).u32(0).u32(0);
    dx::UState* pingState = makeState(cls, "CorePing", ping, 7);
    dx::UState* pongState = makeState(cls, "CorePong", pong, 7);
    Bytes count = incrementXnum(cls);
    count.op({ 0x04, 0x0B });
    dx::UFunction* beginState = makeFunction(cls, "BeginState", count);
    pingState->functions[dx::Name("BeginState")] = beginState;
    pongState->functions[dx::Name("BeginState")] = beginState;
    REQUIRE(dx::vm::gotoState(object, dx::Name("CorePing")));
    xnum(object) = 0;
    dx::vm::runStateCode(object);
    CHECK(xnum(object) == 5);
    CHECK(dx::vm::stateName(object) == dx::Name("CorePong"));
    dx::vm::runStateCode(object);
    CHECK(xnum(object) == 10);
    CHECK(dx::vm::stateName(object) == dx::Name("CorePing"));
}

TEST_CASE("globalconfig properties load from the declaring class section", "[data]")
{
    initEngine();
    dx::UClass* player = dx::objects::findClass("DeusEx.DeusExPlayer");
    dx::objects::flushLoads();
    REQUIRE(player != nullptr);
    uint8_t* address = player->defaultObject->propertyAddress(dx::Name("bSnapToLevel"));
    REQUIRE(address != nullptr);
    CHECK(*reinterpret_cast<uint32_t*>(address) == 0);
}

TEST_CASE("SaveConfig writes globalconfig to the owner section and reloads class defaults", "[data]")
{
    initEngine();
    dx::UClass* gameInfo = dx::objects::findClass("Engine.GameInfo");
    dx::UClass* deusExGameInfo = dx::objects::findClass("DeusEx.DeusExGameInfo");
    dx::objects::flushLoads();
    REQUIRE(gameInfo != nullptr);
    REQUIRE(deusExGameInfo != nullptr);
    dx::UObject* object = dx::objects::newObject(deusExGameInfo, nullptr, dx::Name(), 0);
    int32_t& spectators = *reinterpret_cast<int32_t*>(object->propertyAddress(dx::Name("MaxSpectators")));
    int32_t original = spectators;
    spectators = original + 5;
    {
        dx::vm::Call call(object, coreFunction("SaveConfig"));
        call.run();
    }
    CHECK(dx::config::forName(dx::config::configNameOf(deusExGameInfo))->get("Engine.GameInfo", "MaxSpectators") == std::to_string(original + 5));
    CHECK(*reinterpret_cast<int32_t*>(gameInfo->defaultObject->propertyAddress(dx::Name("MaxSpectators"))) == original + 5);
    CHECK(*reinterpret_cast<int32_t*>(deusExGameInfo->defaultObject->propertyAddress(dx::Name("MaxSpectators"))) == original + 5);
    spectators = original;
    {
        dx::vm::Call call(object, coreFunction("SaveConfig"));
        call.run();
    }
    CHECK(*reinterpret_cast<int32_t*>(deusExGameInfo->defaultObject->propertyAddress(dx::Name("MaxSpectators"))) == original);
}

TEST_CASE("Normal returns zero below the SafeNormal size squared threshold", "[data]")
{
    initEngine();
    {
        dx::vm::Call call(dx::objects::objectClass()->defaultObject, coreFunction("Normal"));
        call.v({ 0.00005f, 0, 0 }).run();
        dx::Vector value = *reinterpret_cast<dx::Vector*>(call.returnValue());
        CHECK(value.x == 0.0f);
        CHECK(value.y == 0.0f);
        CHECK(value.z == 0.0f);
    }
    {
        dx::vm::Call call(dx::objects::objectClass()->defaultObject, coreFunction("Normal"));
        call.v({ 0, 0.5f, 0 }).run();
        dx::Vector value = *reinterpret_cast<dx::Vector*>(call.returnValue());
        CHECK(value.x == 0.0f);
        CHECK(value.y == 1.0f);
        CHECK(value.z == 0.0f);
    }
}

TEST_CASE("Chr keeps sixteen bits of the code and Caps only uppercases ASCII", "[data]")
{
    initEngine();
    {
        dx::vm::Call call(dx::objects::objectClass()->defaultObject, coreFunction("Chr"));
        call.i(321).run();
        CHECK(*reinterpret_cast<std::string*>(call.returnValue()) == "?");
    }
    {
        dx::vm::Call call(dx::objects::objectClass()->defaultObject, coreFunction("Chr"));
        call.i(65536).run();
        CHECK(reinterpret_cast<std::string*>(call.returnValue())->empty());
    }
    {
        dx::vm::Call call(dx::objects::objectClass()->defaultObject, coreFunction("Chr"));
        call.i(65601).run();
        CHECK(*reinterpret_cast<std::string*>(call.returnValue()) == "A");
    }
    {
        dx::vm::Call call(dx::objects::objectClass()->defaultObject, coreFunction("Caps"));
        call.s("a\xe9z").run();
        CHECK(*reinterpret_cast<std::string*>(call.returnValue()) == "A\xe9Z");
    }
}

TEST_CASE("new defaults the outer to self and masks the object flags", "[data]")
{
    initEngine();
    dx::UClass* cls = testInfoClass();
    dx::UObject* self = dx::objects::newObject(cls, nullptr, dx::Name(), 0);
    Bytes code;
    code.op({ 0x11, 0x0B, 0x21 }).u32(0).op({ 0x1D }).u32(0xffffffffu).op({ 0x20 }).object(cls);
    dx::UObject* created = evaluate<dx::UObject*>(self, code);
    REQUIRE(created != nullptr);
    CHECK(created->outer == self);
    CHECK(created->flags == 0x704005u);
}

TEST_CASE("a destroyed but not yet cleaned up object still runs calls made through it", "[data]")
{
    initEngine();
    dx::UClass* cls = testInfoClass();
    dx::UObject* caller = dx::objects::newObject(cls, nullptr, dx::Name(), 0);
    dx::UObject* target = dx::objects::newObject(cls, nullptr, dx::Name(), 0);
    Bytes incrementCode = incrementXnum(cls);
    incrementCode.op({ 0x04, 0x0B });
    dx::UFunction* increment = makeFunction(cls, "CoreContextIncrement", incrementCode);
    Bytes callerCode;
    callerCode.op({ 0x19, 0x20 }).object(target).u16(6).op({ 0x00, 0x1C }).object(increment).op({ 0x16, 0x04, 0x0B });
    dx::UFunction* through = makeFunction(cls, "CoreContextCaller", callerCode);
    xnum(target) = 0;
    dx::objects::destroyObject(target);
    dx::vm::Call call(caller, through);
    call.run();
    CHECK(xnum(target) == 1);
}

TEST_CASE("relative and partial travel inherit the current URL options like FURL")
{
    std::string base = "01_NYC_UNATCOIsland?Name=JC?Class=DeusEx.JCDentonMale?load?Difficulty=2?push";
    CHECK(dx::travelUrl(base, "01_NYC_UNATCOHQ?difficulty=3?Quiet", dx::TRAVEL_Relative) == "01_NYC_UNATCOHQ?Name=JC?Class=DeusEx.JCDentonMale?difficulty=3?Quiet");
    CHECK(dx::travelUrl(base, "?Name=Paul", dx::TRAVEL_Relative) == "01_NYC_UNATCOIsland?Name=Paul?Class=DeusEx.JCDentonMale?Difficulty=2");
    CHECK(dx::travelUrl(base, "02_NYC_BatteryPark", dx::TRAVEL_Partial) == "02_NYC_BatteryPark?Name=JC?Class=DeusEx.JCDentonMale?Difficulty=2");
    CHECK(dx::travelUrl(base, "02_NYC_BatteryPark?Name=X", dx::TRAVEL_Absolute) == "02_NYC_BatteryPark?Name=X");
}

TEST_CASE("a subclass created at runtime inherits the parent's defaults, functions and states", "[data]")
{
    initEngine();
    dx::UClass* parent = testInfoClass();
    dx::UClass* sub = dx::objects::createSubclass(parent, dx::Name("RuntimeTestInfo"));
    CHECK(sub->superClass() == parent);
    CHECK(sub->isChildOf(parent));
    CHECK(sub->findFunction(dx::Name("TestStructBools")) == parent->findFunction(dx::Name("TestStructBools")));
    CHECK(sub->findState(dx::Name("AA")) == parent->findState(dx::Name("AA")));
    dx::UObject* object = dx::objects::newObject(sub, nullptr, dx::Name(), 0);
    CHECK(object->cls == sub);
    CHECK(object->isA(parent));
    CHECK(xnum(object) == 777);
    CHECK(*reinterpret_cast<std::string*>(object->propertyAddress(dx::Name("sxx"))) == "Tim");
}

TEST_CASE("findGameRoot looks for game-data next to the AppImage")
{
    std::filesystem::path base = std::filesystem::temp_directory_path() / "deusex-appimage-test";
    std::filesystem::create_directories(base / "game-data" / "System");
    std::ofstream marker(base / "game-data" / "System" / "DeusEx.u");
    marker.close();
    std::string previous = std::getenv("DEUSEX_GAME_DATA") ? std::getenv("DEUSEX_GAME_DATA") : "";
    setEnvironment("DEUSEX_GAME_DATA", (base / "missing").string().c_str());
    setEnvironment("APPIMAGE", (base / "deusex.AppImage").string().c_str());
    std::filesystem::path root = dx::config::findGameRoot("fallback");
    setEnvironment("APPIMAGE", "");
    setEnvironment("DEUSEX_GAME_DATA", previous.c_str());
    std::filesystem::remove_all(base);
    CHECK(root == base / "game-data");
}

TEST_CASE("an explicit system and user ini replace DeusEx.ini and User.ini and save under their own names", "[data]")
{
    initEngine();
    dx::Ini system = dx::config::system();
    dx::Ini user = dx::config::user();
    std::filesystem::path base = std::filesystem::temp_directory_path() / "deusex-mod-ini-test";
    std::filesystem::create_directories(base);
    std::ofstream(base / "Mod.ini") << "[URL]\r\nMap=ModIndex.dx\r\n";
    std::ofstream(base / "ModUser.ini") << "[Engine.Input]\r\nF1=ModHelp\r\n";
    dx::config::system() = dx::Ini();
    dx::config::user() = dx::Ini();
    dx::config::initialize(std::filesystem::path(DEUSEX_GAME_DATA) / "System", base / "Mod.ini", base / "ModUser.ini");
    std::optional<std::string> map = dx::config::system().get("URL", "Map");
    std::optional<std::string> key = dx::config::user().get("Engine.Input", "F1");
    std::filesystem::path systemPath = dx::config::system().path;
    std::filesystem::path userPath = dx::config::user().path;
    dx::config::system() = system;
    dx::config::user() = user;
    std::filesystem::remove_all(base);
    CHECK(map == "ModIndex.dx");
    CHECK(key == "ModHelp");
    CHECK(systemPath == dx::config::userDirectory() / "Mod.ini");
    CHECK(userPath == dx::config::userDirectory() / "ModUser.ini");
}

TEST_CASE("packages are found in the order of the Core.System Paths entries", "[data]")
{
    const char* xdg = std::getenv("XDG_DATA_HOME");
    REQUIRE(xdg != nullptr);
    std::filesystem::path home = std::filesystem::path(xdg) / "ini-paths";
    std::filesystem::remove_all(home);
    std::filesystem::path mod = home / "Mod" / "Textures";
    std::filesystem::create_directories(mod);
    std::filesystem::create_directories(home / "deusex");
    std::filesystem::copy_file(std::filesystem::path(DEUSEX_GAME_DATA) / "Textures" / "CoreTexCeramic.utx", mod / "CoreTexBrick.utx");
    std::ofstream(home / "deusex" / "DeusEx.ini") << "[Core.System]\r\nPaths=" << (mod / "*.utx").string() << "\r\nPaths=..\\System\\*.u\r\nPaths=..\\Textures\\*.utx\r\n";
    std::filesystem::path out = home / "out";
    std::string command = "XDG_DATA_HOME=\"" + home.string() + "\" \"" UCC_PATH "\" batchexport CoreTexBrick Texture bmp \"" + out.string() + "\" > /dev/null 2>&1";
    REQUIRE(std::system(command.c_str()) == 0);
    CHECK(std::filesystem::exists(out / "CermBeigFloor_A.bmp"));
    CHECK_FALSE(std::filesystem::exists(out / "rough_greywall.bmp"));
    std::filesystem::remove_all(home);
}

TEST_CASE("a mod finds its own inis, localization and user directory", "[data]")
{
    initEngine();
    dx::Ini system = dx::config::system();
    dx::Ini user = dx::config::user();
    std::filesystem::path base = std::filesystem::temp_directory_path() / "deusex-mod-test";
    std::filesystem::remove_all(base);
    std::filesystem::path mod = base / "mods" / "TestMod";
    std::filesystem::create_directories(mod / "Extras");
    std::filesystem::create_directories(mod / "TestModData" / "System");
    std::filesystem::create_directories(mod / "System");
    std::ofstream(mod / "Extras" / "Readme.ini") << "[URL]\r\nMap=Wrong.dx\r\n";
    std::ofstream(mod / "TestModData" / "System" / "testmod.ini") << "[URL]\r\nMap=ModIndex.dx\r\n[Core.System]\r\nPaths=..\\TestModData\\Maps\\*.dx\r\n";
    std::ofstream(mod / "TestModData" / "System" / "TESTMODuser.ini") << "[Engine.Input]\r\nF1=ModHelp\r\n";
    std::ofstream(mod / "System" / "ModOverlayText.int") << "[Labels]\r\nTitle=From the mod\r\n";
    std::filesystem::path vanillaUser = dx::config::userDirectory();
    dx::config::useMod(mod);
    dx::config::system() = dx::Ini();
    dx::config::user() = dx::Ini();
    dx::config::initialize(std::filesystem::path(DEUSEX_GAME_DATA) / "System");
    std::optional<std::string> map = dx::config::system().get("URL", "Map");
    std::optional<std::string> key = dx::config::user().get("Engine.Input", "F1");
    std::filesystem::path systemPath = dx::config::system().path;
    std::filesystem::path userPath = dx::config::user().path;
    std::filesystem::path modUser = dx::config::userDirectory();
    std::string title = dx::config::localize("Labels", "Title", "ModOverlayText");
    dx::config::useMod({});
    dx::config::system() = system;
    dx::config::user() = user;
    std::filesystem::remove_all(base);
    CHECK(map == "ModIndex.dx");
    CHECK(key == "ModHelp");
    CHECK(modUser == vanillaUser / "mods" / "TestMod");
    CHECK(systemPath == modUser / "testmod.ini");
    CHECK(userPath == modUser / "TESTMODuser.ini");
    CHECK(title == "From the mod");
}

TEST_CASE("a mod's packages win over the game's for the same Paths entry", "[data]")
{
    initEngine();
    dx::Ini system = dx::config::system();
    std::filesystem::path base = std::filesystem::temp_directory_path() / "deusex-mod-overlay-test";
    std::filesystem::remove_all(base);
    std::filesystem::path game = base / "game";
    std::filesystem::path mod = base / "mods" / "Overlay";
    std::filesystem::create_directories(game / "Textures");
    std::filesystem::create_directories(mod / "Textures");
    std::filesystem::copy_file(std::filesystem::path(DEUSEX_GAME_DATA) / "Textures" / "CoreTexBrick.utx", game / "Textures" / "ModOverlayTex.utx");
    std::filesystem::copy_file(std::filesystem::path(DEUSEX_GAME_DATA) / "Textures" / "CoreTexCeramic.utx", mod / "Textures" / "ModOverlayTex.utx");
    dx::config::system() = dx::Ini();
    dx::config::system().set("Core.System", "Paths", "..\\Textures\\*.utx");
    dx::config::useMod(mod);
    std::filesystem::path root = dx::objects::gameRoot();
    dx::objects::initialize(game);
    dx::objects::initialize(root);
    dx::config::useMod({});
    dx::config::system() = system;
    dx::Linker* linker = dx::objects::loadPackage(dx::Name("ModOverlayTex"));
    REQUIRE(linker != nullptr);
    bool ceramic = linker->file().findExport("Texture", "CermBeigFloor_A") != 0;
    bool brick = linker->file().findExport("Texture", "rough_greywall") != 0;
    std::filesystem::remove_all(base);
    CHECK(ceramic);
    CHECK_FALSE(brick);
}

TEST_CASE("Paths entries match directories regardless of case and their maps are listed", "[data]")
{
    initEngine();
    dx::Ini system = dx::config::system();
    std::filesystem::path base = std::filesystem::temp_directory_path() / "deusex-paths-case-test";
    std::filesystem::remove_all(base);
    std::filesystem::create_directories(base / "game" / "Mod Data" / "Maps");
    std::ofstream(base / "game" / "Mod Data" / "Maps" / "ModListedMap.dx").close();
    dx::config::system() = dx::Ini();
    dx::config::system().set("Core.System", "Paths", "..\\MOD DATA\\maps\\*.dx");
    std::filesystem::path root = dx::objects::gameRoot();
    dx::objects::initialize(base / "game");
    dx::objects::initialize(root);
    dx::config::system() = system;
    bool registered = false;
    for (const std::filesystem::path& path : dx::objects::packageFiles(".dx")) {
        if (path.filename() == "ModListedMap.dx") {
            registered = true;
        }
    }
    dx::UClass* directoryClass = dx::objects::findClass("DeusEx.GameDirectory");
    dx::UClass* actorClass = dx::objects::findClass("Engine.Actor");
    dx::objects::flushLoads();
    REQUIRE(directoryClass != nullptr);
    REQUIRE(actorClass != nullptr);
    dx::UObject* directory = dx::objects::newObject(directoryClass, dx::objects::packageObject(dx::Name("Transient")), dx::Name(), 0);
    {
        dx::vm::Call call(directory, dx::Name("SetDirType"));
        call.byte(0);
        call.run();
    }
    {
        dx::vm::Call call(directory, dx::Name("SetDirFilter"));
        call.s("ModListed");
        call.run();
    }
    dx::vm::Call(directory, dx::Name("GetGameDirectory")).run();
    dx::vm::Call count(directory, dx::Name("GetDirCount"));
    count.run();
    int32_t listed = *reinterpret_cast<int32_t*>(count.returnValue());
    dx::vm::Call filename(directory, dx::Name("GetDirFilename"));
    filename.i(0);
    filename.run();
    std::string first = *reinterpret_cast<std::string*>(filename.returnValue());
    dx::vm::Call mapName(actorClass->defaultObject, dx::Name("GetMapName"));
    mapName.s("ModListed").s("").i(0);
    mapName.run();
    std::string chosen = *reinterpret_cast<std::string*>(mapName.returnValue());
    dx::objects::setPackageFile(dx::Name("ModListedMap"), {});
    std::filesystem::remove_all(base);
    CHECK(registered);
    CHECK(listed == 1);
    CHECK(first == "ModListedMap.dx");
    CHECK(chosen == "ModListedMap.dx");
}

TEST_CASE("a mod with Default and DefUser inis, its own language and files outside Paths", "[data]")
{
    initEngine();
    dx::Ini system = dx::config::system();
    dx::Ini user = dx::config::user();
    std::filesystem::path base = std::filesystem::temp_directory_path() / "deusex-mod-default-test";
    std::filesystem::remove_all(base);
    std::filesystem::path mod = base / "mods" / "Lang";
    std::filesystem::create_directories(mod / "System");
    std::filesystem::create_directories(mod / "Music" / "Ogg");
    std::ofstream(mod / "System" / "LangDefault.ini") << "[URL]\r\nMap=LangIndex.dx\r\n[Engine.Engine]\r\nLanguage=opt\r\n[Core.System]\r\nPaths=..\\Maps\\*.dx\r\n";
    std::ofstream(mod / "System" / "LangDefUser.ini") << "[Engine.Input]\r\nF1=LangHelp\r\n";
    std::ofstream(mod / "System" / "LangText.opt") << "[Labels]\r\nTitle=Translated\r\n";
    std::ofstream(mod / "System" / "LangText.int") << "[Labels]\r\nTitle=Original\r\nSubtitle=Only in int\r\n";
    std::ofstream(mod / "Music" / "Ogg" / "Track.OGG").close();
    dx::config::useMod(mod);
    dx::config::system() = dx::Ini();
    dx::config::user() = dx::Ini();
    dx::config::initialize(std::filesystem::path(DEUSEX_GAME_DATA) / "System");
    std::optional<std::string> map = dx::config::system().get("URL", "Map");
    std::optional<std::string> key = dx::config::user().get("Engine.Input", "F1");
    std::filesystem::path systemPath = dx::config::system().path;
    std::filesystem::path userPath = dx::config::user().path;
    std::filesystem::path modUser = dx::config::userDirectory();
    std::string title = dx::config::localize("Labels", "Title", "LangText");
    std::string subtitle = dx::config::localize("Labels", "Subtitle", "LangText");
    std::filesystem::path track = dx::config::findFile("..\\music\\ogg\\track.ogg");
    std::filesystem::path stock = dx::config::findFile("..\\System\\Engine.u");
    dx::config::useMod({});
    dx::config::system() = system;
    dx::config::user() = user;
    std::filesystem::remove_all(base);
    CHECK(map == "LangIndex.dx");
    CHECK(key == "LangHelp");
    CHECK(systemPath == modUser / "Lang.ini");
    CHECK(userPath == modUser / "LangUser.ini");
    CHECK(title == "Translated");
    CHECK(subtitle == "Only in int");
    CHECK(track == mod / "Music" / "Ogg" / "Track.OGG");
    CHECK(stock == std::filesystem::path(DEUSEX_GAME_DATA) / "System" / "Engine.u");
}
