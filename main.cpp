#include <iostream>
#include <ctime>
#include <string>
#include <chrono>
#include <iomanip>
#include <fstream>

#include "raylib.h"
#include "choice.h"
#include "game.h"
#include "state.h"
#include "reward.h"
#include "q_learning.h"
#include "config.h"

void spawnGoal(State& state, const Game& game, const Config& config) {
    // making sure the goal does not spawn on top of the agent
    /*
    do {
        state.goalX = GetRandomValue(0, game.gridSize - 1) ;
        state.goalY = GetRandomValue(0, game.gridSize - 1) ;
    } while (state.goalX == state.agentX && state.goalY == state.agentY);
    */

    // fixing goal at (9,9)
    state.goalX = config.goalX;
    state.goalY = config.goalY;
}

void updateStats(Game& game, bool reachedGoal) {
    // rolling average: last 50 training episodes
    if (game.mode == Mode::TRAINING) {
        game.recentSteps.push_back(game.steps);

        if (game.recentSteps.size() > 50) {
            game.recentSteps.pop_front();
        }

        // Best successful training episode
        if (reachedGoal && (game.bestSteps == 0 || game.steps < game.bestSteps)) {
            game.bestSteps = game.steps;
        }
    }
}

void printProgress(const Game& game, const Config& config) {
    if (game.mode == Mode::TRAINING && game.trainingEpisodes % 500 == 0) {
        std::cout << "Progress: " << game.trainingEpisodes << " / " << config.maxTrainingEpisodes << '\n';
    }
}

void respawn(State& state, Game& game, const Reward& reward, bool reachedGoal, const Config& config) {
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

    updateStats(game, reachedGoal);

    if (game.mode == Mode::TRAINING) {
        printProgress(game, config);
    }

    // reset
    game.steps = 0;

    state.agentX = config.startX;
    state.agentY = config.startY;

    spawnGoal(state, game, config);
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

void finishEpisodes(State& state, Game& game, Reward& reward, QLearning& qLearning, bool reachedGoal, const Config& config, std::ofstream& csvFile) { // handles anything that happens when an episode ends
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

    if (game.mode == Mode::TRAINING) {
        csvFile << game.trainingEpisodes + 1 << ',' << game.steps << ',' << reward.episodeReward << ',' << reachedGoal << '\n';
    }

    respawn(state, game, reward, reachedGoal, config);

    reward.episodeReward = 0;
}


int main(int argc, char* argv[]) {
    Config config{};

    unsigned int seed = config.randomSeed ? static_cast<unsigned int>(time(nullptr)) : static_cast<unsigned int>(config.seed);
    SetRandomSeed(seed);

    std::ofstream csvFile("training.csv");
    //check
    if (!csvFile.is_open()) {
        std::cerr << "Error: could not open file for writing.\n";
        return 1;
    }
    //write
    csvFile << "episode,steps,reward,success\n";

    for (int i{1}; i < argc; ++i) {
        if (std::string(argv[i]) == "--render") {
            config.render = true;
        }
    }

    State state{};
    Game game{};
    Reward reward{config};
    QLearning qLearning{config};

    std::cout << "\nGridWorld RL\n"
          << "------------------------------\n"
          << "Mode:     Training\n"
          << "Grid:     " << game.gridSize << "x" << game.gridSize << '\n'
          << "Episodes: " << config.maxTrainingEpisodes << '\n'
          << "Seed:     " << seed << "\n\n"
          << "Training...\n";

    const int screenSize { game.cellSize * game.gridSize };

    // variables for speed and time
    constexpr float speed {0.2f};
    float timer{0.0f};

    spawnGoal(state, game, config);

    auto startTime {std::chrono::steady_clock::now()};

    while (game.trainingEpisodes < config.maxTrainingEpisodes) {
        step(state, game, reward, qLearning);

        bool reachedGoal { state.agentX == state.goalX && state.agentY == state.goalY };
        bool truncated { game.steps >= Game::maxSteps && !reachedGoal };

        if (reachedGoal || truncated) {
            finishEpisodes(state, game, reward, qLearning, reachedGoal, config, csvFile);
        }
    }

    auto endTime {std::chrono::steady_clock::now()};
    auto elapsedTime {endTime - startTime};
    std::chrono::duration<double> seconds {elapsedTime};
    
    double duration = seconds.count();
    double stepsPerSecond = duration > 0.0 ? game.totalSteps / duration : 0.0;
    double episodesPerSecond = duration > 0.0 ? game.trainingEpisodes / duration : 0.0;
    
    // formatting
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\nTraining complete\n"
          << "------------------------------\n"
          << "Total episodes: " << game.trainingEpisodes << '\n'
          << "Total steps:    " << game.totalSteps << '\n'
          << "Best steps:    " << game.bestSteps << '\n'
          << "Duration:      " << std::fixed << std::setprecision(2)
          << duration << " s\n";

    // more formatting ...
    if (stepsPerSecond >= 1'000'000) {
        std::cout << "Steps/second:  " << stepsPerSecond / 1'000'000 << "M\n";
    } else if (stepsPerSecond >= 1'000) {
        std::cout << "Steps/second:  " << stepsPerSecond / 1'000 << "K\n";
    } else {
        std::cout << "Steps/second:  " << stepsPerSecond << '\n';
    }

    if (episodesPerSecond >= 1'000) {
        std::cout << "Episodes/sec:  " << episodesPerSecond / 1'000 << "K\n";
    } else {
        std::cout << "Episodes/sec:  " << episodesPerSecond << '\n';
    }

    // headless evaluation (no UI)
    game.mode = Mode::EVALUATION;
    state.agentX = config.startX;
    state.agentY = config.startY;
    game.steps = 0;

    while (game.evaluationEpisodes < config.maxEvaluationEpisodes) {
        step(state, game, reward, qLearning);

        bool reachedGoal { state.agentX == state.goalX && state.agentY == state.goalY };
        bool truncated { game.steps >= Game::maxSteps && !reachedGoal };

        if (reachedGoal || truncated) {
            finishEpisodes(state, game, reward, qLearning, reachedGoal, config, csvFile);
        }
    }

    double evalSuccessRate = game.evaluationEpisodes > 0
        ? (100.0 * game.evaluationSuccesses / game.evaluationEpisodes) : 0.0;
    double evalAvgSteps = game.evaluationEpisodes > 0
        ? (static_cast<double>(game.evaluationTotalSteps) / game.evaluationEpisodes) : 0.0;

    std::cout << "\nEvaluation (headless)\n"
              << "------------------------------\n"
              << "Episodes:     " << game.evaluationEpisodes << '\n'
              << "Success rate: " << evalSuccessRate << "%\n"
              << "Avg steps:    " << evalAvgSteps << '\n';

    if (config.render) {
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

                finishEpisodes(state, game, reward, qLearning, reachedGoal, config, csvFile);

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
