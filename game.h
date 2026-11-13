#pragma once

struct Game {
    const int cellSize {60};
    static constexpr int gridSize {10};
    int episodeCount {};
    int steps {};

    int totalSteps{};
    int totalRewards{};
};