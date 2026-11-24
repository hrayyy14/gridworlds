#pragma once
#include <cstdint>

struct Rng {
    uint64_t state;

    explicit Rng(uint64_t seed)
        : state{seed != 0 ? seed : 0x9e3779b97f4a7c15ULL} {}

    inline uint64_t nextU64() {
        uint64_t x = state;
        x ^= x >> 12;
        x ^= x << 25;
        x ^= x >> 27;
        state = x;
        return x * 0x2545F4914F6CDD1DULL;
    }

    inline int nextInt(int min, int max) {
        return min + static_cast<int>(nextU64() % static_cast<uint64_t>(max - min + 1));
    }
};