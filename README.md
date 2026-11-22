# GridWorld RL

A personal reinforcement learning project built in C++ using raylib.

The goal is to learn reinforcement learning, C++ systems programming, and performance optimization by building a small RL engine from scratch.

## Features

- 10×10 GridWorld environment
- Tabular Q-learning with epsilon-greedy exploration
- Training and evaluation modes
- Configurable hyperparameters and random seeds
- Headless training for better performance
- CSV logging and Python visualization

## Build

Requires a C++ compiler and raylib.

```bash
g++ -O2 main.cpp game.cpp q_learning.cpp -o grid $(pkg-config --cflags --libs raylib)
```

## Run

Train the agent:

```bash
./grid
```

Train and visualize evaluation:

```bash
./grid --render
```

## Ongoing

Currently working on improving CPU performance and exploring model-based reinforcement learning with Dyna-Q.
