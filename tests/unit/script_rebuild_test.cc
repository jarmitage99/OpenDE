#include "compiler/exporter.h"
#include "core/objects.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

#include <string>

TEST_CASE("exported string defaults escape backslashes so they import unchanged", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::UClass* setup = dx::objects::findClass("UBrowser.UBrowserIRCSetupClient");
    REQUIRE(setup != nullptr);
    dx::UClass* parent = setup->superClass();
    std::string text = dx::compiler::exportProperties(setup, setup->defaultObject->data, parent, parent->defaultObject->data, 4);
    CHECK(text.find("     WarningText=\"The Chat facility will connect you to the Internet Relay Chat (IRC) network.\\\\n\\\\nEpic Games") != std::string::npos);
}

TEST_CASE("finding an object by path skips same named objects of another class", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    REQUIRE(dx::objects::loadPackage(dx::Name("Effects")) != nullptr);
    dx::UClass* texture = dx::objects::findClass("Engine.Texture");
    dx::UClass* palette = dx::objects::findClass("Engine.Palette");
    REQUIRE(texture != nullptr);
    REQUIRE(palette != nullptr);
    dx::UObject* found = dx::objects::findObject("Effects.Electricity.BioCell_SFX", texture);
    REQUIRE(found != nullptr);
    CHECK(found->isA(texture));
    CHECK(found->pathName() == "Effects.Electricity.BioCell_SFX");
    dx::UObject* other = dx::objects::findObject("Effects.Electricity.BioCell_SFX", palette);
    REQUIRE(other != nullptr);
    CHECK(other->isA(palette));
    CHECK(other != found);
}
