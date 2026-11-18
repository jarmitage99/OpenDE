#include "core/objects.h"
#include "core/vm.h"
#include "deusex/deusex.h"
#include "engine/actor.h"
#include "engine/console.h"
#include "engine/net.h"
#include "engine/props_deusex.h"
#include "engine/props_extension.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {
    struct SentMessage {
        uint32_t connection = 0;
        std::vector<uint8_t> data;
        bool reliable = false;
    };

    class RecordingTransport : public dx::net::Transport {
    public:
        void send(uint32_t connection, const std::vector<uint8_t>& data, bool reliable) override
        {
            sent.push_back({ connection, data, reliable });
        }

        void close(uint32_t connection) override
        {
            closed.push_back(connection);
        }

        std::vector<dx::net::Event> poll() override
        {
            std::vector<dx::net::Event> result;
            result.swap(pending);
            return result;
        }

        std::string address(uint32_t) override
        {
            return "10.1.2.3:4567";
        }

        int ping(uint32_t) override
        {
            return 42;
        }

        int loss(uint32_t) override
        {
            return 7;
        }

        void connect(uint32_t connection)
        {
            dx::net::Event event;
            event.kind = dx::net::Event::Kind::Connected;
            event.connection = connection;
            pending.push_back(event);
        }

        void disconnect(uint32_t connection)
        {
            dx::net::Event event;
            event.kind = dx::net::Event::Kind::Disconnected;
            event.connection = connection;
            pending.push_back(event);
        }

        void receive(uint32_t connection, std::vector<uint8_t> data)
        {
            dx::net::Event event;
            event.kind = dx::net::Event::Kind::Message;
            event.connection = connection;
            event.data = std::move(data);
            pending.push_back(event);
        }

        std::vector<SentMessage> sent;
        std::vector<uint32_t> closed;
        std::vector<dx::net::Event> pending;
    };

    std::vector<uint8_t> joinMessage(const std::string& options)
    {
        std::vector<uint8_t> message { 3, static_cast<uint8_t>(options.size()) };
        message.insert(message.end(), options.begin(), options.end());
        return message;
    }

    std::vector<dx::UObject*> playerPawns(dx::World& world)
    {
        std::vector<dx::UObject*> pawns;
        for (dx::UObject* a : world.actors) {
            if (a && !a->destroyed && dx::isPlayerPawn(a)) {
                pawns.push_back(a);
            }
        }
        return pawns;
    }

    size_t countType(const RecordingTransport& transport, uint8_t type)
    {
        size_t count = 0;
        for (const SentMessage& message : transport.sent) {
            if (!message.data.empty() && message.data[0] == type) {
                count++;
            }
        }
        return count;
    }
}

TEST_CASE("packets use little endian fields and seven bit variable length integers")
{
    dx::net::Packet packet;
    packet.u8(7);
    packet.varint(300);
    packet.i32(-2);
    packet.f32(1.0f);
    packet.str("ab");
    std::vector<uint8_t> expected { 7, 0xAC, 0x02, 0xFE, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x80, 0x3F, 2, 'a', 'b' };
    REQUIRE(packet.data == expected);

    dx::net::PacketReader reader(packet.data.data(), packet.data.size());
    CHECK(reader.u8() == 7);
    CHECK(reader.varint() == 300);
    CHECK(reader.i32() == -2);
    CHECK(reader.f32() == 1.0f);
    CHECK(reader.str() == "ab");
    CHECK(reader.atEnd());
    CHECK_FALSE(reader.failed);
}

TEST_CASE("reading past the end of a packet fails instead of returning stale bytes")
{
    std::vector<uint8_t> truncated { 5, 'a', 'b' };
    dx::net::PacketReader text(truncated.data(), truncated.size());
    CHECK(text.str().empty());
    CHECK(text.failed);

    std::vector<uint8_t> shortInt { 1, 2, 3 };
    dx::net::PacketReader number(shortInt.data(), shortInt.size());
    CHECK(number.i32() == 0);
    CHECK(number.failed);

    std::vector<uint8_t> endless { 0x80, 0x80, 0x80, 0x80, 0x80, 0x80 };
    dx::net::PacketReader varint(endless.data(), endless.size());
    CHECK(varint.varint() == 0);
    CHECK(varint.failed);
}

TEST_CASE("server welcomes a client, logs it in on join and removes its pawn on disconnect", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::unloadPackage(dx::Name("DXMP_Smuggler"));
    RecordingTransport transport;
    {
        dx::World world;
        world.netMode = dx::NM_DedicatedServer;
        dx::net::Driver driver(transport, true);
        driver.setWorld(&world);
        REQUIRE(world.loadMap("DXMP_Smuggler?game=DeusEx.DeathMatchGame"));
        CHECK(world.player() == nullptr);
        CHECK(playerPawns(world).empty());
        CHECK(*world.levelInfo->propertyAddress(dx::Name("NetMode")) == 1);

        transport.connect(9);
        transport.receive(9, { 1, 1 });
        world.tick(0.05f);
        driver.tick(0.05f);
        REQUIRE(transport.sent.size() == 1);
        std::vector<uint8_t> welcome { 2, 13, 'D', 'X', 'M', 'P', '_', 'S', 'm', 'u', 'g', 'g', 'l', 'e', 'r' };
        CHECK(transport.sent[0].data == welcome);
        CHECK(transport.sent[0].connection == 9);
        CHECK(transport.sent[0].reliable);

        transport.receive(9, joinMessage("?Name=Tester"));
        world.tick(0.05f);
        driver.tick(0.05f);
        std::vector<dx::UObject*> pawns = playerPawns(world);
        REQUIRE(pawns.size() == 1);
        dx::UObject* pawn = pawns[0];
        CHECK(dx::playerreplicationinfo::PlayerName(dx::pawn::PlayerReplicationInfo(pawn)) == "Tester");
        CHECK(dx::actor::Role(pawn) == 4);
        CHECK(dx::actor::RemoteRole(pawn) == 3);
        CHECK(dx::vm::stateName(pawn) == dx::Name("PlayerWalking"));
        REQUIRE(dx::playerpawn::Player(pawn) != nullptr);
        CHECK(dx::playerpawn::Player(pawn)->cls->name == dx::Name("NetConnection"));
        CHECK(driver.playerAddress(pawn) == "10.1.2.3:4567");
        CHECK(dx::console::execute(pawn, "GETPING") == "42");
        CHECK(dx::console::execute(pawn, "GETLOSS") == "7");
        CHECK(countType(transport, 6) > 0);
        CHECK(countType(transport, 4) == 0);
        dx::UObject* hud = dx::playerpawnext::RootWindow(pawn);
        REQUIRE(hud != nullptr);
        CHECK_FALSE(hud->destroyed);

        transport.disconnect(9);
        world.tick(0.05f);
        driver.tick(0.05f);
        CHECK(playerPawns(world).empty());
        CHECK(driver.connectionCount() == 0);
        CHECK(hud->destroyed);
        driver.setWorld(nullptr);
    }
    dx::objects::unloadPackage(dx::Name("DXMP_Smuggler"));
}

TEST_CASE("server rejects a client that speaks another protocol version", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::unloadPackage(dx::Name("DXMP_Smuggler"));
    RecordingTransport transport;
    {
        dx::World world;
        world.netMode = dx::NM_DedicatedServer;
        dx::net::Driver driver(transport, true);
        driver.setWorld(&world);
        REQUIRE(world.loadMap("DXMP_Smuggler?game=DeusEx.DeathMatchGame"));
        transport.connect(4);
        transport.receive(4, { 1, 99 });
        transport.receive(4, joinMessage("?Name=Intruder"));
        world.tick(0.05f);
        driver.tick(0.05f);
        REQUIRE(transport.sent.size() == 1);
        CHECK(transport.sent[0].data[0] == 4);
        CHECK(std::string(transport.sent[0].data.begin() + 2, transport.sent[0].data.end()) == "Incompatible game version");
        CHECK(playerPawns(world).empty());
        driver.setWorld(nullptr);
    }
    dx::objects::unloadPackage(dx::Name("DXMP_Smuggler"));
}

TEST_CASE("listen server host spawn immunity lasts six seconds", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::unloadPackage(dx::Name("DXMP_Smuggler"));
    {
        dx::World world;
        world.netMode = dx::NM_ListenServer;
        REQUIRE(world.loadMap("DXMP_Smuggler?game=DeusEx.DeathMatchGame?listen"));
        dx::UObject* host = world.player();
        REQUIRE(host != nullptr);
        for (int i = 0; i < 20; i++) {
            world.tick(0.05f);
        }
        float left = dx::deusexplayer::NintendoImmunityTimeLeft(host);
        CHECK(left > 4.0f);
        CHECK(left < 6.0f);
        for (int i = 0; i < 120; i++) {
            world.tick(0.05f);
        }
        CHECK(dx::deusexplayer::NintendoImmunityTimeLeft(host) == 0.0f);
    }
    dx::objects::unloadPackage(dx::Name("DXMP_Smuggler"));
}

TEST_CASE("server speed option scales player movement and reaches clients", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::unloadPackage(dx::Name("DXMP_Smuggler"));
    dx::UObject* jcDefaults = dx::objects::findClass("DeusEx.JCDentonMale")->defaultObject;
    float& defaultGround = *reinterpret_cast<float*>(jcDefaults->propertyAddress(dx::Name("mpGroundSpeed")));
    RecordingTransport transport;
    {
        dx::World world;
        world.netMode = dx::NM_DedicatedServer;
        dx::net::Driver driver(transport, true);
        driver.setWorld(&world);
        REQUIRE(world.loadMap("DXMP_Smuggler?game=DeusEx.DeathMatchGame?Speed=150"));
        transport.connect(9);
        transport.receive(9, { 1, 1 });
        world.tick(0.05f);
        driver.tick(0.05f);
        REQUIRE(transport.sent.size() == 1);
        CHECK(std::string(transport.sent[0].data.begin() + 2, transport.sent[0].data.end()) == "DXMP_Smuggler?Speed=150");
        transport.receive(9, joinMessage("?Name=Tester"));
        world.tick(0.05f);
        driver.tick(0.05f);
        std::vector<dx::UObject*> pawns = playerPawns(world);
        REQUIRE(pawns.size() == 1);
        CHECK(dx::pawn::GroundSpeed(pawns[0]) == 345.0f);
        CHECK(dx::pawn::WaterSpeed(pawns[0]) == 165.0f);
        driver.setWorld(nullptr);
    }
    dx::objects::unloadPackage(dx::Name("DXMP_Smuggler"));
    {
        dx::World world;
        world.netMode = dx::NM_Client;
        REQUIRE(world.loadMap("DXMP_Smuggler?Speed=50"));
        CHECK(defaultGround == 115.0f);
    }
    dx::objects::unloadPackage(dx::Name("DXMP_Smuggler"));
    {
        dx::World world;
        world.netMode = dx::NM_DedicatedServer;
        REQUIRE(world.loadMap("DXMP_Smuggler?game=DeusEx.DeathMatchGame"));
        CHECK(defaultGround == 230.0f);
    }
    dx::objects::unloadPackage(dx::Name("DXMP_Smuggler"));
}

TEST_CASE("bot opens the doors of a corridor without closing them again", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::unloadPackage(dx::Name("DXMP_Smuggler"));
    {
        dx::World world;
        world.netMode = dx::NM_DedicatedServer;
        REQUIRE(world.loadMap("DXMP_Smuggler?game=DeusEx.DeathMatchGame?Bots=1"));
        for (int i = 0; i < 30 && playerPawns(world).empty(); i++) {
            world.tick(0.05f);
        }
        std::vector<dx::UObject*> pawns = playerPawns(world);
        REQUIRE(pawns.size() == 1);
        REQUIRE(world.farMoveActor(pawns[0], { 620.0f, -166.0f, 15.75f }, false, false));
        std::vector<dx::UObject*> doors;
        for (dx::UObject* a : world.actors) {
            if (a && dx::isMover(a) && std::abs(dx::actor::Location(a).x - 656.0f) < 1.0f && (std::abs(dx::actor::Location(a).y + 80.0f) < 1.0f || std::abs(dx::actor::Location(a).y + 256.0f) < 1.0f)) {
                doors.push_back(a);
            }
        }
        REQUIRE(doors.size() == 2);
        int opened = 0;
        int closedAgain = 0;
        std::vector<uint8_t> previous { 0, 0 };
        for (int i = 0; i < 160; i++) {
            world.tick(0.05f);
            for (size_t d = 0; d < doors.size(); d++) {
                uint8_t key = dx::mover::KeyNum(doors[d]);
                if (previous[d] == 0 && key == 1) {
                    opened++;
                }
                if (previous[d] == 1 && key == 0) {
                    closedAgain++;
                }
                previous[d] = key;
            }
        }
        CHECK(opened > 0);
        CHECK(closedAgain == 0);
    }
    dx::objects::unloadPackage(dx::Name("DXMP_Smuggler"));
}

TEST_CASE("client level keeps only static actors and runs no game", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::unloadPackage(dx::Name("DXMP_Smuggler"));
    RecordingTransport transport;
    {
        dx::World world;
        world.netMode = dx::NM_Client;
        dx::net::Driver driver(transport, false);
        driver.setWorld(&world);
        REQUIRE(world.loadMap("DXMP_Smuggler"));
        CHECK(world.gameInfo == nullptr);
        CHECK(world.player() == nullptr);
        CHECK(*world.levelInfo->propertyAddress(dx::Name("NetMode")) == 3);
        size_t movable = 0;
        size_t authoritative = 0;
        for (size_t i = 2; i < world.actors.size(); i++) {
            dx::UObject* a = world.actors[i];
            if (!a || a->destroyed) {
                continue;
            }
            if (!dx::actor::bStatic(a) && !dx::actor::bNoDelete(a)) {
                movable++;
            }
            if (dx::actor::Role(a) == 4 && dx::actor::RemoteRole(a) != 0) {
                authoritative++;
            }
        }
        CHECK(world.actors.size() > 100);
        CHECK(movable == 0);
        CHECK(authoritative == 0);
        driver.setWorld(nullptr);
    }
    dx::objects::unloadPackage(dx::Name("DXMP_Smuggler"));
}

TEST_CASE("bots join a hosted game, arm themselves, roam and score kills", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::unloadPackage(dx::Name("DXMP_Smuggler"));
    {
        dx::World world;
        world.netMode = dx::NM_DedicatedServer;
        REQUIRE(world.loadMap("DXMP_Smuggler?game=DeusEx.DeathMatchGame?Bots=6"));
        CHECK(playerPawns(world).empty());
        for (int i = 0; i < 60; i++) {
            world.tick(0.05f);
        }
        std::vector<dx::UObject*> pawns = playerPawns(world);
        REQUIRE(pawns.size() == 6);
        std::vector<dx::Vector> starts;
        for (dx::UObject* pawn : pawns) {
            CHECK(dx::deusex::isBot(pawn));
            CHECK(dx::playerreplicationinfo::PlayerName(dx::pawn::PlayerReplicationInfo(pawn)).rfind("Bot", 0) == 0);
            starts.push_back(dx::actor::Location(pawn));
        }
        float score = 0;
        for (int i = 0; i < 2400 && score < 1.0f; i++) {
            world.tick(0.05f);
            score = 0;
            for (dx::UObject* pawn : playerPawns(world)) {
                score += dx::playerreplicationinfo::Score(dx::pawn::PlayerReplicationInfo(pawn));
            }
        }
        CHECK(score >= 1.0f);
        pawns = playerPawns(world);
        REQUIRE(pawns.size() == 6);
        size_t moved = 0;
        size_t armed = 0;
        for (size_t i = 0; i < pawns.size(); i++) {
            dx::Vector delta = dx::actor::Location(pawns[i]) - starts[i];
            if (delta.x * delta.x + delta.y * delta.y > 100.0f * 100.0f) {
                moved++;
            }
            dx::UObject* held = dx::deusexplayer::inHand(pawns[i]);
            if (held && held->isA(dx::Name("WeaponPistol"))) {
                armed++;
            }
        }
        CHECK(moved >= 5);
        CHECK(armed >= 4);
    }
    dx::objects::unloadPackage(dx::Name("DXMP_Smuggler"));
}

TEST_CASE("no bots are added without the option or outside a hosted game", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::unloadPackage(dx::Name("DXMP_Smuggler"));
    {
        dx::World world;
        world.netMode = dx::NM_DedicatedServer;
        REQUIRE(world.loadMap("DXMP_Smuggler?game=DeusEx.DeathMatchGame"));
        for (int i = 0; i < 60; i++) {
            world.tick(0.05f);
        }
        CHECK(playerPawns(world).empty());
    }
    dx::objects::unloadPackage(dx::Name("DXMP_Smuggler"));
    {
        dx::World world;
        REQUIRE(world.loadMap("DXMP_Smuggler?game=DeusEx.DeathMatchGame?Bots=3"));
        for (int i = 0; i < 60; i++) {
            world.tick(0.05f);
        }
        CHECK(playerPawns(world).size() == 1);
    }
    dx::objects::unloadPackage(dx::Name("DXMP_Smuggler"));
}

#ifndef _WIN32
namespace {
    std::string queryServer(dx::World& world, int port, const std::string& request)
    {
        int handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        sockaddr_in target {};
        target.sin_family = AF_INET;
        target.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        target.sin_port = htons(static_cast<uint16_t>(port));
        sendto(handle, request.data(), request.size(), 0, reinterpret_cast<sockaddr*>(&target), sizeof(target));
        std::string reply;
        for (int attempt = 0; attempt < 40 && reply.empty(); attempt++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            world.tick(0.05f);
            char buffer[2048];
            ssize_t count = recv(handle, buffer, sizeof(buffer), MSG_DONTWAIT);
            if (count > 0) {
                reply.assign(buffer, static_cast<size_t>(count));
            }
        }
        close(handle);
        return reply;
    }

    dx::UObject* firstActor(dx::World& world, const char* className)
    {
        for (dx::UObject* a : world.actors) {
            if (a && !a->destroyed && a->isA(dx::Name(className))) {
                return a;
            }
        }
        return nullptr;
    }
}

TEST_CASE("a hosted level answers LAN beacon and status queries", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::unloadPackage(dx::Name("DXMP_Smuggler"));
    {
        dx::World world;
        world.netMode = dx::NM_DedicatedServer;
        world.netPort = 7790;
        REQUIRE(world.loadMap("DXMP_Smuggler?game=DeusEx.DeathMatchGame"));
        dx::UObject* beacon = firstActor(world, "UdpBeacon");
        dx::UObject* query = firstActor(world, "UdpServerQuery");
        REQUIRE(beacon != nullptr);
        REQUIRE(query != nullptr);
        CHECK(firstActor(world, "UdpServerUplink") == nullptr);
        int beaconPort = *reinterpret_cast<int32_t*>(beacon->propertyAddress(dx::Name("Port")));
        int queryPort = *reinterpret_cast<int32_t*>(query->propertyAddress(dx::Name("Port")));
        REQUIRE(beaconPort >= 8777);
        REQUIRE(queryPort > 0);

        CHECK(queryServer(world, beaconPort, "REPORTQUERY") == "DeusEx " + std::to_string(queryPort));
        std::string info = queryServer(world, queryPort, "\\info\\");
        CHECK(info.find("\\mapname\\DXMP_Smuggler") != std::string::npos);
        CHECK(info.find("\\hostport\\7790") != std::string::npos);
        CHECK(info.find("\\numplayers\\0") != std::string::npos);
    }
    dx::objects::unloadPackage(dx::Name("DXMP_Smuggler"));
}
#endif

#if defined(DEUSEX_PATH) && !defined(_WIN32)
namespace {
    class GameProcess {
    public:
        GameProcess(const std::string& arguments, const std::filesystem::path& logFile)
            : log(logFile)
        {
            std::string command = "DEUSEX_GAME_DATA=\"" DEUSEX_GAME_DATA "\" XDG_DATA_HOME=\"" + log.parent_path().string() + "\" \"" DEUSEX_PATH "\" " + arguments + " --control > \"" + log.string() + "\" 2>&1";
            pipe = popen(command.c_str(), "w");
        }

        ~GameProcess()
        {
            send("exec quit");
            pclose(pipe);
        }

        GameProcess(const GameProcess&) = delete;
        GameProcess& operator=(const GameProcess&) = delete;

        void send(const std::string& line)
        {
            std::fprintf(pipe, "%s\n", line.c_str());
            std::fflush(pipe);
        }

        std::string output() const
        {
            std::ifstream file(log);
            std::stringstream text;
            text << file.rdbuf();
            return text.str();
        }

        std::vector<std::string> ask(const std::string& command, const std::string& prefix)
        {
            size_t mark = output().size();
            send(command);
            std::vector<std::string> lines;
            for (int attempt = 0; attempt < 200; attempt++) {
                std::this_thread::sleep_for(std::chrono::milliseconds(25));
                std::string reply = output().substr(mark);
                if (reply.find("done\n") == std::string::npos) {
                    continue;
                }
                std::istringstream text(reply);
                for (std::string line; std::getline(text, line);) {
                    if (line.rfind(prefix, 0) == 0) {
                        lines.push_back(line);
                    }
                }
                break;
            }
            return lines;
        }

    private:
        std::filesystem::path log;
        FILE* pipe = nullptr;
    };

    struct ScratchDirectory {
        ScratchDirectory()
            : path(std::filesystem::temp_directory_path() / ("deusex-net-test-" + std::to_string(getpid())))
        {
            std::filesystem::remove_all(path);
            std::filesystem::create_directories(path);
        }

        ~ScratchDirectory()
        {
            std::error_code error;
            std::filesystem::remove_all(path, error);
        }

        std::filesystem::path path;
    };

    bool eventually(const std::function<bool()>& condition, int seconds = 30)
    {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
        while (std::chrono::steady_clock::now() < deadline) {
            if (condition()) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        return false;
    }

    bool anyContains(const std::vector<std::string>& lines, const std::string& text)
    {
        for (const std::string& line : lines) {
            if (line.find(text) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    bool pawnPosition(GameProcess& process, const std::string& name, float& x, float& y)
    {
        std::string prefix = "pawn JCDentonMale name " + name + " role ";
        for (const std::string& line : process.ask("players", prefix)) {
            int role = 0;
            float z = 0;
            if (std::sscanf(line.c_str() + prefix.size(), "%d at %f %f %f", &role, &x, &y, &z) == 4) {
                return true;
            }
        }
        return false;
    }

    std::string value(const std::vector<std::string>& lines, const std::string& marker, const std::string& key)
    {
        for (const std::string& line : lines) {
            size_t at = line.find(key + "=");
            if (line.find(marker) != std::string::npos && at != std::string::npos) {
                size_t start = at + key.size() + 1;
                return line.substr(start, line.find(' ', start) - start);
            }
        }
        return {};
    }
}

TEST_CASE("two clients play on a dedicated server over the network", "[data][net]")
{
    ScratchDirectory scratch;
    const std::filesystem::path& directory = scratch.path;
    std::string port = std::to_string(20000 + getpid() % 20000);
    {
        GameProcess server("--server \"DXMP_Smuggler?game=DeusEx.DeathMatchGame\" --port " + port, directory / "server.log");
        REQUIRE(eventually([&]() {
            return server.output().find("server running") != std::string::npos;
        },
            120));
        GameProcess alice("\"127.0.0.1:" + port + "?Name=Alice\" --headless", directory / "alice.log");
        GameProcess bob("\"127.0.0.1:" + port + "?Name=Bob\" --headless", directory / "bob.log");
        REQUIRE(eventually([&]() {
            return anyContains(alice.ask("players", "pawn "), "name Alice role 3") && anyContains(bob.ask("players", "pawn "), "name Bob role 3");
        },
            120));
        REQUIRE(anyContains(server.ask("players", "pawn "), "name Alice role 4"));

        SECTION("movement reaches the server and the other client")
        {
            REQUIRE_FALSE(server.ask("place Alice PlayerStart 0 60", "placed").empty());
            REQUIRE_FALSE(server.ask("place Bob PlayerStart 0 200", "placed").empty());
            float startX = 0;
            float startY = 0;
            REQUIRE(eventually([&]() {
                float x = 0;
                float y = 0;
                return pawnPosition(server, "Alice", startX, startY) && pawnPosition(alice, "Alice", x, y) && std::abs(x - startX) + std::abs(y - startY) < 1.0f;
            }));
            alice.send("press W");
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));
            alice.send("release W");
            float serverX = 0;
            float serverY = 0;
            CHECK(eventually([&]() {
                float ownX = 0;
                float ownY = 0;
                float seenX = 0;
                float seenY = 0;
                if (!pawnPosition(server, "Alice", serverX, serverY) || !pawnPosition(alice, "Alice", ownX, ownY) || !pawnPosition(bob, "Alice", seenX, seenY)) {
                    return false;
                }
                bool moved = std::abs(serverX - startX) + std::abs(serverY - startY) > 20.0f;
                return moved && std::abs(serverX - ownX) + std::abs(serverY - ownY) < 3.0f && std::abs(serverX - seenX) + std::abs(serverY - seenY) < 3.0f;
            }));
        }

        SECTION("a door opened on the server opens on both clients")
        {
            std::string query = "dump DeusExMover KeyNum Rotation";
            std::vector<std::string> closed = server.ask(query, "actor ");
            REQUIRE(closed.size() > 1);
            REQUIRE(closed[1] == "actor DeusExMover KeyNum=0 Rotation=(Pitch=0,Yaw=-16384,Roll=0)");
            REQUIRE_FALSE(server.ask("frob Alice DeusExMover 1", "frobbed").empty());
            std::string open = "actor DeusExMover KeyNum=1 Rotation=(Pitch=0,Yaw=-32768,Roll=0)";
            for (GameProcess* process : { &server, &alice, &bob }) {
                CHECK(eventually([&]() {
                    std::vector<std::string> rows = process->ask(query, "actor ");
                    return rows.size() > 1 && rows[1] == open;
                }));
            }
        }

        SECTION("a client picks up a weapon by frobbing it")
        {
            REQUIRE_FALSE(server.ask("place Alice WeaponPistol 0 45", "placed").empty());
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));
            REQUIRE_FALSE(server.ask("face Alice WeaponPistol 0", "placed").empty());
            REQUIRE(eventually([&]() {
                return anyContains(server.ask("dump DeusExPlayer FrobTarget", "actor "), "FrobTarget=WeaponPistol");
            }));
            CHECK_FALSE(anyContains(server.ask("dump WeaponPistol Owner", "actor "), "Owner=JCDentonMale"));
            alice.send("press RightMouse");
            alice.send("release RightMouse");
            for (GameProcess* process : { &server, &alice }) {
                CHECK(eventually([&]() {
                    return anyContains(process->ask("dump WeaponPistol Owner", "actor "), "Owner=JCDentonMale");
                }));
            }
        }

        SECTION("augmentations and skills bought by a client change on the server and back")
        {
            REQUIRE_FALSE(server.ask("give Alice DeusEx.AugSpeed", "gave").empty());
            std::string hotKey;
            REQUIRE(eventually([&]() {
                hotKey = value(alice.ask("dump AugSpeed bHasIt bIsActive HotKeyNum", "actor "), "bHasIt=True bIsActive=False", "HotKeyNum");
                return !hotKey.empty();
            }));
            alice.send("exec ActivateAugmentation " + std::to_string(std::atoi(hotKey.c_str()) - 3));
            for (GameProcess* process : { &server, &alice }) {
                CHECK(eventually([&]() {
                    return anyContains(process->ask("dump AugSpeed bHasIt bIsActive", "actor "), "bHasIt=True bIsActive=True");
                }));
            }
            CHECK(eventually([&]() {
                std::string energy = value(alice.ask("dump DeusExPlayer Energy Role", "actor "), "Role=ROLE_AutonomousProxy", "Energy");
                return !energy.empty() && std::atof(energy.c_str()) < 99.0;
            }));

            REQUIRE(anyContains(server.ask("dump DeusExPlayer SkillPointsAvail", "actor "), "SkillPointsAvail=2000"));
            REQUIRE(anyContains(alice.ask("dump SkillWeaponHeavy CurrentLevel", "actor "), "CurrentLevel=1"));
            alice.send("exec BuySkills");
            REQUIRE(eventually([&]() {
                return anyContains(alice.ask("dump DeusExPlayer bBuySkills", "actor "), "bBuySkills=True");
            }));
            alice.send("exec ActivateBelt 1");
            for (GameProcess* process : { &server, &alice }) {
                CHECK(eventually([&]() {
                    return anyContains(process->ask("dump SkillWeaponHeavy CurrentLevel", "actor "), "CurrentLevel=2") && anyContains(process->ask("dump DeusExPlayer SkillPointsAvail", "actor "), "SkillPointsAvail=0");
                }));
            }
        }

        SECTION("a thrown grenade flies on every machine and its explosion kills the thrower")
        {
            REQUIRE_FALSE(server.ask("place Alice PlayerStart 0 60", "placed").empty());
            REQUIRE_FALSE(server.ask("place Bob PlayerStart 0 200", "placed").empty());
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));
            REQUIRE_FALSE(server.ask("face Alice PlayerStart 2 0 60", "placed").empty());
            REQUIRE_FALSE(server.ask("give Alice DeusEx.WeaponLAM", "gave").empty());
            std::string beltPos;
            REQUIRE(eventually([&]() {
                beltPos = value(alice.ask("dump WeaponLAM Owner beltPos", "actor "), "Owner=JCDentonMale", "beltPos");
                return !beltPos.empty();
            }));
            alice.send("exec ActivateBelt " + beltPos);
            REQUIRE(eventually([&]() {
                return anyContains(server.ask("dump DeusExPlayer inHand", "actor "), "inHand=WeaponLAM");
            }));
            CHECK(server.ask("dump ThrownProjectile Role", "actor ").empty());
            REQUIRE(eventually([&]() {
                alice.send("press LeftMouse");
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                alice.send("release LeftMouse");
                return anyContains(server.ask("dump ThrownProjectile Role", "actor "), "LAM Role=ROLE_Authority");
            }));
            bool seenByThrower = false;
            bool seenByObserver = false;
            CHECK(eventually([&]() {
                seenByThrower = seenByThrower || anyContains(alice.ask("dump ThrownProjectile Role", "actor "), "LAM Role=ROLE_SimulatedProxy");
                seenByObserver = seenByObserver || anyContains(bob.ask("dump ThrownProjectile Role", "actor "), "LAM Role=ROLE_SimulatedProxy");
                return seenByThrower && seenByObserver;
            },
                5));
            for (GameProcess* process : { &server, &alice }) {
                CHECK(eventually([&]() {
                    return anyContains(process->ask("players", "pawn "), "name Alice") && anyContains(process->ask("players", "pawn JCDentonMale name Alice"), "state Dying");
                }));
            }
        }
    }
}
#endif
