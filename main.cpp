#include <iostream>
#include <ctime>
#include "raylib.h"
#include "game.h"
#include "state.h"
#include "reward.h"
#include "q_learning.h"

enum Choice {
    LEFT,   // 0
    RIGHT,  // 1
    UP,     // 2
    DOWN    // 3

};

void spawnGoal(State& state, const Game& game) {
    // making sure the goal does not spawn on top of the agent
    /*
    do {
        state.goalX = GetRandomValue(0, game.gridSize - 1) ;
        state.goalY = GetRandomValue(0, game.gridSize - 1) ;
    } while (state.goalX == state.agentX && state.goalY == state.agentY);
    */

    // fixing goal at (9,9)
    state.goalX = game.gridSize - 1;
    state.goalY = game.gridSize - 1;
}

void respawn(State& state, Game& game, const Reward& reward, bool reachedGoal) {
    // update episode count
    ++game.episodeCount;

    if (game.mode == Mode::TRAINING) {
        ++game.trainingEpisodes;
    } else {
        ++game.evaluationEpisodes;
        game.evaluationTotalSteps += game.steps;
        if (reachedGoal) {
            ++game.evaluationSuccesses;
        }

    }

    // update performance stats
    float averageSteps {static_cast<float>(game.totalSteps) / game.episodeCount};
    float averageReward {static_cast<float>(game.totalRewards) / game.episodeCount};

    game.recentSteps.push_back(game.steps);
    if (game.recentSteps.size() > 50) {
        game.recentSteps.pop_front();
    }

    int sumSteps{};
    for (int num : game.recentSteps) {
        sumSteps += num;
    }

    if (reachedGoal && (game.bestSteps == 0 || game.steps < game.bestSteps)) { // fewer steps ever taken to reach a goal
        game.bestSteps = game.steps;
    }

    // prints
    std::cout << "Episode " << game.episodeCount << " completed in " << game.steps << " steps" << '\n';
    std::cout << "\tReward for this episode: " << reward.episodeReward << " completed in " << game.steps << " steps" << '\n';


    std::cout << "\tAverage steps: " << averageSteps << '\n';
    std::cout << "\tAverage steps last " << game.recentSteps.size() << " episodes: " << static_cast<float>(sumSteps) / game.recentSteps.size() << '\n';
    std::cout << "\tBest steps: " << game.bestSteps << '\n';
    if (game.evaluationEpisodes > 0) {
        float averageEvaluationSteps { static_cast<float>(game.evaluationTotalSteps) / game.evaluationEpisodes };
        float evaluationSuccessRate { static_cast<float>(game.evaluationSuccesses) / game.evaluationEpisodes * 100.0f };
        std::cout << "\tAverage evaluation steps: " << averageEvaluationSteps << '\n';
        std::cout << "\tEvaluation success rate: " << evaluationSuccessRate << "%\n";
    }
    std::cout << "\tAverage reward: " << averageReward << '\n';

    // reset
    game.steps = 0;

    state.agentX = 0;
    state.agentY = 0;

    spawnGoal(state, game);
}


int main() {
    SetRandomSeed(static_cast<unsigned int>(time(nullptr))); // random seed

    State state{};
    Game game{};
    Reward reward{};
    QLearning qLearning{};
    const int screenSize { game.cellSize * game.gridSize };

    // variables for speed and time
    constexpr float speed {0.01f};
    float timer{0.0f};

    spawnGoal(state, game);

    InitWindow(screenSize, screenSize, "GridWorld");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {

        timer += GetFrameTime(); // adding framae time to the timer

        if (timer >= speed) {

            int oldX {state.agentX};
            int oldY {state.agentY};

            int bestAction {LEFT};
            float bestQ {qLearning.qTable[state.agentX][state.agentY][LEFT]};

            for (int i{1}; i < 4; ++i) {
                if (qLearning.qTable[state.agentX][state.agentY][i] > bestQ) {
                    bestQ = qLearning.qTable[state.agentX][state.agentY][i];
                    bestAction = i;
                }
            }

            // choose between exploration and exploitation
            int randomNumber { GetRandomValue(0, 99) };
            Choice chosenAction {};

            if ((game.mode == Mode::TRAINING) && (randomNumber < qLearning.epsilon * 100)) {
                // EXPLORATION
                chosenAction = static_cast<Choice>(GetRandomValue(LEFT, DOWN));
            } else {
                // EXPLOITATION
                chosenAction = static_cast<Choice>(bestAction);
            }

            switch (chosenAction) {
                case LEFT:
                    if (state.agentX != 0) {
                        state.agentX += -1;
                    }
                    break;
                case RIGHT:
                    if (state.agentX != (game.gridSize -1)) {
                        state.agentX += 1;
                    }
                    break;
                case UP:
                    if (state.agentY != 0) {
                        state.agentY += -1;
                    }
                    break;
                case DOWN:
                    if (state.agentY != (game.gridSize - 1)) {
                        state.agentY += 1;
                    }
                    break;
            }

            float maxnextQ { qLearning.qTable[state.agentX][state.agentY][LEFT] };

            for (int i{1}; i < 4; ++i) { // 4 because we have 4 actions {0, 1, 2, 3}
                if (qLearning.qTable[state.agentX][state.agentY][i] > maxnextQ) {
                    maxnextQ = qLearning.qTable[state.agentX][state.agentY][i];
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

            ++game.steps;
            reward.episodeReward +=  reward.stepPenalty;

            timer -= speed;
        }

        bool reachedGoal { state.agentX == state.goalX && state.agentY == state.goalY };
        bool truncated { game.steps >= Game::maxSteps && !reachedGoal };

        if (reachedGoal || truncated) {
            if (reachedGoal) {
                reward.episodeReward +=  reward.goalReward;
            }

            game.totalRewards += reward.episodeReward;
            game.totalSteps += game.steps;

            // epsilon decay
            if ((game.mode == Mode::TRAINING) && (qLearning.epsilon > qLearning.epsilonMin)) {
                qLearning.epsilon *= qLearning.epsilonDecay;

                if (qLearning.epsilon < qLearning.epsilonMin) {
                    qLearning.epsilon = qLearning.epsilonMin;
                }
            }

            respawn(state, game, reward, reachedGoal);
  
            // switch between training and evaluation
            if (game.mode == Mode::TRAINING && game.trainingEpisodes % 100 == 0) {
                game.mode = Mode::EVALUATION;
                std::cout << "SWITCHING TO EVALUATION MODE\n";
            }
            else if (game.mode == Mode::EVALUATION && game.evaluationEpisodes % 20 == 0) {
                game.mode = Mode::TRAINING;
                std::cout << "SWITCHING TO TRAINING MODE\n";
            }

            std::cout << "\tEpsilon: " << qLearning.epsilon << '\n';
            reward.episodeReward = 0;
        }

        BeginDrawing();
        ClearBackground(BLACK);

        for (int i {}; i <= game.gridSize; ++i) {
            const int pos { i * game.cellSize };
            DrawLine(pos, 0, pos, screenSize, LIGHTGRAY);
            DrawLine(0, pos, screenSize, pos, LIGHTGRAY);

        }

        DrawRectangle(state.agentX * game.cellSize, state.agentY * game.cellSize, game.cellSize, game.cellSize, ORANGE);  
        DrawRectangle(state.goalX * game.cellSize, state.goalY * game.cellSize, game.cellSize, game.cellSize, GREEN);   
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
