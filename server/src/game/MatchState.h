#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>

namespace battlegrid
{
struct MatchState
{
    std::string state = "in_progress";
    double matchDurationSeconds = 300.0;
    double timeRemainingSeconds = 300.0;
    int targetScore = 20;
    bool gameOver = false;
    std::uint64_t winnerPlayerId = 0;
    std::string winnerNickname;
    int winnerScore = 0;
    int winnerKills = 0;
    bool isDraw = false;
    bool noWinner = false;
    bool matchEndedEventEmitted = false;
    std::uint64_t matchId = 1;

    void Reset();
    void Tick(double deltaSeconds, bool bAutoEndByTimer = true);
    bool IsGameOver() const;
    void EndMatch(
        std::uint64_t inWinnerPlayerId,
        const std::string& inWinnerNickname,
        int inWinnerScore,
        int inWinnerKills,
        bool bNoWinner
    );
    nlohmann::json ToJson() const;
};
}
