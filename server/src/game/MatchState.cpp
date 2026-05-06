#include "game/MatchState.h"

#include <algorithm>

namespace battlegrid
{
void MatchState::Reset()
{
    state = "in_progress";
    matchDurationSeconds = 300.0;
    timeRemainingSeconds = matchDurationSeconds;
    targetScore = 20;
    gameOver = false;
    winnerPlayerId = 0;
    winnerNickname.clear();
}

void MatchState::Tick(double deltaSeconds, bool bAutoEndByTimer)
{
    if (gameOver)
    {
        return;
    }

    timeRemainingSeconds = std::max(0.0, timeRemainingSeconds - std::max(0.0, deltaSeconds));
    if (bAutoEndByTimer && timeRemainingSeconds <= 0.0)
    {
        gameOver = true;
        state = "game_over";
    }
}

bool MatchState::IsGameOver() const
{
    return gameOver;
}

void MatchState::EndMatch(
    std::uint64_t inWinnerPlayerId,
    const std::string& inWinnerNickname
)
{
    gameOver = true;
    state = "game_over";
    winnerPlayerId = inWinnerPlayerId;
    winnerNickname = inWinnerNickname;
}

nlohmann::json MatchState::ToJson() const
{
    nlohmann::json json;
    json["state"] = state;
    json["time_left"] = timeRemainingSeconds;
    json["duration"] = matchDurationSeconds;
    json["target_score"] = targetScore;
    json["game_over"] = gameOver;
    json["winner_player_id"] = winnerPlayerId;
    json["winner_nickname"] = winnerNickname;
    json["match_id"] = matchId;
    return json;
}
}
