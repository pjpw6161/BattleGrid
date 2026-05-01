#pragma once

#include <cstdint>
#include <string>

namespace battlegrid
{
struct ServerConfig
{
    std::string host = "0.0.0.0";
    std::uint16_t port = 7777;
    int tickRate = 30;

    static ServerConfig FromArgs(int argc, char** argv);
};
}
