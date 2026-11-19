import csv
from pathlib import Path
import matplotlib.pyplot as plt

csv_path = Path(__file__).resolve().parent.parent / "training.csv"

episodes = []
rewards = []
steps = []

with open(csv_path, "r") as file:
    reader = csv.DictReader(file)

    for row in reader:
        episodes.append(int(row["episode"]))
        rewards.append(int(row["reward"]))
        steps.append(int(row["steps"]))


# moving average
def moving_average(values, window=50):
    averages = []

    for i in range(len(values)):
        start = max(0, i - window + 1)
        recent_values = values[start:i + 1]

        average = sum(recent_values) / len(recent_values)
        averages.append(average)

    return averages


average_rewards = moving_average(rewards)
average_steps = moving_average(steps)


fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))

# rewards
ax1.plot(episodes, rewards, alpha=0.2, label="Episode reward")
ax1.plot(episodes, average_rewards, color="blue", label="50-episode average")

ax1.set_title("Q-Learning: Reward")
ax1.set_xlabel("Episode")
ax1.set_ylabel("Reward")
ax1.legend()
ax1.grid(True)

# steps
ax2.plot(episodes, steps, alpha=0.2, label="Episode steps")
ax2.plot(episodes, average_steps, color="green", label="50-episode average")

ax2.set_title("Q-Learning: Steps")
ax2.set_xlabel("Episode")
ax2.set_ylabel("Steps")
ax2.legend()
ax2.grid(True)

plt.tight_layout()
plt.show()