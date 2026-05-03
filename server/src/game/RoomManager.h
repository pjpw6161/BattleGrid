#pragma once

#include "game/GameRoom.h"

#include <cstdint>
#include <memory>

namespace battlegrid
{
class RoomManager
{
public:
    RoomManager();

    std::shared_ptr<GameRoom> GetDefaultRoom();
    std::uint64_t GetDefaultRoomId() const;

private:
    std::shared_ptr<GameRoom> defaultRoom;
};
}
