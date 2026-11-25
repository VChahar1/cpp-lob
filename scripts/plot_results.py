"""Plot the timing recorded in the committed benchmark output."""

from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt


def main() -> None:
    """Write the observed throughput chart from the saved benchmark log."""
    root = Path(__file__).resolve().parents[1]
    line = next(
        line
        for line in (root / "results/cleanup-benchmark.txt").read_text().splitlines()
        if line.startswith("Throughput:")
    )
    value = float(line.split()[1]) / 1e6
    fig, ax = plt.subplots(figsize=(7, 3.5), constrained_layout=True)
    bars = ax.barh(["Single saved run"], [value], color="#176b60", height=0.4)
    ax.bar_label(bars, fmt="%.4f", padding=5)
    ax.set(
        xlabel="Million events / second",
        title="C++17 order book · synthetic flow · GCC 15.2 Release",
        xlim=(0, value * 1.2),
    )
    ax.spines[["top", "right"]].set_visible(False)
    (root / "assets").mkdir(exist_ok=True)
    fig.savefig(root / "assets/results.png", dpi=160)
    plt.close(fig)


if __name__ == "__main__":
    main()
