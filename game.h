#pragma once
#include <deque>

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

    int totalSteps{};
    int totalRewards{};
    int evaluationTotalSteps {};
    int evaluationSuccesses {};

    std::deque<int> recentSteps {};

    int bestSteps {};

    Mode mode{Mode::TRAINING};
};