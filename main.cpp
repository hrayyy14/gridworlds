
#include "raylib.h"


int main() {
    const int cellSize {80};
    const int gridSize {5};
    const int screenSize { cellSize * gridSize };

    InitWindow(screenSize, screenSize, "GridWorld");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        for (int i {}; i <= gridSize; ++i) {
            const int pos { i * cellSize };

            DrawLine(pos, 0, pos, screenSize, LIGHTGRAY);
            DrawLine(0, pos, screenSize, pos, LIGHTGRAY);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
