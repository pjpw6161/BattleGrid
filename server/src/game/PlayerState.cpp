#include "game/PlayerState.h"

#include <utility>

namespace battlegrid
{
PlayerState::PlayerState(
    std::uint64_t inPlayerId,
    std::uint64_t inRoomId,
    std::string inNickname
)
    : playerId(inPlayerId),
      roomId(inRoomId),
      nickname(std::move(inNickname)),
      latestInput(),
      connected(true)
{
}
}
