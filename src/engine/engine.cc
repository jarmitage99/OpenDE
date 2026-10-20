#include "audio/audio.h"
#include "core/config.h"
#include "core/natives.h"
#include "core/objects.h"
#include "engine/props_consys.h"
#include "engine/props_deusex.h"
#include "engine/props_engine.h"
#include "engine/props_extension.h"
#include "engine/props_fire.h"
#include "engine/mesh.h"
#include "engine/resources.h"
#include "engine/actor.h"
#include "engine/console.h"
#include "engine/world.h"
#include "extension/extension.h"

#include <filesystem>

namespace dx::engine {
    void initializeCore(const std::string& gameRoot)
    {
        static bool initialized = false;
        if (initialized) {
            return;
        }
        initialized = true;
        std::filesystem::path root(gameRoot);
        config::initialize(root / "System");
        registerEngineResources();
        registerMeshResources();
        registerObjectNatives();
        registerEngineNatives();
        extension::registerNatives();
        registerDeusExNatives();
        objects::initialize(root);
    }

    void initialize(const std::string& gameRoot)
    {
        static bool initialized = false;
        if (initialized) {
            return;
        }
        initialized = true;
        initializeCore(gameRoot);
        const char* packages[] = { "Engine", "Fire", "Extension", "ConSys", "DeusExText", "DeusEx" };
        for (const char* package : packages) {
            objects::loadPackage(Name(package));
        }
        bindProps_engine();
        bindProps_fire();
        bindProps_extension();
        bindProps_consys();
        bindProps_deusex();
        objects::flushLoads();
        bindEngineClasses();
        console::loadBindings();
    }
}
