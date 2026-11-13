#include "raylib.h"
#include <iostream>
#include <ctime>

struct Game {
    const int cellSize {60};
    const int gridSize {10};
    int episodeCount {};
    int steps {};

    int totalSteps{};
    int totalRewards{};
};

struct State {
    // AGENT
    int agentX {};
    int agentY {};

    //GOAL
    int goalX{};
    int goalY{};
};

struct Reward {
    const int goalReward {10};
    const int stepPenalty {-1};
    int episodeReward {};
};

enum Choice {
    LEFT,   // 0
    RIGHT,  // 1
    UP,     // 2
    DOWN    // 3

};

void spawnGoal(State& state, const Game& game) {
    // making sure the goal does not spawn on top of the agent
    do {
        state.goalX = GetRandomValue(0, game.gridSize - 1) ;
        state.goalY = GetRandomValue(0, game.gridSize - 1) ;
    } while (state.goalX == state.agentX && state.goalY == state.agentY);
}

void respawn(State& state, Game& game, const Reward& reward) {
    ++game.episodeCount;
    state.agentX = 0;
    state.agentY = 0;

    spawnGoal(state, game);

    std::cout << "Episode " << game.episodeCount << " completed in " << game.steps << " steps" << '\n';
    std::cout << "Reward for this episode: " << reward.episodeReward << " completed in " << game.steps << " steps" << '\n';

    // average performance
    float averageSteps {static_cast<float>(game.totalSteps) / game.episodeCount};
    float averageReward {static_cast<float>(game.totalRewards) / game.episodeCount};

    std::cout << "Average steps: " << averageSteps << '\n';
    std::cout << "Average reward: " << averageReward << '\n';

    game.steps = 0;
}


int main() {
    SetRandomSeed(static_cast<unsigned int>(time(nullptr))); // random seed

    State state{};
    Game game{};
    Reward reward{};
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

            Choice random_choice = (Choice)GetRandomValue(LEFT, DOWN);

            switch (random_choice) {
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

            ++game.steps;
            reward.episodeReward +=  reward.stepPenalty;
            timer -= speed;
        }

        if (state.goalX == state.agentX && state.goalY == state.agentY) {
            reward.episodeReward +=  reward.goalReward;
            game.totalRewards += reward.episodeReward;
            game.totalSteps += game.steps;
            respawn(state, game, reward);
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
