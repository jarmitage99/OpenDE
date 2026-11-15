#pragma once

#include "core/math.h"
#include "core/object.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace dx {
    class World;

    constexpr uint8_t ROLE_None = 0;
    constexpr uint8_t ROLE_DumbProxy = 1;
    constexpr uint8_t ROLE_SimulatedProxy = 2;
    constexpr uint8_t ROLE_AutonomousProxy = 3;
    constexpr uint8_t ROLE_Authority = 4;

    constexpr uint8_t NM_Standalone = 0;
    constexpr uint8_t NM_DedicatedServer = 1;
    constexpr uint8_t NM_ListenServer = 2;
    constexpr uint8_t NM_Client = 3;

    namespace net {
        constexpr uint32_t ProtocolVersion = 1;
        constexpr int DefaultPort = 7790;

        struct Event {
            enum class Kind : uint8_t {
                Connected,
                Disconnected,
                Message,
            };

            Kind kind = Kind::Message;
            uint32_t connection = 0;
            std::vector<uint8_t> data;
        };

        class Transport {
        public:
            virtual ~Transport() = default;
            virtual void send(uint32_t connection, const std::vector<uint8_t>& data, bool reliable) = 0;
            virtual void close(uint32_t connection) = 0;
            virtual std::vector<Event> poll() = 0;
            virtual std::string address(uint32_t connection) = 0;
            virtual int ping(uint32_t connection) = 0;
            virtual int loss(uint32_t connection) = 0;
        };

        class Packet {
        public:
            void u8(uint8_t value);
            void varint(uint32_t value);
            void i32(int32_t value);
            void f32(float value);
            void str(std::string_view value);
            void vector(const Vector& value);
            void rotator(const Rotator& value);

            std::vector<uint8_t> data;
        };

        class PacketReader {
        public:
            PacketReader(const uint8_t* begin, size_t size);

            uint8_t u8();
            uint32_t varint();
            int32_t i32();
            float f32();
            std::string str();
            Vector vector();
            Rotator rotator();
            bool atEnd() const;

            bool failed = false;

        private:
            const uint8_t* cursor;
            const uint8_t* end;
        };

        struct RepEntry {
            UProperty* property = nullptr;
            size_t offset = 0;
            size_t condition = 0;
            uint32_t wireIndex = 0;
        };

        struct RepCondition {
            UClass* owner = nullptr;
            uint16_t offset = 0;
            bool animation = false;
        };

        struct ClassLayout {
            std::vector<RepEntry> entries;
            std::vector<RepCondition> conditions;
            std::vector<Name> functions;
        };

        struct Channel {
            UObject* actor = nullptr;
            UClass* cls = nullptr;
            uint32_t id = 0;
            uint8_t* shadow = nullptr;
            bool initial = true;
            double relevantTime = 0;
            double lastUpdate = 0;
        };

        struct Connection {
            uint32_t id = 0;
            UObject* player = nullptr;
            UObject* pawn = nullptr;
            bool greeted = false;
            bool welcomed = false;
            bool joined = false;
            std::unordered_map<UObject*, Channel> channels;
            std::unordered_map<UObject*, uint32_t> sentObjects;
            std::vector<UObject*> receivedObjects;
            std::unordered_set<UObject*> temporaries;
        };

        class Driver {
        public:
            Driver(Transport& link, bool isServer);
            ~Driver();
            Driver(const Driver&) = delete;
            Driver& operator=(const Driver&) = delete;

            void setWorld(World* target);
            void tick(float deltaSeconds);
            void join(const std::string& options);
            bool remoteCall(UObject* context, UFunction* function, uint8_t* locals);
            void actorDestroyed(UObject* actor);
            bool canDestroy(UObject* actor) const;
            std::string playerAddress(UObject* pawn);
            int playerPing(UObject* pawn);
            int playerLoss(UObject* pawn);
            std::vector<UObject*> remotePlayers() const;
            size_t connectionCount() const;

            const bool server;
            std::string welcomeUrl;
            std::string failure;
            bool closed = false;

        private:
            const ClassLayout& layout(UClass* cls);
            Connection* connectionOf(UObject* actor);
            bool owns(const Connection& connection, UObject* actor) const;
            void send(Connection& connection, const Packet& packet, bool reliable);
            void handle(Connection& connection, const std::vector<uint8_t>& data);
            void handleHello(Connection& connection, PacketReader& in);
            void handleWelcome(PacketReader& in);
            void handleJoin(Connection& connection, PacketReader& in);
            void handleDefine(Connection& connection, PacketReader& in);
            void handleActor(Connection& connection, PacketReader& in);
            void handleClose(PacketReader& in);
            void handleCall(Connection& connection, PacketReader& in);
            void welcome(Connection& connection);
            void drop(Connection& connection, bool notify);
            void replicateConnection(Connection& connection);
            bool relevant(Connection& connection, UObject* actor, UObject* viewer, const Vector& viewLocation);
            Channel& openChannel(Connection& connection, UObject* actor);
            void closeChannel(Connection& connection, UObject* actor, bool notify);
            void replicateActor(Connection& connection, Channel& channel);
            bool evaluate(UObject* actor, const RepCondition& condition);
            bool mappable(const Connection& connection, UObject* object) const;
            bool writeObject(Packet& out, Connection& connection, UObject* object);
            UObject* readObject(PacketReader& in, Connection& connection);
            bool writeValue(Packet& out, Connection& connection, UProperty* property, const uint8_t* data);
            void readValue(PacketReader& in, Connection& connection, UProperty* property, uint8_t* data);
            void possess(UObject* pawn);

            Transport& transport;
            World* world = nullptr;
            std::unordered_map<uint32_t, Connection> connections;
            std::unordered_map<UClass*, ClassLayout> layouts;
            std::unordered_map<UObject*, uint32_t> netIds;
            std::unordered_map<uint32_t, UObject*> actorsById;
            uint32_t nextNetId = 1;
            double time = 0;
            bool receiving = false;
        };
    }
}
