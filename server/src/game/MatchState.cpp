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
    winnerScore = 0;
    winnerKills = 0;
    isDraw = false;
    noWinner = false;
    matchEndedEventEmitted = false;
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
    const std::string& inWinnerNickname,
    int inWinnerScore,
    int inWinnerKills,
    bool bNoWinner
)
{
    gameOver = true;
    state = "game_over";
    winnerPlayerId = inWinnerPlayerId;
    winnerNickname = inWinnerNickname;
    winnerScore = std::max(0, inWinnerScore);
    winnerKills = std::max(0, inWinnerKills);
    noWinner = bNoWinner || winnerPlayerId == 0 || winnerScore <= 0;
    isDraw = noWinner;
    if (noWinner)
    {
        winnerPlayerId = 0;
        winnerNickname.clear();
        winnerScore = 0;
        winnerKills = 0;
    }
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
    json["winner_score"] = winnerScore;
    json["winner_kills"] = winnerKills;
    json["is_draw"] = isDraw;
    json["no_winner"] = noWinner;
    json["match_id"] = matchId;
    return json;
}
}
