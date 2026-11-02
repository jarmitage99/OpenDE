#include "compiler/exporter.h"
#include "compiler/package_dump.h"
#include "core/objects.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {
    std::string lowered(std::string text)
    {
        for (char& c : text) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return text;
    }

    std::string normalized(const dx::compiler::DumpedExport& entry)
    {
        static const std::regex dependencyCrc(R"((\{[^}]*\}:[01]):[0-9]+)");
        static const std::regex importTime(R"(InternalTime\[[01]\]:2=[0-9a-f]+ )");
        static const std::regex leadingText(R"(text=\s+)");
        std::string body = std::regex_replace(entry.body, dependencyCrc, "$1");
        body = std::regex_replace(body, importTime, "");
        if (entry.className == "TextBuffer") {
            body = std::regex_replace(body, leadingText, "text=", std::regex_constants::format_first_only);
            while (!body.empty() && (body.back() == '\r' || body.back() == '\n' || body.back() == ' ')) {
                body.pop_back();
            }
        }
        return lowered(body);
    }

    std::map<std::string, dx::compiler::DumpedExport> withCanonicalPalettes(const std::map<std::string, dx::compiler::DumpedExport>& dump)
    {
        static const std::regex paletteRef(R"(Palette\[0\]:5=\{([^}]*)\})");
        std::map<std::string, std::string> names;
        for (const auto& [path, entry] : dump) {
            for (std::sregex_iterator it(entry.body.begin(), entry.body.end(), paletteRef), end; it != end; ++it) {
                names.emplace(lowered((*it)[1].str()), "palette#" + std::to_string(names.size()));
            }
        }
        std::map<std::string, std::string> keys;
        for (const auto& [ref, name] : names) {
            keys[ref.substr(ref.find('.') + 1)] = name;
        }
        std::map<std::string, dx::compiler::DumpedExport> result;
        for (const auto& [path, entry] : dump) {
            dx::compiler::DumpedExport copy = entry;
            copy.body.clear();
            auto last = entry.body.cbegin();
            for (std::sregex_iterator it(entry.body.begin(), entry.body.end(), paletteRef), end; it != end; ++it) {
                copy.body.append(last, (*it)[1].first);
                copy.body += names[lowered((*it)[1].str())];
                last = (*it)[1].second;
            }
            copy.body.append(last, entry.body.cend());
            auto key = keys.find(path);
            result[key != keys.end() ? key->second : path] = copy;
        }
        return result;
    }

    std::vector<std::string> editPackages()
    {
        std::ifstream in(std::filesystem::path(DEUSEX_GAME_DATA) / "System" / "DeusEx.ini");
        std::vector<std::string> packages;
        std::string line;
        while (std::getline(in, line)) {
            if (line.rfind("EditPackages=", 0) != 0) {
                continue;
            }
            std::string name = line.substr(13);
            while (!name.empty() && (name.back() == '\r' || name.back() == ' ')) {
                name.pop_back();
            }
            packages.push_back(name);
        }
        return packages;
    }

    std::map<std::string, dx::compiler::DumpedExport> dumpByPath(const std::filesystem::path& file)
    {
        std::map<std::string, dx::compiler::DumpedExport> result;
        for (auto& [path, entry] : dx::compiler::dumpPackage(file, std::filesystem::path(DEUSEX_GAME_DATA) / "System")) {
            result[lowered(path)] = entry;
        }
        return result;
    }

    int run(const std::string& command)
    {
        return std::system(command.c_str());
    }
}

TEST_CASE("exported default properties use the UnrealEd format", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::UClass* pawn = dx::objects::findClass("DeusEx.ScriptedPawn");
    REQUIRE(pawn != nullptr);
    dx::UClass* parent = pawn->superClass();
    std::string text = dx::compiler::exportProperties(pawn, pawn->defaultObject->data, parent, parent->defaultObject->data, 4);
    CHECK(text.find("     Restlessness=0.500000\r\n") != std::string::npos);
    CHECK(text.find("     Orders=Wandering\r\n") != std::string::npos);
    CHECK(text.find("     RaiseAlarm=RAISEALARM_BeforeFleeing\r\n") != std::string::npos);
}

TEST_CASE("exported floats that six decimals cannot represent keep full precision", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::UClass* cat = dx::objects::findClass("DeusEx.Cat");
    REQUIRE(cat != nullptr);
    dx::UClass* parent = cat->superClass();
    std::string text = dx::compiler::exportProperties(cat, cat->defaultObject->data, parent, parent->defaultObject->data, 4);
    std::string key = "     WalkingSpeed=";
    size_t at = text.find(key);
    REQUIRE(at != std::string::npos);
    std::string value = text.substr(at + key.size(), text.find("\r\n", at) - at - key.size());
    CHECK(value == "0.111111112");
    float parsed = static_cast<float>(std::atof(value.c_str()));
    uint32_t bits = 0;
    std::memcpy(&bits, &parsed, sizeof(bits));
    CHECK(bits == 0x3de38e39u);
}

TEST_CASE("ucc make rebuilds every EditPackage from exported sources", "[data]")
{
    const char* xdg = std::getenv("XDG_DATA_HOME");
    REQUIRE(xdg != nullptr);
    std::filesystem::path home = std::filesystem::path(xdg) / "ucc-make";
    std::filesystem::remove_all(home);
    std::filesystem::path user = home / "deusex";
    std::vector<std::string> packages = editPackages();
    REQUIRE(packages.size() == 19);
    CHECK(packages.front() == "Core");
    std::string environment = "XDG_DATA_HOME=\"" + home.string() + "\" ";
    for (const std::string& package : packages) {
        INFO(package);
        std::string command = environment + "\"" UCC_PATH "\" exportsources " + package + ".u \"" + user.string() + "\" > /dev/null 2>&1";
        REQUIRE(run(command) == 0);
    }
    CHECK(std::filesystem::exists(user / "Engine" / "Fonts" / "LargeFont.txt"));
    REQUIRE(run(environment + "\"" UCC_PATH "\" make > /dev/null 2>&1") == 0);
    const std::set<std::string> lossyMeshes = { "DeusExCharacters", "MPCharacters", "DeusExDeco" };
    for (const std::string& package : packages) {
        INFO(package);
        std::filesystem::path shipped = std::filesystem::path(DEUSEX_GAME_DATA) / "System" / (package + ".u");
        std::filesystem::path built = user / "System" / (package + ".u");
        REQUIRE(std::filesystem::exists(built));
        auto expected = withCanonicalPalettes(dumpByPath(shipped));
        auto actual = withCanonicalPalettes(dumpByPath(built));
        CHECK(actual.size() == expected.size());
        int mismatches = 0;
        std::string first;
        for (auto& [path, entry] : expected) {
            auto it = actual.find(path);
            bool same = it != actual.end() && it->second.className == entry.className && lowered(it->second.super) == lowered(entry.super) && it->second.flags == entry.flags;
            if (same && lossyMeshes.count(package) && entry.className == "LodMesh") {
                std::string left = normalized(entry);
                std::string right = normalized(it->second);
                same = left.substr(0, left.find("\nrest=")) == right.substr(0, right.find("\nrest="));
            } else if (same) {
                same = normalized(it->second) == normalized(entry);
            }
            if (!same) {
                mismatches++;
                if (first.empty()) {
                    first = path;
                }
            }
        }
        INFO("first mismatch " << first);
        CHECK(mismatches == 0);
    }
    auto ipServer = dumpByPath(user / "System" / "IpServer.u");
    CHECK(ipServer["udpserverquery"].body.find("{IpServer.UdpServerQuery}:1:1727771771,{IpDrv.UdpLink}:1:2233407030,") != std::string::npos);
    auto engine = dumpByPath(user / "System" / "Engine.u");
    CHECK(engine["largefont"].className == "Font");
    CHECK(engine["actor.ambientsound"].body.find("type={Engine.Sound}") != std::string::npos);
    std::filesystem::remove_all(home);
}

TEST_CASE("ucc make rebuilds Core on its own", "[data]")
{
    const char* xdg = std::getenv("XDG_DATA_HOME");
    REQUIRE(xdg != nullptr);
    std::filesystem::path home = std::filesystem::path(xdg) / "ucc-core";
    std::filesystem::remove_all(home);
    std::filesystem::path user = home / "deusex";
    std::filesystem::create_directories(user);
    {
        std::ifstream in(std::filesystem::path(DEUSEX_GAME_DATA) / "System" / "DeusEx.ini", std::ios::binary);
        std::ofstream out(user / "DeusEx.ini", std::ios::binary);
        std::string line;
        while (std::getline(in, line)) {
            if (line.rfind("EditPackages=", 0) != 0 || line.rfind("EditPackages=Core", 0) == 0) {
                out << line << "\n";
            }
        }
    }
    std::string environment = "XDG_DATA_HOME=\"" + home.string() + "\" ";
    REQUIRE(run(environment + "\"" UCC_PATH "\" exportsources Core.u \"" + user.string() + "\" > /dev/null 2>&1") == 0);
    std::filesystem::path log = home / "make.log";
    REQUIRE(run(environment + "\"" UCC_PATH "\" make > \"" + log.string() + "\" 2>&1") == 0);
    std::ifstream in(log);
    std::stringstream output;
    output << in.rdbuf();
    CHECK(output.str().find("Building Core from ") != std::string::npos);
    CHECK(output.str().find("--------------------Engine") == std::string::npos);
    auto expected = dumpByPath(std::filesystem::path(DEUSEX_GAME_DATA) / "System" / "Core.u");
    auto actual = dumpByPath(user / "System" / "Core.u");
    REQUIRE(actual.size() == expected.size());
    int mismatches = 0;
    for (auto& [path, entry] : expected) {
        auto it = actual.find(path);
        if (it == actual.end() || it->second.className != entry.className || it->second.flags != entry.flags || normalized(it->second) != normalized(entry)) {
            mismatches++;
        }
    }
    CHECK(mismatches == 0);
    CHECK(actual["object"].className == "Class");
    std::filesystem::remove_all(home);
}

TEST_CASE("ucc make reports script errors with file and line and writes no package", "[data]")
{
    struct BrokenScript {
        const char* source;
        const char* error;
    };

    const BrokenScript cases[] = {
        { "class Broken extends Actor;\r\n\r\nvar int Count\r\nvar int Other;\r\n", "Broken.uc(4) : Error, Missing ';' before 'var'" },
        { "class Broken extends Actor;\r\n\r\nfunction Touch(Actor Other)\r\n{\r\n\tUndefinedThing = 1;\r\n}\r\n", "Broken.uc(5) : Error, 'UndefinedThing': Bad command or expression" },
        { "class Broken extends Actor;\r\n\r\nfunction PostBeginPlay()\r\n{\r\n\tlocal int Value;\r\n\r\n\tValue = Self;\r\n}\r\n", "Broken.uc(7) : Error, Type mismatch in '='" },
    };
    const char* xdg = std::getenv("XDG_DATA_HOME");
    REQUIRE(xdg != nullptr);
    std::filesystem::path home = std::filesystem::path(xdg) / "ucc-errors";
    std::filesystem::path user = home / "deusex";
    for (const BrokenScript& broken : cases) {
        INFO(broken.error);
        std::filesystem::remove_all(home);
        std::filesystem::create_directories(user / "BrokenMod" / "Classes");
        {
            std::ifstream in(std::filesystem::path(DEUSEX_GAME_DATA) / "System" / "DeusEx.ini", std::ios::binary);
            std::stringstream ini;
            ini << in.rdbuf();
            std::string text = ini.str();
            size_t last = text.rfind("EditPackages=");
            REQUIRE(last != std::string::npos);
            text.insert(text.find('\n', last) + 1, "EditPackages=BrokenMod\r\n");
            std::ofstream out(user / "DeusEx.ini", std::ios::binary);
            out << text;
        }
        {
            std::ofstream out(user / "BrokenMod" / "Classes" / "Broken.uc", std::ios::binary);
            out << broken.source;
        }
        std::filesystem::path log = home / "make.log";
        CHECK(run("XDG_DATA_HOME=\"" + home.string() + "\" \"" UCC_PATH "\" make > \"" + log.string() + "\" 2>&1") != 0);
        std::ifstream in(log);
        std::stringstream output;
        output << in.rdbuf();
        CHECK(output.str().find(broken.error) != std::string::npos);
        CHECK_FALSE(std::filesystem::exists(user / "System" / "BrokenMod.u"));
    }
    std::filesystem::remove_all(home);
}
