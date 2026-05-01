#include "core/Logger.h"

#include <iostream>

namespace battlegrid
{
namespace
{
void Write(std::ostream& stream, std::string_view message)
{
    stream << "[BattleGridServer] " << message << '\n';
}
}

void Logger::Info(std::string_view message)
{
    Write(std::cout, message);
}

void Logger::Warn(std::string_view message)
{
    Write(std::cout, message);
}

void Logger::Error(std::string_view message)
{
    Write(std::cerr, message);
}
}
