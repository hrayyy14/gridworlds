#pragma once
#include <deque>

enum class Mode {
    TRAINING,
    EVALUATION
};

struct Game {
    const int cellSize {60};
    static constexpr int gridSize {10};
    int episodeCount {};
    int steps {};

    int trainingEpisodes {};
    int evaluationEpisodes {};

    int totalSteps{};
    int totalRewards{};
    int evaluationTotalSteps {};

    std::deque<int> recentSteps {};

    int bestSteps {};

    Mode mode{Mode::TRAINING};
};