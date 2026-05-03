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
      connected(true),
      x(static_cast<double>(inPlayerId) * 100.0),
      y(0.0),
      speed(500.0),
      hp(100),
      score(0)
{
}
}
