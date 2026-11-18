#include "net/gns_transport.h"

#include <steam/isteamnetworkingutils.h>
#include <steam/steamnetworkingsockets.h>

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace dx::net {
    namespace {
        class GnsTransport;

        GnsTransport* instance = nullptr;
        int libraryUsers = 0;

        bool resolve(const std::string& text, SteamNetworkingIPAddr& address)
        {
            std::string host = text;
            int port = DefaultPort;
            size_t colon = host.rfind(':');
            if (colon != std::string::npos && host.find(':') == colon) {
                port = std::atoi(host.substr(colon + 1).c_str());
                host = host.substr(0, colon);
            }
            address.Clear();
            if (address.ParseString(host.c_str())) {
                address.m_port = static_cast<uint16>(port);
                return true;
            }
            addrinfo hints {};
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_DGRAM;
            addrinfo* found = nullptr;
            if (getaddrinfo(host.c_str(), nullptr, &hints, &found) != 0 || !found) {
                return false;
            }
            const sockaddr_in* ipv4 = reinterpret_cast<const sockaddr_in*>(found->ai_addr);
            address.SetIPv4(ntohl(ipv4->sin_addr.s_addr), static_cast<uint16>(port));
            freeaddrinfo(found);
            return true;
        }

        class GnsTransport : public Transport {
        public:
            GnsTransport()
            {
                if (libraryUsers++ == 0) {
                    SteamDatagramErrMsg error;
                    if (!GameNetworkingSockets_Init(nullptr, error)) {
                        std::fprintf(stderr, "GameNetworkingSockets failed to start: %s\n", error);
                    }
                }
                sockets = SteamNetworkingSockets();
                instance = this;
                if (const char* loss = std::getenv("DX_NET_LOSS")) {
                    float percent = static_cast<float>(std::atof(loss));
                    SteamNetworkingUtils()->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketLoss_Send, percent);
                    SteamNetworkingUtils()->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketLoss_Recv, percent);
                }
                if (const char* lag = std::getenv("DX_NET_LAG")) {
                    SteamNetworkingUtils()->SetGlobalConfigValueInt32(k_ESteamNetworkingConfig_FakePacketLag_Send, std::atoi(lag));
                    SteamNetworkingUtils()->SetGlobalConfigValueInt32(k_ESteamNetworkingConfig_FakePacketLag_Recv, std::atoi(lag));
                }
            }

            ~GnsTransport() override
            {
                for (HSteamNetConnection connection : open) {
                    sockets->CloseConnection(connection, 0, nullptr, true);
                }
                if (listener != k_HSteamListenSocket_Invalid) {
                    sockets->CloseListenSocket(listener);
                }
                if (instance == this) {
                    instance = nullptr;
                }
                if (--libraryUsers == 0) {
                    GameNetworkingSockets_Kill();
                }
            }

            bool listen(int port)
            {
                SteamNetworkingIPAddr address;
                address.Clear();
                address.m_port = static_cast<uint16>(port);
                SteamNetworkingConfigValue_t options[2];
                configure(options);
                listener = sockets->CreateListenSocketIP(address, 2, options);
                return listener != k_HSteamListenSocket_Invalid;
            }

            bool connect(const std::string& text)
            {
                SteamNetworkingIPAddr address;
                if (!resolve(text, address)) {
                    return false;
                }
                SteamNetworkingConfigValue_t options[2];
                configure(options);
                HSteamNetConnection connection = sockets->ConnectByIPAddress(address, 2, options);
                if (connection == k_HSteamNetConnection_Invalid) {
                    return false;
                }
                open.push_back(connection);
                return true;
            }

            void send(uint32_t connection, const std::vector<uint8_t>& data, bool reliable) override
            {
                sockets->SendMessageToConnection(connection, data.data(), static_cast<uint32>(data.size()), reliable ? k_nSteamNetworkingSend_Reliable : k_nSteamNetworkingSend_Unreliable, nullptr);
            }

            void close(uint32_t connection) override
            {
                sockets->CloseConnection(connection, 0, nullptr, true);
                std::erase(open, connection);
            }

            std::vector<Event> poll() override
            {
                instance = this;
                sockets->RunCallbacks();
                for (HSteamNetConnection connection : std::vector<HSteamNetConnection>(open)) {
                    while (true) {
                        ISteamNetworkingMessage* messages[32];
                        int count = sockets->ReceiveMessagesOnConnection(connection, messages, 32);
                        if (count <= 0) {
                            break;
                        }
                        for (int i = 0; i < count; i++) {
                            Event event;
                            event.kind = Event::Kind::Message;
                            event.connection = messages[i]->m_conn;
                            const uint8_t* bytes = static_cast<const uint8_t*>(messages[i]->m_pData);
                            event.data.assign(bytes, bytes + messages[i]->m_cbSize);
                            events.push_back(std::move(event));
                            messages[i]->Release();
                        }
                    }
                }
                std::vector<Event> result;
                result.swap(events);
                return result;
            }

            std::string address(uint32_t connection) override
            {
                SteamNetConnectionInfo_t info;
                if (!sockets->GetConnectionInfo(connection, &info)) {
                    return {};
                }
                char text[SteamNetworkingIPAddr::k_cchMaxString];
                info.m_addrRemote.ToString(text, sizeof(text), true);
                return text;
            }

            int ping(uint32_t connection) override
            {
                SteamNetConnectionRealTimeStatus_t status;
                if (sockets->GetConnectionRealTimeStatus(connection, &status, 0, nullptr) != k_EResultOK) {
                    return 0;
                }
                return std::max(status.m_nPing, 0);
            }

            int loss(uint32_t connection) override
            {
                SteamNetConnectionRealTimeStatus_t status;
                if (sockets->GetConnectionRealTimeStatus(connection, &status, 0, nullptr) != k_EResultOK || status.m_flConnectionQualityLocal < 0) {
                    return 0;
                }
                return static_cast<int>((1.0f - status.m_flConnectionQualityLocal) * 100.0f);
            }

            void statusChanged(SteamNetConnectionStatusChangedCallback_t* info)
            {
                HSteamNetConnection connection = info->m_hConn;
                switch (info->m_info.m_eState) {
                case k_ESteamNetworkingConnectionState_Connecting:
                    if (info->m_info.m_hListenSocket != k_HSteamListenSocket_Invalid && sockets->AcceptConnection(connection) != k_EResultOK) {
                        sockets->CloseConnection(connection, 0, nullptr, false);
                    }
                    break;
                case k_ESteamNetworkingConnectionState_Connected: {
                    if (info->m_info.m_hListenSocket != k_HSteamListenSocket_Invalid) {
                        open.push_back(connection);
                    }
                    Event event;
                    event.kind = Event::Kind::Connected;
                    event.connection = connection;
                    events.push_back(std::move(event));
                    break;
                }
                case k_ESteamNetworkingConnectionState_ClosedByPeer:
                case k_ESteamNetworkingConnectionState_ProblemDetectedLocally: {
                    sockets->CloseConnection(connection, 0, nullptr, false);
                    std::erase(open, connection);
                    Event event;
                    event.kind = Event::Kind::Disconnected;
                    event.connection = connection;
                    events.push_back(std::move(event));
                    break;
                }
                case k_ESteamNetworkingConnectionState_None:
                case k_ESteamNetworkingConnectionState_FindingRoute:
                case k_ESteamNetworkingConnectionState_FinWait:
                case k_ESteamNetworkingConnectionState_Linger:
                case k_ESteamNetworkingConnectionState_Dead:
                case k_ESteamNetworkingConnectionState__Force32Bit:
                    break;
                }
            }

        private:
            static void onStatusChanged(SteamNetConnectionStatusChangedCallback_t* info)
            {
                if (instance) {
                    instance->statusChanged(info);
                }
            }

            void configure(SteamNetworkingConfigValue_t* options)
            {
                options[0].SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, reinterpret_cast<void*>(onStatusChanged));
                options[1].SetInt32(k_ESteamNetworkingConfig_SendBufferSize, 8 * 1024 * 1024);
            }

            ISteamNetworkingSockets* sockets = nullptr;
            HSteamListenSocket listener = k_HSteamListenSocket_Invalid;
            std::vector<HSteamNetConnection> open;
            std::vector<Event> events;
        };
    }

    std::unique_ptr<Transport> listenTransport(int port)
    {
        auto transport = std::make_unique<GnsTransport>();
        if (!transport->listen(port)) {
            return nullptr;
        }
        return transport;
    }

    std::unique_ptr<Transport> connectTransport(const std::string& address)
    {
        auto transport = std::make_unique<GnsTransport>();
        if (!transport->connect(address)) {
            return nullptr;
        }
        return transport;
    }
}
