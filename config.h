#pragma once

struct Config {
    const int startX{};
    const int startY{};
    const int goalX{9};
    const int goalY{9};
    const int seed{14};
    const int maxTrainingEpisodes{2000};
    bool render{false};
};