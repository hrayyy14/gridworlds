#pragma once
#include "game.h"
#include "state.h"
#include "choice.h"
#include "config.h"

struct QLearning {
    float qTable[Game::gridSize][Game::gridSize][4] {}; // for example 10 x 10 grid and 4 states (left, right, up, down)
    const float alpha{};
    const float gamma{};
    float epsilon{};
    const float epsilonDecay{};
    const float epsilonMin{};

    QLearning(const Config& config)
        : alpha{config.alpha}, 
        gamma{config.gamma},
        epsilon{config.epsilon},
        epsilonDecay{config.epsilonDecay},
        epsilonMin{config.epsilonMin}
    {}
};

Choice chooseAction(const State& state, const Game& game, const QLearning& qLearning);
void updateQTable(const State& state, const Game& game, QLearning& qLearning, int oldX, int oldY, Choice chosenAction);