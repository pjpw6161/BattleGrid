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
      alive(true),
      invincible(false),
      x(static_cast<double>(inPlayerId) * 100.0),
      y(0.0),
      z(0.0),
      speed(600.0),
      maxHp(100),
      hp(100),
      score(0),
      deaths(0),
      kills(0),
      playerKills(0),
      targetKills(0),
      botKills(0),
      respawnTimerSeconds(0.0),
      invincibleTimerSeconds(0.0),
      bodyRadius(60.0),
      headRadius(35.0),
      bodyHeight(90.0),
      headHeight(160.0),
      lastProcessedFireSeq(0)
{
}
}
