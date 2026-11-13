#include "raylib.h"
#include <iostream>
#include <ctime>

struct Game {
    const int cellSize {60};
    const int gridSize {10};
    int episodeCount {};
    int steps {};
};

struct State {
    // AGENT
    int agentX {};
    int agentY {};

    //GOAL
    int goalX{};
    int goalY{};
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

void respawn(State& state, Game& game) {
    ++game.episodeCount;
    state.agentX = 0;
    state.agentY = 0;

    spawnGoal(state, game);

    std::cout << "Episode " << game.episodeCount << " completed in " << game.steps << " steps" << '\n';

    game.steps = 0;
}


int main() {
    SetRandomSeed(static_cast<unsigned int>(time(nullptr))); // random seed

    State state{};
    Game game{};
    const int screenSize { game.cellSize * game.gridSize };

    // variables for speed and time
    constexpr float speed {0.2f};
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
                        ++game.steps;
                    }
                    break;
                case RIGHT:
                    if (state.agentX != (game.gridSize -1)) {
                        state.agentX += 1;
                        ++game.steps;
                    }
                    break;
                case UP:
                    if (state.agentY != 0) {
                        state.agentY += -1;
                        ++game.steps;
                    }
                    break;
                case DOWN:
                    if (state.agentY != (game.gridSize - 1)) {
                        state.agentY += 1;
                        ++game.steps;
                    }
                    break;
            }
            timer -= speed;
        }

        if (state.goalX == state.agentX && state.goalY == state.agentY) {
            respawn(state, game);
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
