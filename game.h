#pragma once
#include <deque>
#include "state.h"
#include "choice.h"

enum class Mode {
    TRAINING,
    EVALUATION
};

struct Game {
    const int cellSize {60};
    static constexpr int gridSize {10};
    static constexpr int maxSteps {200};
    int episodeCount {};
    int steps {};

    int trainingEpisodes {};
    int evaluationEpisodes {};

    long long totalSteps{};
    int totalRewards{};
    int evaluationTotalSteps {};
    int evaluationSuccesses {};

    std::deque<int> recentSteps {};

    int bestSteps {};

    Mode mode{Mode::TRAINING};
};

inline bool isValidAction(int x, int y, Choice action) {
    switch (action) {
        case LEFT:  return x > 0;
        case RIGHT: return x < Game::gridSize - 1;
        case UP:    return y > 0;
        case DOWN:  return y < Game::gridSize - 1;
    }
    return false;
}

inline bool isValidAction(const State& state, const Game& game, Choice action) {
    (void)game;
    return isValidAction(state.agentX, state.agentY, action);
}