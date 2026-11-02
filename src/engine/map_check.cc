#include "engine/map_check.h"

#include "core/objects.h"
#include "engine/actor.h"
#include "engine/csg.h"
#include "engine/props_engine.h"
#include "engine/resources.h"

#include <unordered_map>
#include <unordered_set>

namespace dx {
    namespace mapcheck {
        namespace {
            bool brushHasPolys(UObject* actor)
            {
                UModel* model = cast<UModel>(actor::Brush(actor));
                if (!model) {
                    return false;
                }
                objects::ensureLoaded(model);
                objects::flushLoads();
                if (!model->polys) {
                    return false;
                }
                objects::ensureLoaded(model->polys);
                objects::flushLoads();
                return !model->polys->polys.empty();
            }

            bool controlsZone(const UModel& model, UObject* actor)
            {
                for (const ZoneProperties& zone : model.zones) {
                    if (zone.zoneActor == actor) {
                        return true;
                    }
                }
                return false;
            }
        }

        const char* severityName(Severity severity)
        {
            return severity == Severity::Error ? "Error" : "Warning";
        }

        std::vector<Issue> check(ULevel* level)
        {
            std::vector<Issue> issues;
            auto report = [&](Severity severity, UObject* actor, const std::string& message) {
                issues.push_back({ severity, actor, message });
            };
            UClass* levelInfoClass = objects::findClass("Engine.LevelInfo");
            UClass* zoneInfoClass = objects::findClass("Engine.ZoneInfo");
            UClass* brushClass = objects::findClass("Engine.Brush");
            UClass* lightClass = objects::findClass("Engine.Light");
            UClass* triggerClass = objects::findClass("Engine.Trigger");
            UClass* startClass = objects::findClass("Engine.PlayerStart");
            UClass* deusExInfoClass = objects::findClass("DeusEx.DeusExLevelInfo");
            std::vector<UObject*> actors;
            for (UObject* actor : level->actors) {
                if (actor && !actor->destroyed && actor->data && actor->isA(Name("Actor"))) {
                    actors.push_back(actor);
                }
            }
            UObject* info = level->actors.empty() ? nullptr : level->actors[0];
            if (!info || !levelInfoClass || !info->isA(levelInfoClass)) {
                report(Severity::Error, nullptr, "The first actor of the level is not a LevelInfo");
            }
            bool hasStart = false;
            bool hasDeusExInfo = false;
            std::unordered_set<Name> tags;
            std::unordered_set<Name> events;
            std::unordered_map<Name, UObject*> names;
            for (UObject* actor : actors) {
                hasStart = hasStart || (startClass && actor->isA(startClass));
                hasDeusExInfo = hasDeusExInfo || (deusExInfoClass && actor->isA(deusExInfoClass));
                if (!actor::Tag(actor).none()) {
                    tags.insert(actor::Tag(actor));
                }
                if (!actor::Event(actor).none()) {
                    events.insert(actor::Event(actor));
                }
            }
            if (!hasStart) {
                report(Severity::Error, nullptr, "No PlayerStart in the level, the player has nowhere to spawn");
            }
            if (deusExInfoClass && !hasDeusExInfo) {
                report(Severity::Warning, nullptr, "No DeusExLevelInfo in the level, mission scripts and level settings will be missing");
            }
            UModel* model = level->model;
            bool built = model && !model->nodes.empty();
            for (UObject* actor : actors) {
                auto inserted = names.emplace(actor->name, actor);
                if (!inserted.second) {
                    report(Severity::Error, actor, "Actor name " + actor->name.str() + " is used by more than one actor");
                }
                if (actor->cls->classFlags & CLASS_Abstract) {
                    report(Severity::Error, actor, "Actor class " + actor->cls->name.str() + " is abstract and cannot exist in a level");
                }
                bool brush = brushClass && actor->isA(brushClass);
                if (brush) {
                    bool builder = actor->cls == brushClass && brush::CsgOper(actor) == csg::CsgActive;
                    if (!builder && !brushHasPolys(actor)) {
                        report(Severity::Error, actor, "Brush has no polygons");
                    }
                } else if (built && actor != info && csg::pointSolid(*model, actor::Location(actor))) {
                    report(Severity::Warning, actor, "Actor is outside the world, inside solid space");
                }
                Name event = actor::Event(actor);
                if (!event.none() && !tags.count(event)) {
                    report(Severity::Warning, actor, "Event " + event.str() + " matches the Tag of no actor");
                }
                if (lightClass && actor->isA(lightClass)) {
                    if (actor::LightType(actor) == 0) {
                        report(Severity::Warning, actor, "Light has no effect, its LightType is LT_None");
                    } else if (actor::LightBrightness(actor) == 0) {
                        report(Severity::Warning, actor, "Light has no effect, its LightBrightness is 0");
                    } else if (actor::LightRadius(actor) == 0) {
                        report(Severity::Warning, actor, "Light has no effect, its LightRadius is 0");
                    }
                }
                if (built && zoneInfoClass && actor->isA(zoneInfoClass) && !(levelInfoClass && actor->isA(levelInfoClass)) && !controlsZone(*model, actor)) {
                    report(Severity::Warning, actor, "ZoneInfo has no effect, it is not inside a zone of its own");
                }
                if (triggerClass && actor->isA(triggerClass) && !actor::bCollideActors(actor) && !events.count(actor::Tag(actor))) {
                    report(Severity::Warning, actor, "Trigger can never fire, its bCollideActors is False and no Event matches its Tag");
                }
            }
            return issues;
        }
    }
}
