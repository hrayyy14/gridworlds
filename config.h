#pragma once

struct Config {
    const int startX{};
    const int startY{};
    const int goalX{9};
    const int goalY{9};
    const int goalReward{10};
    const int stepPenalty{-1};

    const int seed{14};
    bool randomSeed{false}; // if true --> generates a random seed each run ; false --> always use 14 (or seed var value)
    const int maxTrainingEpisodes{2000};
    bool render{false};

    // hyperparams
    const float alpha {0.1f};  // learning rate
    const float gamma {0.95f};  // discount factor
    const float epsilon {1.0f}; // start with 100% exploration
    const float epsilonDecay {0.9995f};
    const float epsilonMin {0.05f}; // minimum 5% exploration
};