#pragma once
#include "config.h"

struct Reward {
    const int goalReward{};
    const int stepPenalty{};
    int episodeReward {};

    Reward(const Config& config)
        : goalReward{config.goalReward},
          stepPenalty{config.stepPenalty}
    {}
};