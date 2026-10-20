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
#include <sstream>
#include <string>

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
        std::string body = std::regex_replace(entry.body, dependencyCrc, "$1");
        if (entry.className == "TextBuffer") {
            while (!body.empty() && (body.back() == '\r' || body.back() == '\n' || body.back() == ' ')) {
                body.pop_back();
            }
        }
        return lowered(body);
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

TEST_CASE("ucc make rebuilds the shipped script packages from exported sources", "[data]")
{
    const char* xdg = std::getenv("XDG_DATA_HOME");
    REQUIRE(xdg != nullptr);
    std::filesystem::path home = std::filesystem::path(xdg) / "ucc-make";
    std::filesystem::remove_all(home);
    std::filesystem::path user = home / "deusex";
    const char* packages[] = { "Fire", "IpDrv", "ConSys", "IpServer", "DeusEx" };
    std::string environment = "XDG_DATA_HOME=\"" + home.string() + "\" ";
    for (const char* package : packages) {
        std::string command = environment + "\"" UCC_PATH "\" batchexport " + package + ".u class uc \"" + (user / package / "Classes").string() + "\" > /dev/null 2>&1";
        REQUIRE(run(command) == 0);
    }
    REQUIRE(run(environment + "\"" UCC_PATH "\" make > /dev/null 2>&1") == 0);
    for (const char* package : packages) {
        INFO(package);
        std::filesystem::path shipped = std::filesystem::path(DEUSEX_GAME_DATA) / "System" / (std::string(package) + ".u");
        std::filesystem::path built = user / "System" / (std::string(package) + ".u");
        REQUIRE(std::filesystem::exists(built));
        auto expected = dumpByPath(shipped);
        auto actual = dumpByPath(built);
        CHECK(actual.size() == expected.size());
        int mismatches = 0;
        std::string first;
        for (auto& [path, entry] : expected) {
            auto it = actual.find(path);
            bool same = it != actual.end() && it->second.className == entry.className && lowered(it->second.super) == lowered(entry.super) && it->second.flags == entry.flags && normalized(it->second) == normalized(entry);
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
