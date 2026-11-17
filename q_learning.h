#pragma once
#include "game.h"
#include "state.h"
#include "choice.h"

struct QLearning {
    float qTable[Game::gridSize][Game::gridSize][4] {}; // for example 10 x 10 grid and 4 states (left, right, up, down)
    const float alpha {0.1f};  // learning rate
    const float gamma {0.95f};  // discount factor
    float epsilon {1.0f}; // start with 100% exploration
    const float epsilonDecay {0.9995f};
    const float epsilonMin {0.05f}; // minimum 5% exploration
};

Choice chooseAction(const State& state, const Game& game, const QLearning& qLearning);
void updateQTable(const State& state, const Game& game, QLearning& qLearning, int oldX, int oldY, Choice chosenAction);