#pragma once

#include <string_view>

namespace battlegrid
{
class Logger
{
public:
    static void Info(std::string_view message);
    static void Warn(std::string_view message);
    static void Error(std::string_view message);
};
}
