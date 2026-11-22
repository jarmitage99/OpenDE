#include "core/config.h"
#include "core/mods.h"
#include "core/objects.h"
#include "engine/console.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
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

    std::filesystem::path scratch(const char* name)
    {
        std::filesystem::path base = std::filesystem::temp_directory_path() / name;
        std::filesystem::remove_all(base);
        std::filesystem::create_directories(base);
        return base;
    }

    void write(const std::filesystem::path& file, const std::string& text)
    {
        std::filesystem::create_directories(file.parent_path());
        std::ofstream(file, std::ios::binary) << text;
    }

    void link(const std::filesystem::path& file, const char* gamePackage)
    {
        std::filesystem::create_directories(file.parent_path());
        std::filesystem::create_symlink(std::filesystem::path(DEUSEX_GAME_DATA) / "System" / gamePackage, file);
    }

    bool hasIssue(const std::vector<dx::mods::Issue>& issues, bool fatal, const std::string& text)
    {
        return std::any_of(issues.begin(), issues.end(), [&](const dx::mods::Issue& issue) {
            return issue.fatal == fatal && issue.text == text;
        });
    }

    bool hasIssueStarting(const std::vector<dx::mods::Issue>& issues, bool fatal, const std::string& prefix)
    {
        return std::any_of(issues.begin(), issues.end(), [&](const dx::mods::Issue& issue) {
            return issue.fatal == fatal && issue.text.rfind(prefix, 0) == 0;
        });
    }
}

TEST_CASE("the mods folder comes from DEUSEX_MODS, then the game folder, then next to it")
{
    std::filesystem::path base = scratch("deusex-mods-root-test");
    std::filesystem::path game = base / "game";
    std::filesystem::create_directories(game);
    std::filesystem::create_directories(base / "mods");
    setEnvironment("DEUSEX_MODS", "");
    std::filesystem::path beside = dx::mods::findRoot(game);
    std::filesystem::create_directories(game / "mods");
    std::filesystem::path inside = dx::mods::findRoot(game);
    setEnvironment("DEUSEX_MODS", (base / "elsewhere").string().c_str());
    std::filesystem::path chosen = dx::mods::findRoot(game);
    setEnvironment("DEUSEX_MODS", "");
    std::filesystem::remove_all(base);
    CHECK(beside == base / "mods");
    CHECK(inside == game / "mods");
    CHECK(chosen == base / "elsewhere");
}

TEST_CASE("each combination of mods gets its own user directory")
{
    std::filesystem::path base = scratch("deusex-mods-user-test");
    std::filesystem::create_directories(base / "Beta");
    std::filesystem::create_directories(base / "Alpha");
    std::filesystem::path vanilla = dx::config::userDirectory();
    dx::config::useMods({ base / "Beta", base / "Alpha" });
    std::filesystem::path combined = dx::config::userDirectory();
    dx::config::useMods({ base / "Alpha" });
    std::filesystem::path single = dx::config::userDirectory();
    dx::config::useMods({});
    std::filesystem::remove_all(base);
    CHECK(vanilla == dx::config::baseUserDirectory());
    CHECK(combined == vanilla / "mods" / "Alpha+Beta");
    CHECK(single == vanilla / "mods" / "Alpha");
}

TEST_CASE("an add-on's packages win over a full mod's, which win over the game's", "[data]")
{
    initEngine();
    dx::Ini system = dx::config::system();
    std::filesystem::path base = scratch("deusex-mods-layer-test");
    std::filesystem::path game = base / "game";
    std::filesystem::path full = base / "mods" / "Full";
    std::filesystem::path addon = base / "mods" / "Addon";
    std::filesystem::path textures = std::filesystem::path(DEUSEX_GAME_DATA) / "Textures";
    std::filesystem::create_directories(game / "Textures");
    std::filesystem::create_directories(full / "Textures");
    std::filesystem::create_directories(addon / "Textures");
    write(full / "System" / "full.ini", "[Core.System]\r\nPaths=..\\Textures\\*.utx\r\n");
    std::filesystem::copy_file(textures / "CoreTexBrick.utx", game / "Textures" / "ModLayerTop.utx");
    std::filesystem::copy_file(textures / "CoreTexCeramic.utx", full / "Textures" / "ModLayerTop.utx");
    std::filesystem::copy_file(textures / "CoreTexMetal.utx", addon / "Textures" / "ModLayerTop.utx");
    std::filesystem::copy_file(textures / "CoreTexBrick.utx", game / "Textures" / "ModLayerMiddle.utx");
    std::filesystem::copy_file(textures / "CoreTexCeramic.utx", full / "Textures" / "ModLayerMiddle.utx");
    dx::config::system() = dx::Ini();
    dx::config::system().set("Core.System", "Paths", "..\\Textures\\*.utx");
    dx::config::useMods({ full, addon });
    std::filesystem::path root = dx::objects::gameRoot();
    dx::objects::initialize(game);
    dx::objects::initialize(root);
    dx::config::useMods({});
    dx::config::system() = system;
    dx::Linker* top = dx::objects::loadPackage(dx::Name("ModLayerTop"));
    dx::Linker* middle = dx::objects::loadPackage(dx::Name("ModLayerMiddle"));
    REQUIRE(top != nullptr);
    REQUIRE(middle != nullptr);
    bool topIsMetal = top->file().findExport("Texture", "StanGrayMetal_A") != 0;
    bool middleIsCeramic = middle->file().findExport("Texture", "CermBeigFloor_A") != 0;
    bool middleIsBrick = middle->file().findExport("Texture", "rough_greywall") != 0;
    std::filesystem::remove_all(base);
    CHECK(topIsMetal);
    CHECK(middleIsCeramic);
    CHECK_FALSE(middleIsBrick);
}

TEST_CASE("two full mods, or different copies of one script package, can't be enabled together")
{
    std::filesystem::path base = scratch("deusex-mods-clash-test");
    std::filesystem::path game = base / "game";
    std::filesystem::path mods = base / "mods";
    std::filesystem::create_directories(game);
    write(mods / "Full1" / "System" / "one.ini", "[Core.System]\r\nPaths=..\\System\\*.u\r\n");
    write(mods / "Full2" / "System" / "two.ini", "[Core.System]\r\nPaths=..\\System\\*.u\r\n");
    write(mods / "AddonA" / "System" / "Shared.u", "aaa");
    write(mods / "AddonB" / "System" / "Shared.u", "bbbb");
    write(mods / "AddonC" / "System" / "Shared.u", "aaa");
    write(mods / "AddonA" / "Textures" / "Look.utx", "x");
    write(mods / "AddonC" / "Textures" / "Look.utx", "yy");
    std::vector<dx::mods::Issue> fulls = dx::mods::check(game, { mods / "Full1", mods / "Full2" });
    std::vector<dx::mods::Issue> scripts = dx::mods::check(game, { mods / "AddonA", mods / "AddonB" });
    std::vector<dx::mods::Issue> textures = dx::mods::check(game, { mods / "AddonA", mods / "AddonC" });
    std::vector<dx::mods::Issue> missing = dx::mods::check(game, { mods / "Nowhere" });
    std::string full = dx::mods::describe(game, mods, "Full1", { "AddonA", "Full1", "Full2" });
    std::string addon = dx::mods::describe(game, mods, "AddonA", { "AddonA", "AddonB", "AddonC" });
    std::filesystem::remove_all(base);
    CHECK(hasIssue(fulls, true, "Full1 and Full2 each bring their own game setup, so only one of them can be enabled."));
    CHECK(hasIssue(scripts, true, "AddonA and AddonB both ship their own Shared.u, so only one of them can be enabled."));
    REQUIRE(textures.size() == 1);
    CHECK(hasIssue(textures, false, "AddonA replaces Look.utx from AddonC."));
    CHECK(hasIssue(missing, true, "The mod folder " + (mods / "Nowhere").string() + " was not found."));
    CHECK(full == "Full mod with its own setup in one.ini. Can't be combined with Full2. Changes apply after restarting the game.");
    CHECK(addon == "Add-on, loaded on top of the game or of a full mod. Can't be combined with AddonB. Changes apply after restarting the game.");
}

TEST_CASE("a mod's package that references objects or packages that are not there is reported", "[data]")
{
    initEngine();
    std::filesystem::path base = scratch("deusex-mods-imports-test");
    std::filesystem::path empty = base / "empty";
    std::filesystem::create_directories(empty);
    link(base / "mods" / "Broken" / "System" / "Extension.u", "Fire.u");
    link(base / "mods" / "User" / "System" / "DeusEx.u", "DeusEx.u");
    link(base / "mods" / "Lonely" / "System" / "DeusEx.u", "DeusEx.u");
    std::vector<dx::mods::Issue> broken = dx::mods::check(DEUSEX_GAME_DATA, { base / "mods" / "Broken", base / "mods" / "User" });
    std::vector<dx::mods::Issue> lonely = dx::mods::check(empty, { base / "mods" / "Lonely" });
    std::vector<dx::mods::Issue> fine = dx::mods::check(DEUSEX_GAME_DATA, { base / "mods" / "Lonely" });
    std::filesystem::remove_all(base);
    CHECK(hasIssueStarting(broken, true, "User: DeusEx.u uses objects that Extension.u from Broken does not have: "));
    CHECK(hasIssue(lonely, true, "Lonely: DeusEx.u needs the package Engine, which is not installed."));
    CHECK(fine.empty());
}

TEST_CASE("native functions that are not implemented are a warning", "[data]")
{
    initEngine();
    std::filesystem::path base = scratch("deusex-mods-natives-test");
    link(base / "mods" / "Natives" / "System" / "Engine.u", "Engine.u");
    std::vector<dx::mods::Issue> issues = dx::mods::check(DEUSEX_GAME_DATA, { base / "mods" / "Natives" });
    std::filesystem::remove_all(base);
    REQUIRE(issues.size() == 1);
    CHECK_FALSE(issues[0].fatal);
    CHECK(issues[0].text.rfind("Natives: native functions that are not implemented: StatLog", 0) == 0);
}

TEST_CASE("the mods menu reads and writes the enabled mods through the console")
{
    std::filesystem::path file = dx::config::baseUserDirectory() / "Mods.ini";
    std::filesystem::remove(file);
    dx::mods::takeChanged();
    dx::console::execute(nullptr, "set mod: New Vision True");
    dx::console::execute(nullptr, "set mod: GMDX 1");
    bool changed = dx::mods::takeChanged();
    dx::console::execute(nullptr, "set mod: GMDX True");
    bool unchanged = dx::mods::takeChanged();
    std::string vision = dx::console::execute(nullptr, "get mod: New Vision");
    std::vector<std::string> both = dx::mods::enabled();
    dx::console::execute(nullptr, "set mod: New Vision False");
    std::string off = dx::console::execute(nullptr, "get mod: New Vision");
    std::vector<std::string> one = dx::mods::enabled();
    std::filesystem::remove(file);
    CHECK(changed);
    CHECK_FALSE(unchanged);
    CHECK(vision == "True");
    CHECK(both == std::vector<std::string> { "GMDX", "New Vision" });
    CHECK(off == "False");
    CHECK(one == std::vector<std::string> { "GMDX" });
}

TEST_CASE("enabled mods are alphabetical unless Mods.ini gives an order")
{
    std::filesystem::path file = dx::config::baseUserDirectory() / "Mods.ini";
    write(file, "[Mods]\r\nzeta=True\r\nBeta=True\r\nAlpha=True\r\nGamma=False\r\n");
    std::vector<std::string> alphabetical = dx::mods::enabled();
    write(file, "[Mods]\r\nzeta=True\r\nBeta=True\r\nAlpha=True\r\nGamma=False\r\n[Order]\r\nMod=Zeta\r\nMod=Gamma\r\nMod=beta\r\n");
    std::vector<std::string> ordered = dx::mods::enabled();
    std::filesystem::remove(file);
    CHECK(alphabetical == std::vector<std::string> { "Alpha", "Beta", "zeta" });
    CHECK(ordered == std::vector<std::string> { "zeta", "Beta", "Alpha" });
}
