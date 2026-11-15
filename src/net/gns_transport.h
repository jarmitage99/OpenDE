#pragma once

#include "engine/net.h"

#include <memory>
#include <string>

namespace dx::net {
    std::unique_ptr<Transport> listenTransport(int port);
    std::unique_ptr<Transport> connectTransport(const std::string& address);
}
