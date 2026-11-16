#include "core/vm.h"
#include "engine/ipdrv.h"

#ifndef __EMSCRIPTEN__

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <algorithm>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace dx {
    namespace {
#ifdef _WIN32
        using SocketHandle = SOCKET;
        using AddressLength = int;
        using DataLength = int;
#else
        using SocketHandle = int;
        using AddressLength = socklen_t;
        using DataLength = size_t;
#endif

        struct IpAddr {
            int32_t addr = 0;
            int32_t port = 0;
        };

        struct Link {
            UObject* actor = nullptr;
            SocketHandle socket = 0;
        };

        struct Resolution {
            UObject* actor = nullptr;
            IpAddr address;
            bool found = false;
        };

        std::vector<Link> links;
        std::vector<Resolution> resolutions;

        void closeSocket(SocketHandle handle)
        {
#ifdef _WIN32
            closesocket(handle);
#else
            close(handle);
#endif
        }

        sockaddr_in socketAddress(uint32_t address, int32_t port)
        {
            sockaddr_in target {};
            target.sin_family = AF_INET;
            target.sin_addr.s_addr = htonl(address);
            target.sin_port = htons(static_cast<uint16_t>(port));
            return target;
        }

        bool parseAddress(const std::string& text, int32_t& address)
        {
            in_addr parsed {};
            if (inet_pton(AF_INET, text.c_str(), &parsed) != 1) {
                return false;
            }
            address = static_cast<int32_t>(ntohl(parsed.s_addr));
            return true;
        }

        bool isLocalAddress(uint32_t host)
        {
            static std::unordered_map<uint32_t, bool> known;
            auto found = known.find(host);
            if (found != known.end()) {
                return found->second;
            }
            SocketHandle probe = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            sockaddr_in local = socketAddress(host, 0);
            bool own = bind(probe, reinterpret_cast<sockaddr*>(&local), sizeof(local)) == 0;
            closeSocket(probe);
            known[host] = own;
            return own;
        }

        Link* linkOf(UObject* actor)
        {
            for (Link& link : links) {
                if (link.actor == actor) {
                    return &link;
                }
            }
            return nullptr;
        }

        void nativeBindPort(UObject* self, Frame& stack, void* result)
        {
            int32_t port = vm::getOpt<int32_t>(stack, 0);
            bool useNext = vm::getBoolOpt(stack, false);
            vm::finish(stack);
            vm::ret<int32_t>(result, 0);
            if (linkOf(self)) {
                return;
            }
#ifdef _WIN32
            WSADATA data;
            WSAStartup(MAKEWORD(2, 2), &data);
#endif
            SocketHandle handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            int enabled = 1;
            setsockopt(handle, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&enabled), sizeof(enabled));
#ifdef _WIN32
            u_long nonBlocking = 1;
            ioctlsocket(handle, static_cast<long>(FIONBIO), &nonBlocking);
#else
            fcntl(handle, F_SETFL, fcntl(handle, F_GETFL, 0) | O_NONBLOCK);
#endif
            sockaddr_in local = socketAddress(INADDR_ANY, port);
            bool bound = bind(handle, reinterpret_cast<sockaddr*>(&local), sizeof(local)) == 0;
            for (int attempt = 1; !bound && useNext && attempt < 100; attempt++) {
                local = socketAddress(INADDR_ANY, port + attempt);
                bound = bind(handle, reinterpret_cast<sockaddr*>(&local), sizeof(local)) == 0;
            }
            AddressLength length = sizeof(local);
            if (!bound || getsockname(handle, reinterpret_cast<sockaddr*>(&local), &length) != 0) {
                closeSocket(handle);
                return;
            }
            links.push_back({ self, handle });
            int32_t boundPort = ntohs(local.sin_port);
            *reinterpret_cast<int32_t*>(self->propertyAddress(Name("Port"))) = boundPort;
            vm::ret<int32_t>(result, boundPort);
        }

        void nativeSendText(UObject* self, Frame& stack, void* result)
        {
            IpAddr address = vm::get<IpAddr>(stack);
            std::string text = vm::get<std::string>(stack);
            vm::finish(stack);
            Link* link = linkOf(self);
            if (!link) {
                vm::retBool(result, false);
                return;
            }
            uint32_t host = static_cast<uint32_t>(address.addr);
            sockaddr_in target = socketAddress(host, address.port);
            auto sent = sendto(link->socket, text.data(), static_cast<DataLength>(text.size()), 0, reinterpret_cast<sockaddr*>(&target), sizeof(target));
            if (host == INADDR_BROADCAST) {
                target = socketAddress(INADDR_LOOPBACK, address.port);
                sent = std::max(sent, sendto(link->socket, text.data(), static_cast<DataLength>(text.size()), 0, reinterpret_cast<sockaddr*>(&target), sizeof(target)));
            }
            vm::retBool(result, sent >= 0);
        }

        void nativeResolve(UObject* self, Frame& stack, void*)
        {
            std::string domain = vm::get<std::string>(stack);
            vm::finish(stack);
            Resolution resolution;
            resolution.actor = self;
            resolution.found = parseAddress(domain, resolution.address.addr);
            if (!resolution.found) {
                addrinfo hints {};
                hints.ai_family = AF_INET;
                addrinfo* found = nullptr;
                if (getaddrinfo(domain.c_str(), nullptr, &hints, &found) == 0 && found) {
                    resolution.address.addr = static_cast<int32_t>(ntohl(reinterpret_cast<const sockaddr_in*>(found->ai_addr)->sin_addr.s_addr));
                    resolution.found = true;
                    freeaddrinfo(found);
                }
            }
            resolutions.push_back(resolution);
        }

        void nativeIpAddrToString(UObject*, Frame& stack, void* result)
        {
            IpAddr address = vm::get<IpAddr>(stack);
            vm::finish(stack);
            uint32_t host = static_cast<uint32_t>(address.addr);
            vm::ret<std::string>(result, std::to_string(host >> 24) + "." + std::to_string((host >> 16) & 255) + "." + std::to_string((host >> 8) & 255) + "." + std::to_string(host & 255) + ":" + std::to_string(address.port));
        }

        void nativeStringToIpAddr(UObject*, Frame& stack, void* result)
        {
            std::string text = vm::get<std::string>(stack);
            IpAddr* address = vm::getRef<IpAddr>(stack);
            vm::finish(stack);
            vm::retBool(result, parseAddress(text, address->addr));
        }

        void receive(const Link& link)
        {
            static const Name names[] = { Name("ReceivedText"), Name("ReceivedLine"), Name("ReceivedBinary") };
            for (int packet = 0; packet < 32 && !link.actor->destroyed; packet++) {
                char buffer[4096];
                sockaddr_in source {};
                AddressLength length = sizeof(source);
                auto count = recvfrom(link.socket, buffer, sizeof(buffer), 0, reinterpret_cast<sockaddr*>(&source), &length);
                if (count <= 0) {
                    return;
                }
                uint8_t mode = std::min<uint8_t>(*link.actor->propertyAddress(Name("LinkMode")), 2);
                vm::Call call(link.actor, names[mode]);
                if (!call.valid()) {
                    continue;
                }
                uint32_t host = ntohl(source.sin_addr.s_addr);
                if (isLocalAddress(host)) {
                    host = INADDR_LOOPBACK;
                }
                IpAddr address { static_cast<int32_t>(host), ntohs(source.sin_port) };
                std::memcpy(call.parameter(0), &address, sizeof(address));
                if (mode == 2) {
                    int32_t size = std::min<int32_t>(static_cast<int32_t>(count), 255);
                    *reinterpret_cast<int32_t*>(call.parameter(1)) = size;
                    std::memcpy(call.parameter(2), buffer, static_cast<size_t>(size));
                } else {
                    *reinterpret_cast<std::string*>(call.parameter(1)) = std::string(buffer, static_cast<size_t>(count));
                }
                call.run();
            }
        }
    }

    void pollLinks()
    {
        std::erase_if(links, [](const Link& link) {
            if (!link.actor->destroyed) {
                return false;
            }
            closeSocket(link.socket);
            return true;
        });
        std::vector<Resolution> resolved;
        resolved.swap(resolutions);
        for (const Resolution& resolution : resolved) {
            if (resolution.actor->destroyed) {
                continue;
            }
            vm::Call call(resolution.actor, Name(resolution.found ? "Resolved" : "ResolveFailed"));
            if (call.valid() && resolution.found) {
                std::memcpy(call.parameter(0), &resolution.address, sizeof(resolution.address));
            }
            call.run();
        }
        for (const Link& link : std::vector<Link>(links)) {
            receive(link);
        }
    }

    void closeLinks()
    {
        for (const Link& link : links) {
            closeSocket(link.socket);
        }
        links.clear();
        resolutions.clear();
    }

    void registerIpDrvNatives()
    {
        vm::registerNative("UdpLink", "BindPort", nativeBindPort);
        vm::registerNative("UdpLink", "SendText", nativeSendText);
        vm::registerNative("InternetLink", "Resolve", nativeResolve);
        vm::registerNative("InternetLink", "IpAddrToString", nativeIpAddrToString);
        vm::registerNative("InternetLink", "StringToIpAddr", nativeStringToIpAddr);
    }
}

#else

namespace dx {
    void pollLinks()
    {
    }

    void closeLinks()
    {
    }

    void registerIpDrvNatives()
    {
    }
}

#endif
