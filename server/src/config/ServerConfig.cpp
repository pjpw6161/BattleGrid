#include "config/ServerConfig.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace battlegrid
{
namespace
{
void PrintUsage()
{
    std::cout
        << "BattleGrid custom server\n"
        << "\n"
        << "Usage:\n"
        << "  battlegrid-server [options]\n"
        << "\n"
        << "Options:\n"
        << "  --host <value>       Bind host address. Default: 0.0.0.0\n"
        << "  --port <value>       Bind port. Default: 7777\n"
        << "  --tick-rate <value>  Game tick rate. Default: 30\n"
        << "  --help               Show this help message.\n";
}

std::string RequireValue(int argc, char** argv, int& index, const std::string& option)
{
    if (index + 1 >= argc)
    {
        throw std::invalid_argument(option + " requires a value.");
    }

    ++index;
    return argv[index];
}

int ParseInt(const std::string& value, const std::string& option)
{
    std::size_t parsedLength = 0;
    const int parsedValue = std::stoi(value, &parsedLength);

    if (parsedLength != value.size())
    {
        throw std::invalid_argument(option + " must be an integer.");
    }

    return parsedValue;
}
}

ServerConfig ServerConfig::FromArgs(int argc, char** argv)
{
    ServerConfig config;

    for (int index = 1; index < argc; ++index)
    {
        const std::string argument = argv[index];

        if (argument == "--help")
        {
            PrintUsage();
            std::exit(0);
        }

        if (argument == "--host")
        {
            config.host = RequireValue(argc, argv, index, argument);
            continue;
        }

        if (argument == "--port")
        {
            const int portValue = ParseInt(RequireValue(argc, argv, index, argument), argument);
            if (portValue < 1 || portValue > 65535)
            {
                throw std::invalid_argument("--port must be between 1 and 65535.");
            }

            config.port = static_cast<std::uint16_t>(portValue);
            continue;
        }

        if (argument == "--tick-rate")
        {
            const int tickRateValue =
                ParseInt(RequireValue(argc, argv, index, argument), argument);
            if (tickRateValue <= 0)
            {
                throw std::invalid_argument("--tick-rate must be greater than 0.");
            }

            config.tickRate = tickRateValue;
            continue;
        }

        throw std::invalid_argument("Unknown argument: " + argument);
    }

    return config;
}
}
