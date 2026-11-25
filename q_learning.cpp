#include "q_learning.h"

Choice chooseAction(const State& state, const Game& game, const QLearning& qLearning, Rng& rng) {
    // check exploration FIRST so we skip scanning Q-values when exploring
    if (game.mode == Mode::TRAINING && rng.nextInt(0, 99) < qLearning.epsilon * 100) {
        Choice chosenAction{};
        do {
            chosenAction = static_cast<Choice>(rng.nextInt(LEFT, DOWN));
        } while (!isValidAction(state, game, chosenAction));
        return chosenAction;
    }

    // EXPLOITATION (or EVALUATION)
    int bestAction {-1};
    float bestQ {};
    int tieCount {1};

    for (int i{}; i < 4; ++i) {
        if (!isValidAction(state, game, static_cast<Choice>(i))) {
            continue;
        }

        float currentQ {qLearning.qTable[state.agentX][state.agentY][i]};

        if (bestAction == -1 || currentQ > bestQ) {
            bestQ = currentQ;
            bestAction = i;
            tieCount = 1;
        }
        else if (currentQ == bestQ) {
            ++tieCount;

            if (rng.nextInt(1, tieCount) == 1) {
                bestAction = i;
            }
        }
    }
    return static_cast<Choice>(bestAction);
}

void updateQTable(const State& state, const Game& game, QLearning& qLearning, int oldX, int oldY, Choice chosenAction) {
    float maxnextQ {};
    bool foundValidAction {false};

    for (int i{0}; i < 4; ++i) { // 4 because we have 4 actions {0, 1, 2, 3}
        if (!isValidAction(state, game, static_cast<Choice>(i))) {
            continue;
        }

        float currentQ {qLearning.qTable[state.agentX][state.agentY][i]};

        if (!foundValidAction || currentQ > maxnextQ) {
            maxnextQ = currentQ;
            foundValidAction = true;
        }
    }

    bool reachedGoal { state.agentX == state.goalX && state.agentY == state.goalY };
    int currentReward {-1};

    if (reachedGoal) {
        currentReward = 9; // -1 +10
        maxnextQ = 0.0f;
    }

    float oldQ {qLearning.qTable[oldX][oldY][chosenAction]};

    // BELLMAN EQUATION 
    if (game.mode == Mode::TRAINING) {
        oldQ = oldQ + qLearning.alpha * (currentReward + (qLearning.gamma * maxnextQ) - oldQ);
        qLearning.qTable[oldX][oldY][chosenAction] = oldQ;
    }
}