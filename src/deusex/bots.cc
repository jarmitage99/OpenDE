#include "deusex/deusex.h"

#include "core/objects.h"
#include "core/vm.h"
#include "engine/actor.h"
#include "engine/net.h"
#include "engine/props_deusex.h"
#include "engine/world.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>

namespace dx::deusex {
    namespace {
        constexpr float MoveInput = 6000.0f;
        constexpr float SightRange = 2500.0f;

        struct BotState {
            UObject* pawn = nullptr;
            int32_t yaw = 0;
            float decideTime = 0;
            float turnTime = 0;
            float stuckTime = 0;
            float strafe = 1;
            float strafeTime = 0;
            float fireTime = 0;
            float armTime = 0;
            float deadTime = 0;
            float doorTime = 0;
        };

        std::vector<BotState> bots;
        size_t wanted = 0;
        size_t spawned = 0;

        float randomUnit()
        {
            static std::mt19937 generator(7);
            return std::uniform_real_distribution<float>(0.0f, 1.0f)(generator);
        }

        UObject* spawnBot(World& world, size_t index)
        {
            UClass* playerClass = cast<UClass>(gameinfo::DefaultPlayerClass(world.gameInfo));
            vm::Call login(world.gameInfo, Name("Login"));
            if (!playerClass || !login.valid()) {
                return nullptr;
            }
            std::string options = "?Name=Bot" + std::to_string(index + 1) + "?Team=" + std::to_string(index % 2);
            login.s("").s(options).s("").obj(playerClass).run();
            UObject* pawn = *reinterpret_cast<UObject**>(login.returnValue());
            if (!pawn) {
                return nullptr;
            }
            UObject* owner = objects::newObject(objects::intrinsicClass(Name("NetConnection")), objects::packageObject(Name("Transient")), Name(), 0);
            playerpawn::Player(pawn) = owner;
            player::Actor(owner) = pawn;
            vm::event(pawn, Name("Possess"));
            vm::event(pawn, Name("TravelPreAccept"));
            vm::Call(world.gameInfo, Name("AcceptInventory")).obj(pawn).run();
            vm::event(pawn, Name("TravelPostAccept"));
            vm::Call(world.gameInfo, Name("PostLogin")).obj(pawn).run();
            vm::Call(pawn, Name("ReceiveFirstOptionSync")).run();
            vm::Call(pawn, Name("ReceiveSecondOptionSync")).run();
            return pawn;
        }

        UObject* carriedWeapon(UObject* pawn)
        {
            int guard = 0;
            for (UObject* item = actor::Inventory(pawn); item && guard++ < 64; item = actor::Inventory(item)) {
                if (item->isA(engineClasses().weapon)) {
                    return item;
                }
            }
            return nullptr;
        }

        void give(World& world, UObject* pawn, const char* className)
        {
            UObject* item = world.spawn(objects::findClass(className), nullptr, Name(), actor::Location(pawn), {}, nullptr, true);
            if (!item) {
                return;
            }
            vm::Call(item, Name("Frob")).obj(pawn).obj(nullptr).run();
            if (!item->destroyed) {
                world.destroy(item);
            }
        }

        void arm(World& world, UObject* pawn)
        {
            if (!carriedWeapon(pawn)) {
                give(world, pawn, "DeusEx.WeaponPistol");
                give(world, pawn, "DeusEx.Ammo10mm");
            }
            UObject* weapon = carriedWeapon(pawn);
            if (weapon) {
                vm::Call(pawn, Name("PutInHand")).obj(weapon).run();
            }
        }

        Vector eyeOf(UObject* pawn)
        {
            return actor::Location(pawn) + Vector { 0, 0, pawn::BaseEyeHeight(pawn) };
        }

        UObject* findEnemy(World& world, UObject* pawn)
        {
            bool teams = gameinfo::bTeamGame(world.gameInfo) != 0;
            UObject* info = pawn::PlayerReplicationInfo(pawn);
            UObject* best = nullptr;
            float bestDistance = SightRange * SightRange;
            for (UObject* other : world.actors) {
                if (!other || other == pawn || other->destroyed || !isPlayerPawn(other) || pawn::Health(other) <= 0 || actor::bHidden(other)) {
                    continue;
                }
                UObject* otherInfo = pawn::PlayerReplicationInfo(other);
                if (teams && info && otherInfo && playerreplicationinfo::Team(info) == playerreplicationinfo::Team(otherInfo)) {
                    continue;
                }
                float distance = (actor::Location(other) - actor::Location(pawn)).sizeSquared();
                if (distance < bestDistance && world.fastLineCheck(actor::Location(other), eyeOf(pawn))) {
                    best = other;
                    bestDistance = distance;
                }
            }
            return best;
        }

        float clearance(World& world, UObject* pawn, int32_t yaw, float range, UObject** blocker)
        {
            float angle = static_cast<float>(yaw) * Pi / 32768.0f;
            Vector start = actor::Location(pawn);
            Vector end = start + Vector { std::cos(angle) * range, std::sin(angle) * range, 0 };
            CheckResult hit;
            if (!world.lineCheck(hit, start, end, { 12, 12, 20 }, pawn, TRACE_World)) {
                return range;
            }
            if (blocker) {
                *blocker = hit.actor;
            }
            return hit.time * range;
        }

        int32_t pickDirection(World& world, UObject* pawn)
        {
            int32_t offset = static_cast<int32_t>(randomUnit() * 65536.0f);
            int32_t best = offset;
            float bestClearance = 0;
            int32_t open[12];
            int openCount = 0;
            for (int i = 0; i < 12; i++) {
                int32_t yaw = (offset + i * 65536 / 12) & 65535;
                float clear = clearance(world, pawn, yaw, 900.0f, nullptr);
                if (clear > 300.0f) {
                    open[openCount++] = yaw;
                }
                if (clear > bestClearance) {
                    bestClearance = clear;
                    best = yaw;
                }
            }
            if (openCount > 0) {
                return open[static_cast<int>(randomUnit() * static_cast<float>(openCount)) % openCount];
            }
            return best;
        }

        void fight(BotState& bot, UObject* enemy, float deltaSeconds)
        {
            UObject* pawn = bot.pawn;
            Vector delta = actor::Location(enemy) - eyeOf(pawn);
            float flat = std::sqrt(delta.x * delta.x + delta.y * delta.y);
            Rotator view;
            view.yaw = (static_cast<int32_t>(std::atan2(delta.y, delta.x) * 32768.0f / Pi) + static_cast<int32_t>((randomUnit() - 0.5f) * 500.0f)) & 65535;
            view.pitch = static_cast<int32_t>(std::atan2(delta.z, flat) * 32768.0f / Pi) & 65535;
            pawn::ViewRotation(pawn) = view;
            bot.yaw = view.yaw;
            if (flat > 600.0f) {
                playerpawn::aForward(pawn) = MoveInput;
            } else if (flat < 200.0f) {
                playerpawn::aForward(pawn) = -MoveInput;
            }
            bot.strafeTime -= deltaSeconds;
            if (bot.strafeTime <= 0) {
                bot.strafe = randomUnit() < 0.5f ? -1.0f : 1.0f;
                bot.strafeTime = 0.5f + randomUnit();
            }
            playerpawn::aStrafe(pawn) = bot.strafe * MoveInput;
            bot.fireTime -= deltaSeconds;
            if (bot.fireTime <= 0 && pawn::Weapon(pawn)) {
                vm::Call(pawn::Weapon(pawn), Name("ServerForceFire")).run();
                bot.fireTime = 0.3f + randomUnit() * 0.4f;
            }
        }

        void wander(World& world, BotState& bot, float deltaSeconds)
        {
            UObject* pawn = bot.pawn;
            Vector velocity = actor::Velocity(pawn);
            if (velocity.x * velocity.x + velocity.y * velocity.y < 1600.0f) {
                bot.stuckTime += deltaSeconds;
            } else {
                bot.stuckTime = 0;
            }
            bot.decideTime -= deltaSeconds;
            bot.doorTime -= deltaSeconds;
            if (bot.decideTime <= 0 || bot.stuckTime > 0.4f) {
                UObject* blocker = nullptr;
                bool blocked = clearance(world, pawn, bot.yaw, 150.0f, &blocker) < 150.0f;
                if (blocked && isMover(blocker) && bot.doorTime <= 0 && mover::KeyNum(blocker) == 0 && !actor::bInterpolating(blocker)) {
                    vm::Call(blocker, Name("Frob")).obj(pawn).obj(nullptr).run();
                    if (actor::bInterpolating(blocker)) {
                        bot.doorTime = mover::MoveTime(blocker) + 0.5f;
                    }
                }
                bot.turnTime -= 0.25f;
                if (bot.doorTime > 0) {
                    bot.stuckTime = 0;
                } else if (blocked || bot.stuckTime > 0.4f || bot.turnTime <= 0) {
                    bot.yaw = pickDirection(world, pawn);
                    bot.turnTime = 3.0f + randomUnit() * 4.0f;
                    bot.stuckTime = 0;
                }
                bot.decideTime = 0.25f;
            }
            pawn::ViewRotation(pawn) = { 0, bot.yaw, 0 };
            playerpawn::aForward(pawn) = MoveInput;
        }

        void think(World& world, BotState& bot, float deltaSeconds)
        {
            UObject* pawn = bot.pawn;
            playerpawn::aForward(pawn) = 0;
            playerpawn::aStrafe(pawn) = 0;
            static const Name nameDying("Dying");
            if (vm::stateName(pawn) == nameDying) {
                bot.deadTime += deltaSeconds;
                if (bot.deadTime > 3.0f) {
                    bot.deadTime = 0;
                    vm::Call(pawn, Name("Fire")).f(0).run();
                }
                return;
            }
            bot.deadTime = 0;
            bot.armTime -= deltaSeconds;
            if (!deusexplayer::inHand(pawn) && bot.armTime <= 0) {
                arm(world, pawn);
                bot.armTime = 2.0f;
            }
            UObject* enemy = findEnemy(world, pawn);
            if (enemy) {
                fight(bot, enemy, deltaSeconds);
            } else {
                wander(world, bot, deltaSeconds);
            }
        }
    }

    bool isBot(UObject* pawn)
    {
        for (const BotState& bot : bots) {
            if (bot.pawn == pawn) {
                return true;
            }
        }
        return false;
    }

    void resetBots(World& world)
    {
        bots.clear();
        spawned = 0;
        wanted = 0;
        std::string url = world.url;
        std::transform(url.begin(), url.end(), url.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        size_t option = url.find("?bots=");
        if (option != std::string::npos && (world.netMode == NM_DedicatedServer || world.netMode == NM_ListenServer)) {
            wanted = static_cast<size_t>(std::max(0, std::atoi(url.c_str() + option + 6)));
        }
    }

    void tickBots(World& world, float deltaSeconds)
    {
        std::erase_if(bots, [](const BotState& bot) {
            return bot.pawn->destroyed;
        });
        if (bots.size() < wanted && world.timeSeconds > 1.0f) {
            UObject* pawn = spawnBot(world, spawned++);
            if (pawn) {
                BotState bot;
                bot.pawn = pawn;
                bots.push_back(bot);
            } else {
                wanted = bots.size();
            }
        }
        for (BotState& bot : bots) {
            think(world, bot, deltaSeconds);
        }
    }
}
