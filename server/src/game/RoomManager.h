#pragma once

#include "game/GameRoom.h"

#include <nlohmann/json.hpp>

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
    void TickAll(double deltaSeconds, std::uint64_t tickNumber);
    nlohmann::json BuildDefaultRoomSnapshotJson(std::uint64_t tickNumber) const;

private:
    std::shared_ptr<GameRoom> defaultRoom;
};
}
