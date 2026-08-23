#ifndef CROSSCTL_SERVER_H
#define CROSSCTL_SERVER_H

#include "crossctl/statemachine.h"

#include <cstdint>
#include <string>

namespace crossctl {

struct ServerConfig {
    uint16_t    port           = 842;
    uint32_t    move_timeout   = 10;
    std::string log_path;      // empty = stderr only
};

// Runs until the client disconnects or SIGINT. Returns process exit code.
int run_server(const ServerConfig &cfg);

} // namespace crossctl

#endif
