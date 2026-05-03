#include "game/RoomManager.h"

#include <memory>

namespace battlegrid
{
namespace
{
constexpr std::uint64_t DefaultRoomId = 1;
}

RoomManager::RoomManager()
    : defaultRoom(std::make_shared<GameRoom>(DefaultRoomId))
{
}

std::shared_ptr<GameRoom> RoomManager::GetDefaultRoom()
{
    return defaultRoom;
}

std::uint64_t RoomManager::GetDefaultRoomId() const
{
    return DefaultRoomId;
}
}
