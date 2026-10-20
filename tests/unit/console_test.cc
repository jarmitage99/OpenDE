#include "core/config.h"
#include "core/objects.h"
#include "core/vm.h"
#include "engine/console.h"
#include "engine/props_engine.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {
    std::string sectionValue(const std::filesystem::path& path, const std::string& section, const std::string& key)
    {
        std::ifstream file(path);
        std::string line;
        bool inSection = false;
        while (std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (!line.empty() && line.front() == '[') {
                inSection = line == "[" + section + "]";
                continue;
            }
            if (inSection && line.rfind(key + "=", 0) == 0) {
                return line.substr(key.size() + 1);
            }
        }
        return {};
    }
}

TEST_CASE("set ini: writes the audio device section and get reads it back", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    std::string original = dx::console::execute(nullptr, "get ini:Engine.Engine.AudioDevice MusicVolume");
    REQUIRE(original == "153");
    dx::console::execute(nullptr, "set ini:Engine.Engine.AudioDevice MusicVolume 100");
    CHECK(dx::console::execute(nullptr, "get ini:Engine.Engine.AudioDevice MusicVolume") == "100");
    CHECK(sectionValue(dx::config::system().path, "Galaxy.GalaxyAudioSubsystem", "MusicVolume") == "100");
    dx::console::execute(nullptr, "set ini:Engine.Engine.AudioDevice MusicVolume " + original);
    CHECK(sectionValue(dx::config::system().path, "Galaxy.GalaxyAudioSubsystem", "MusicVolume") == "153");
}

TEST_CASE("SET InputExt rebinds a key and saves it to the user ini", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    std::string original = dx::console::binding(dx::console::keyFromName("N"));
    dx::console::execute(nullptr, "SET InputExt N ShowGoalsWindow");
    CHECK(dx::console::binding(dx::console::keyFromName("N")) == "ShowGoalsWindow");
    CHECK(dx::console::execute(nullptr, "KEYBINDING N") == "ShowGoalsWindow");
    CHECK(sectionValue(dx::config::user().path, "Engine.Input", "N") == "ShowGoalsWindow");
    dx::console::execute(nullptr, "SET InputExt N " + original);
    CHECK(sectionValue(dx::config::user().path, "Engine.Input", "N") == original);
}

TEST_CASE("set of a config property updates the class default and persists it", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    std::string original = dx::console::execute(nullptr, "get DeusExPlayer bHelpMessages");
    REQUIRE(original == "True");
    dx::console::execute(nullptr, "set DeusExPlayer bHelpMessages False");
    CHECK(dx::console::execute(nullptr, "get DeusExPlayer bHelpMessages") == "False");
    CHECK(sectionValue(dx::config::user().path, "DeusEx.DeusExPlayer", "bHelpMessages") == "False");
    dx::console::execute(nullptr, "set DeusExPlayer bHelpMessages " + original);
    CHECK(sectionValue(dx::config::user().path, "DeusEx.DeusExPlayer", "bHelpMessages") == "True");
}

TEST_CASE("open start and servertravel pass partial absolute and relative travel types", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    std::vector<std::pair<std::string, int32_t>> requests;
    auto previous = dx::console::openMapHook;
    dx::console::openMapHook = [&](const std::string& url, int32_t travelType) {
        requests.emplace_back(url, travelType);
    };
    dx::console::execute(nullptr, "open 02_NYC_BatteryPark?Difficulty=2");
    dx::console::execute(nullptr, "start 01_NYC_UNATCOIsland");
    dx::console::execute(nullptr, "servertravel 03_NYC_AirfieldHeliBase");
    dx::console::openMapHook = previous;
    REQUIRE(requests.size() == 3);
    CHECK(requests[0].first == "02_NYC_BatteryPark?Difficulty=2");
    CHECK(requests[0].second == 1);
    CHECK(requests[1].second == 0);
    CHECK(requests[2].second == 2);
}

TEST_CASE("a Talk binding opens the script console with a Say prompt", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
    {
        dx::World world;
        REQUIRE(world.loadMap("01_NYC_UNATCOIsland"));
        dx::UObject* scriptConsole = dx::player::Console(world.viewport);
        REQUIRE(scriptConsole != nullptr);
        CHECK(dx::console::bTyping(scriptConsole) == 0);
        dx::console::execute(world.player(), "Talk");
        CHECK(dx::console::bTyping(scriptConsole) != 0);
        CHECK(dx::console::TypedStr(scriptConsole) == "Say ");
    }
    dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
}

TEST_CASE("Console.ConsoleCommand prints the command output to the console", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
    {
        dx::World world;
        REQUIRE(world.loadMap("01_NYC_UNATCOIsland"));
        dx::UObject* scriptConsole = dx::player::Console(world.viewport);
        REQUIRE(scriptConsole != nullptr);
        dx::vm::Call call(scriptConsole, dx::Name("ConsoleCommand"));
        call.s("get DeusExPlayer bCheatsEnabled").run();
        CHECK(dx::console::MsgText(scriptConsole, dx::console::TopLine(scriptConsole)) == "False");
    }
    dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
}
