#include "q_learning.h"
#include "model.h"

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

void updateQTable(QLearning& qLearning, int oldX, int oldY, Choice chosenAction, int nextX, int nextY, int stepReward, bool reachedGoal) {
    float maxNextQ {};

    if (!reachedGoal) {
        bool foundValidAction {false};
        for (int i{0}; i < 4; ++i) {
            if (!isValidAction(nextX, nextY, static_cast<Choice>(i))) {
                continue;
            }
            float currentQ {qLearning.qTable[nextX][nextY][i]};
            if (!foundValidAction || currentQ > maxNextQ) {
                maxNextQ = currentQ;
                foundValidAction = true;
            }
        }
    }

    float oldQ {qLearning.qTable[oldX][oldY][chosenAction]};
    // BELLMAN EQUATION
    qLearning.qTable[oldX][oldY][chosenAction] = oldQ + qLearning.alpha * (stepReward + (qLearning.gamma * maxNextQ) - oldQ);
}

void plan(QLearning& qLearning, const EnvModel& model, Rng& rng, int planningSteps) {
    if (model.visitedCount == 0) {
        return;
    }

    for (int p{}; p < planningSteps; ++p) {
        StateAction sa {model.sampleRandom(rng)};
        const Transition& t { model.table[sa.x][sa.y][sa.action] };
        updateQTable(qLearning, sa.x, sa.y, sa.action, t.nextX, t.nextY, t.reward, t.reachedGoal);
    }
}