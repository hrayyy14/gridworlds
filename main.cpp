#include "raylib.h"
#include <iostream>



int main() {
    const int cellSize {60};
    const int gridSize {10};
    const int screenSize { cellSize * gridSize };

    // AGENT
    int agentX {};
    int agentY {};

    //GOAL
    int goalX{};
    int goalY{};
    // making sure the goal does not spawn on top of the agent
    do {
        goalX = GetRandomValue(0, gridSize - 1) ;
        goalY = GetRandomValue(0, gridSize - 1) ;
    } while (goalX != agentX && goalY != agentY);

    bool goalReached { false };

    InitWindow(screenSize, screenSize, "GridWorld");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        if (IsKeyPressed(KEY_LEFT) && (agentX != 0)) {
            agentX += -1;
        }
        if (IsKeyPressed(KEY_RIGHT) && (agentX != (gridSize -1))) {
            agentX += 1;
        }
        if (IsKeyPressed(KEY_UP) && (agentY != 0)) {
            agentY += -1;
        }
        if (IsKeyPressed(KEY_DOWN) && (agentY != (gridSize - 1))) {
            agentY += 1;
        }

        // detect when the agent reaches the goal 
        if (agentX == goalX && agentY == goalY && !goalReached) {
            std::cout << "Goal Reached!\n";
            goalReached = true;
        }

        for (int i {}; i <= gridSize; ++i) {
            const int pos { i * cellSize };
            DrawLine(pos, 0, pos, screenSize, LIGHTGRAY);
            DrawLine(0, pos, screenSize, pos, LIGHTGRAY);

        }

        DrawRectangle(agentX * cellSize, agentY * cellSize, cellSize, cellSize, ORANGE);  
        DrawRectangle(goalX * cellSize, goalY * cellSize, cellSize, cellSize, GREEN);   
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
