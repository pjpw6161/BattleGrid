#include "config/ServerConfig.h"
#include "core/Logger.h"
#include "server/GameServer.h"

#include <exception>

int main(int argc, char** argv)
{
    try
    {
        const battlegrid::ServerConfig config =
            battlegrid::ServerConfig::FromArgs(argc, argv);

        battlegrid::GameServer server(config);
        server.Run();
        return 0;
    }
    catch (const std::exception& exception)
    {
        battlegrid::Logger::Error(exception.what());
        return 1;
    }
}
