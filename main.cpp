#include <iostream>
#include <ctime>
#include <string>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <thread>
#include <vector>

#include "raylib.h"
#include "choice.h"
#include "game.h"
#include "state.h"
#include "reward.h"
#include "q_learning.h"
#include "config.h"
#include "rng.h"
#include "model.h"

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
    if (!config.benchmark && game.mode == Mode::TRAINING && game.trainingEpisodes % 500 == 0) {
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

void step(State& state, Game& game, Reward& reward, QLearning& qLearning, Rng& rng, EnvModel& model, const Config& config) { // one action and one q learning update
    int oldX {state.agentX};
    int oldY {state.agentY};

    Choice chosenAction { chooseAction(state, game, qLearning, rng) };

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

    if (game.mode == Mode::TRAINING) {
        bool reachedGoal { state.agentX == state.goalX && state.agentY == state.goalY };
        int stepReward {reward.stepPenalty + (reachedGoal ? reward.goalReward : 0)};
        // Direct RL update
        updateQTable(qLearning, oldX, oldY, chosenAction, state.agentX, state.agentY, stepReward, reachedGoal);
        // model learning and planning (Dyna Q)
        if (config.planningSteps > 0) {
            model.record(oldX, oldY, chosenAction, state.agentX, state.agentY, stepReward, reachedGoal);
            plan(qLearning, model, rng, config.planningSteps);
        }
    }
    ++game.steps;
    reward.episodeReward += reward.stepPenalty;
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

    if (game.mode == Mode::TRAINING && !config.benchmark) {
        csvFile << game.trainingEpisodes + 1 << ',' << game.steps << ',' << reward.episodeReward << ',' << reachedGoal << '\n';
    }

    respawn(state, game, reward, reachedGoal, config);

    reward.episodeReward = 0;
}

// multithreading
struct WorkerResult {
    int episodes{};
    long long totalSteps{};
    int bestSteps{};
    int evalEpisodes{};
    int evalSuccesses{};
    int evalTotalSteps{};
};
// multithreading
// IMPORTANT!: Each thread runs an independent environment + Q-table (no shared updates):
// avoids lock/cache-line contention on a tiny 1.6KB Q-table and preserves determinism
// Tradeoff: splitting a small episode budget across threads gives each agent fewer episodes to converge
WorkerResult runWorker(const Config& config, unsigned int seed, int episodesToRun) {
    State state{};
    EnvModel model{};
    Game game{};
    Reward reward{config};
    QLearning qLearning{config};
    Rng rng{seed};
    std::ofstream unusedCsv;
    Config localConfig{config};
    localConfig.benchmark = true; // no CSV/stdout inside worker threads

    spawnGoal(state, game, localConfig);

    while (game.trainingEpisodes < episodesToRun) {
        step(state, game, reward, qLearning, rng, model, config);
        bool reachedGoal { state.agentX == state.goalX && state.agentY == state.goalY };
        bool truncated { game.steps >= Game::maxSteps && !reachedGoal };
        if (reachedGoal || truncated) {
            finishEpisodes(state, game, reward, qLearning, reachedGoal, localConfig, unusedCsv);
        }
    }
    // Headless evaluation on this thread's learned Q-table
    game.mode = Mode::EVALUATION;
    state.agentX = localConfig.startX;
    state.agentY = localConfig.startY;
    game.steps = 0;

    while (game.evaluationEpisodes < localConfig.maxEvaluationEpisodes) {
        step(state, game, reward, qLearning, rng, model, config);
        bool reachedGoal { state.agentX == state.goalX && state.agentY == state.goalY };
        bool truncated { game.steps >= Game::maxSteps && !reachedGoal };
        if (reachedGoal || truncated) {
            finishEpisodes(state, game, reward, qLearning, reachedGoal, localConfig, unusedCsv);
        }
    }

    return WorkerResult{
        game.trainingEpisodes,
        game.totalSteps,
        game.bestSteps,
        game.evaluationEpisodes,
        game.evaluationSuccesses,
        game.evaluationTotalSteps
    };
}

int main(int argc, char* argv[]) {
    Config config{};
    EnvModel model{};

    for (int i{1}; i < argc; ++i) {
        std::string arg{argv[i]};
        if (arg == "--render") {
            config.render = true;
        } else if (arg == "--benchmark") {
            config.benchmark = true;
            config.maxTrainingEpisodes = 5'000'000;
        } else if (arg == "--threads" && i + 1 < argc) {
            config.threads = std::stoi(argv[++i]);
        } else if (arg == "--episodes" && i + 1 < argc) {
            config.maxTrainingEpisodes = std::stoi(argv[++i]);
        }
    }

    unsigned int seed = config.randomSeed ? static_cast<unsigned int>(time(nullptr)) : static_cast<unsigned int>(config.seed);

    if (config.threads > 1) {
        std::cout << "\nGridWorld RL (Multithreaded)\n"
                    << "------------------------------\n"
                    << "Threads:  " << config.threads << '\n'
                    << "Episodes: " << config.maxTrainingEpisodes << '\n'
                    << "Seed:     " << seed << "\n\n"
                    << "Training...\n";

        std::vector<std::thread> workers;
        std::vector<WorkerResult> results(config.threads);

        int baseEpisodes { config.maxTrainingEpisodes / config.threads };
        int remainder { config.maxTrainingEpisodes % config.threads };

        auto startTime { std::chrono::steady_clock::now() };

        for (int t{0}; t < config.threads; ++t) {
            int episodesForThread { baseEpisodes + (t < remainder ? 1 : 0) };
            workers.emplace_back([&, t, episodesForThread]() {
                results[t] = runWorker(config, seed + static_cast<unsigned int>(t), episodesForThread);
            });
        }

        for (auto& w : workers) {
            w.join();
        }

        auto endTime { std::chrono::steady_clock::now() };
        double duration = std::chrono::duration<double>(endTime - startTime).count();

        int totalEpisodes{};
        long long totalSteps{};
        int bestSteps{};
        int totalEvalEpisodes{};
        int totalEvalSuccesses{};
        int totalEvalSteps{};

        for (const auto& r : results) {
            totalEpisodes += r.episodes;
            totalSteps += r.totalSteps;
            if (r.bestSteps > 0 && (bestSteps == 0 || r.bestSteps < bestSteps)) {
                bestSteps = r.bestSteps;
            }
            totalEvalEpisodes += r.evalEpisodes;
            totalEvalSuccesses += r.evalSuccesses;
            totalEvalSteps += r.evalTotalSteps;
        }

        double stepsPerSecond = duration > 0.0 ? totalSteps / duration : 0.0;
        double episodesPerSecond = duration > 0.0 ? totalEpisodes / duration : 0.0;

        std::cout << std::fixed << std::setprecision(2)
                    << "\nTraining complete\n"
                    << "------------------------------\n"
                    << "Total episodes: " << totalEpisodes << '\n'
                    << "Total steps:    " << totalSteps << '\n'
                    << "Best steps:     " << bestSteps << '\n'
                    << "Duration:       " << std::setprecision(4) << duration << " s\n" << std::setprecision(2)
                    << "Steps/second:   " << stepsPerSecond / 1'000'000 << "M\n"
                    << "Episodes/sec:   " << episodesPerSecond / 1'000 << "K\n"
                    << "\nEvaluation (all threads)\n"
                    << "------------------------------\n"
                    << "Episodes:       " << totalEvalEpisodes << '\n'
                    << "Success rate:   " << (100.0 * totalEvalSuccesses / totalEvalEpisodes) << "%\n"
                    << "Avg steps:      " << (static_cast<double>(totalEvalSteps) / totalEvalEpisodes) << '\n';
        return 0;
    }

    Rng rng{seed};

    std::ofstream csvFile;
    if (!config.benchmark) {
        csvFile.open("training.csv");
        if (!csvFile.is_open()) {
            std::cerr << "Error: could not open file for writing.\n";
            return 1;
        }
        csvFile << "episode,steps,reward,success\n";
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
        step(state, game, reward, qLearning, rng, model, config);

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
          << "Duration:      " << std::fixed << std::setprecision(4)
          << duration << " s\n" << std::setprecision(2);

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
        step(state, game, reward, qLearning, rng, model, config);

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
                step(state, game, reward, qLearning, rng, model, config);
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
