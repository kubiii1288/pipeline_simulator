import csv
from pathlib import Path

import matplotlib.pyplot as plt

folder = Path(__file__).resolve().parent
with (folder / "results.csv").open(newline="", encoding="utf-8") as file:
    cpi = {
        (row["workload"], row["predictor"]):
            int(row["cycles"]) / int(row["retired_instructions"])
        for row in csv.DictReader(file)
    }

workloads = ["Countdown", "Correlated branches"]
predictors = ["Always not taken", "Bimodal", "Gshare"]
positions = range(len(workloads))
width = 0.24

fig, ax = plt.subplots(figsize=(9, 4.5), constrained_layout=True)

for i, predictor in enumerate(predictors):
    bars = ax.bar(
        [x + (i - 1) * width for x in positions],
        [cpi[workload, predictor] for workload in workloads],
        width,
        label=predictor,
    )
    ax.bar_label(bars, fmt="%.3f", padding=4)

ax.set_xticks(list(positions), workloads)
ax.set_ylabel("Cycles per instruction (CPI)")
ax.set_title("CPI by Branch Predictor and Workload")
ax.set_ylim(0, 1.9)
ax.legend()

fig.savefig(folder / "branch_predictor_cpi.png", dpi=300)
fig.savefig(folder / "branch_predictor_cpi.pdf")
