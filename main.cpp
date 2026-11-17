#include <iostream>
#include <ctime>
#include <string>
#include <chrono>
#include <iomanip>
#include "raylib.h"
#include "choice.h"
#include "game.h"
#include "state.h"
#include "reward.h"
#include "q_learning.h"

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

void updateAndPrintStats(Game& game, const Reward& reward, bool reachedGoal) {
    // update performance stats
    float averageSteps {static_cast<float>(game.totalSteps) / game.episodeCount};
    float averageReward {static_cast<float>(game.totalRewards) / game.episodeCount};

    // rolling average last 50 episodes to measure recent performance
    game.recentSteps.push_back(game.steps);
    if (game.recentSteps.size() > 50) {
        game.recentSteps.pop_front();
    }

    int sumSteps{};
    for (int num : game.recentSteps) {
        sumSteps += num;
    }

    // fewer steps ever taken to reach a goal when reached goal is true
    if (reachedGoal && (game.bestSteps == 0 || game.steps < game.bestSteps)) {
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

    updateAndPrintStats(game, reward, reachedGoal);

    // reset
    game.steps = 0;

    state.agentX = 0;
    state.agentY = 0;

    spawnGoal(state, game);
}

void step(State& state, Game& game, Reward& reward, QLearning& qLearning) { // one action and one q learning update
    int oldX {state.agentX};
    int oldY {state.agentY};

    Choice chosenAction { chooseAction(state, game, qLearning) };

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

    updateQTable(state, game, qLearning, oldX, oldY, chosenAction);

    ++game.steps;
    reward.episodeReward +=  reward.stepPenalty;
}

void finishEpisodes(State& state, Game& game, Reward& reward, QLearning& qLearning, bool reachedGoal) { // handles anything that happens when an episode ends
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

    std::cout << "\tEpsilon: " << qLearning.epsilon << '\n';
    reward.episodeReward = 0;
}


int main(int argc, char* argv[]) {
    SetRandomSeed(static_cast<unsigned int>(14)); // random seed

    bool render {false};
    for (int i{1}; i < argc; ++i) {
        if (std::string(argv[i]) == "--render") {
            render = true;
        }
    }

    State state{};
    Game game{};
    Reward reward{};
    QLearning qLearning{};
    const int screenSize { game.cellSize * game.gridSize };

    // variables for speed and time
    constexpr float speed {0.2f};
    float timer{0.0f};

    spawnGoal(state, game);

    auto startTime {std::chrono::steady_clock::now()};

    while (game.trainingEpisodes < game.maxTrainingEpisodes) {
        step(state, game, reward, qLearning);

        bool reachedGoal { state.agentX == state.goalX && state.agentY == state.goalY };
        bool truncated { game.steps >= Game::maxSteps && !reachedGoal };

        if (reachedGoal || truncated) {
            finishEpisodes(state, game, reward, qLearning, reachedGoal);
        }
    }

    auto endTime {std::chrono::steady_clock::now()};
    auto elapsedTime {endTime - startTime};
    std::chrono::duration<double> seconds {elapsedTime};
    double stepsPerSecond {game.totalSteps / seconds.count()};
    
    std::cout << std::fixed << std::setprecision(2);
    if (stepsPerSecond >= 1'000'000) {
        std::cout << "Steps Per Second: " << stepsPerSecond / 1'000'000 << "M\n";
    }
    else if (stepsPerSecond >= 1'000) {
        std::cout << "Steps Per Second: " << stepsPerSecond / 1'000 << "K\n";
    }
    else {
        std::cout << "Steps Per Second: " << stepsPerSecond << '\n';
    }

    if (render) {
        game.mode = Mode::EVALUATION;
        state.agentX = 5;
        state.agentY = 5;

        InitWindow(screenSize, screenSize, "GridWorld");
        SetTargetFPS(60);

        while (!WindowShouldClose()) {

            timer += GetFrameTime();

            if (timer >= speed) {
                step(state, game, reward, qLearning);
                timer -= speed;
            }

            bool reachedGoal { state.agentX == state.goalX && state.agentY == state.goalY };
            bool truncated { game.steps >= Game::maxSteps && !reachedGoal };

            if (reachedGoal || truncated) { // episode completion block

                finishEpisodes(state, game, reward, qLearning, reachedGoal);

                if (game.mode == Mode::EVALUATION) {
                    state.agentX = 5;
                    state.agentY = 5;
                }
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
    }
    return 0;
}
