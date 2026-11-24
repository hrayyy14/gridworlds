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

bool isValidAction(const State& state, const Game& game, Choice action);