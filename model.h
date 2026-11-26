#pragma once
#include "game.h"
#include "choice.h"
#include "rng.h"

struct Transition {
    int nextX{};
    int nextY{};
    int reward{};
    int reachedGoal{};
    bool seen{false};
};

struct StateAction {
    int x{};
    int y{};
    Choice action{};
};

struct EnvModel {
    static constexpr int maxEntries { Game::gridSize * Game::gridSize * 4 };
    Transition table[Game::gridSize][Game::gridSize][4] {};
    StateAction visited[maxEntries] {};
    int visitedCount {0};
    inline void record(int oldX, int oldY, Choice action, int nextX, int nextY, int reward, bool reachedGoal) {
        Transition& entry {table[oldX][oldY][action]};
        if (!entry.seen) {
            entry.seen = true;
            visited[visitedCount++] = StateAction{oldX, oldY, action};
        }
        entry.nextX = nextX;
        entry.nextY = nextY;
        entry.reward = reward;
        entry.reachedGoal = reachedGoal;
    }

    inline StateAction sampleRandom(Rng& rng) const {
        return visited[rng.nextInt(0, visitedCount - 1)];
    }
};